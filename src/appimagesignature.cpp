#include "appimagesignature.h"

#include <QCryptographicHash>
#include <QFile>
#include <QProcess>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QtEndian>

namespace {

// Read a fixed-width little-endian integer at `pos`, bounds-checked against
// `buf`. Sets `ok` to false and returns 0 when the field runs past the buffer.
template <typename T>
T readLE(const QByteArray &buf, qint64 pos, bool &ok) {
  if (pos < 0 || pos + static_cast<qint64>(sizeof(T)) > buf.size()) {
    ok = false;
    return 0;
  }
  return qFromLittleEndian<T>(
      reinterpret_cast<const uchar *>(buf.constData()) + pos);
}

} // namespace

namespace AppImageSignature {

Sections findSections(const QByteArray &elf) {
  Sections s;

  // ELF identification: magic, 64-bit class, little-endian data. Anything else
  // is not something this parser understands, so isElf stays false.
  if (elf.size() < 64 || !elf.startsWith(QByteArrayLiteral("\x7f""ELF")) ||
      elf.at(4) != 2 /* ELFCLASS64 */ || elf.at(5) != 1 /* ELFDATA2LSB */)
    return s;
  s.isElf = true;

  bool ok = true;
  const quint64 shoff = readLE<quint64>(elf, 0x28, ok);      // e_shoff
  const quint16 shentsize = readLE<quint16>(elf, 0x3a, ok);  // e_shentsize
  const quint16 shnum = readLE<quint16>(elf, 0x3c, ok);      // e_shnum
  const quint16 shstrndx = readLE<quint16>(elf, 0x3e, ok);   // e_shstrndx
  if (!ok || shentsize < 64 || shnum == 0 || shstrndx >= shnum) {
    // A malformed or unsupported header (e.g. section-count in the first entry,
    // which real AppImage runtimes do not use) counts as "cannot parse here".
    s.truncated = true;
    return s;
  }

  const qint64 tableEnd =
      static_cast<qint64>(shoff) + static_cast<qint64>(shnum) * shentsize;
  if (tableEnd > elf.size()) {
    s.truncated = true;
    return s;
  }

  // The section-name string table, named by e_shstrndx.
  bool ok2 = true;
  const qint64 strEntry = static_cast<qint64>(shoff) + shstrndx * shentsize;
  const quint64 strOff = readLE<quint64>(elf, strEntry + 24, ok2);  // sh_offset
  const quint64 strSize = readLE<quint64>(elf, strEntry + 32, ok2); // sh_size
  if (!ok2 || static_cast<qint64>(strOff + strSize) > elf.size()) {
    s.truncated = true;
    return s;
  }

  const auto sectionName = [&](quint32 nameOff) -> QByteArray {
    if (nameOff >= strSize)
      return {};
    const qint64 start = static_cast<qint64>(strOff) + nameOff;
    const int end = elf.indexOf('\0', start);
    if (end < 0)
      return {};
    return elf.mid(start, end - start);
  };

  bool sawSig = false, sawKey = false;
  for (quint16 i = 0; i < shnum; ++i) {
    const qint64 e = static_cast<qint64>(shoff) + i * shentsize;
    bool ok3 = true;
    const quint32 nameOff = readLE<quint32>(elf, e + 0, ok3);   // sh_name
    const quint64 off = readLE<quint64>(elf, e + 24, ok3);      // sh_offset
    const quint64 size = readLE<quint64>(elf, e + 32, ok3);     // sh_size
    if (!ok3)
      continue;
    const QByteArray name = sectionName(nameOff);
    if (name == ".sha256_sig") {
      s.sigOffset = static_cast<qint64>(off);
      s.sigSize = static_cast<qint64>(size);
      sawSig = true;
    } else if (name == ".sig_key") {
      s.keyOffset = static_cast<qint64>(off);
      s.keySize = static_cast<qint64>(size);
      sawKey = true;
    }
  }
  s.found = sawSig && sawKey;
  return s;
}

QByteArray digestHexWithZeroedRanges(
    QIODevice &dev, const QVector<QPair<qint64, qint64>> &zeroRanges) {
  QCryptographicHash hash(QCryptographicHash::Sha256);
  const int kChunk = 1 << 20; // 1 MiB
  qint64 pos = 0;
  for (;;) {
    QByteArray chunk = dev.read(kChunk);
    if (chunk.isEmpty())
      break;
    // Zero any bytes of this chunk that fall inside a zeroed range.
    for (const auto &r : zeroRanges) {
      const qint64 rStart = r.first, rEnd = r.first + r.second;
      const qint64 cStart = pos, cEnd = pos + chunk.size();
      const qint64 lo = qMax(rStart, cStart), hi = qMin(rEnd, cEnd);
      for (qint64 b = lo; b < hi; ++b)
        chunk[static_cast<int>(b - cStart)] = '\0';
    }
    hash.addData(chunk);
    pos += chunk.size();
  }
  return hash.result().toHex();
}

QByteArray armoredSignature(const QByteArray &sigSection) {
  static const QByteArray begin = QByteArrayLiteral("-----BEGIN PGP SIGNATURE-----");
  static const QByteArray end = QByteArrayLiteral("-----END PGP SIGNATURE-----");
  const int b = sigSection.indexOf(begin);
  if (b < 0)
    return {};
  const int e = sigSection.indexOf(end, b);
  if (e < 0)
    return {};
  return sigSection.mid(b, e - b + end.size()) + '\n';
}

QByteArray trustedPublicKey() {
  QFile f(QStringLiteral(":/appimage/whatly-appimage-pubkey.asc"));
  if (!f.open(QIODevice::ReadOnly))
    return {};
  return f.readAll();
}

Result verify(const QString &path, const QByteArray &trustedPublicKeyAsc,
              const QString &gpgProgram) {
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly))
    return Result::CannotVerify;

  // The section header table and both sections live in the runtime at the front
  // of the image; 8 MiB covers it with room to spare, and avoids reading the
  // whole (large) file just to parse the ELF.
  const QByteArray prefix = file.read(8 << 20);
  const Sections s = findSections(prefix);
  if (!s.isElf || s.truncated)
    return Result::CannotVerify;
  if (!s.found)
    return Result::Unsigned; // no signature sections in this runtime

  // The raw signature section is present but empty on an unsigned image.
  const QByteArray sigRaw = prefix.mid(static_cast<int>(s.sigOffset),
                                       static_cast<int>(s.sigSize));
  const QByteArray sig = armoredSignature(sigRaw);
  if (sig.isEmpty())
    return Result::Unsigned;

  if (trustedPublicKeyAsc.isEmpty())
    return Result::CannotVerify; // no key to trust against

  QString gpg = gpgProgram;
  if (gpg.isEmpty())
    gpg = QStandardPaths::findExecutable(QStringLiteral("gpg"));
  if (gpg.isEmpty())
    gpg = QStandardPaths::findExecutable(QStringLiteral("gpg2"));
  if (gpg.isEmpty())
    return Result::CannotVerify; // nothing to verify with

  // Recompute the signed digest: the whole file with the two sections zeroed.
  file.seek(0);
  const QByteArray digestHex = digestHexWithZeroedRanges(
      file, {{s.sigOffset, s.sigSize}, {s.keyOffset, s.keySize}});
  if (digestHex.size() != 64)
    return Result::CannotVerify;

  // A throwaway keyring holding only the trusted key: gpg then reports a good
  // signature only when it was made by that key, so the exit code alone is
  // enough. GNUPGHOME must be short (the agent's socket path has a length
  // limit), so use the system temp root rather than anywhere deep.
  QTemporaryDir home;
  if (!home.isValid())
    return Result::CannotVerify;

  const QString keyPath = home.filePath(QStringLiteral("key.asc"));
  const QString sigPath = home.filePath(QStringLiteral("image.sig"));
  const QString digPath = home.filePath(QStringLiteral("digest.txt"));
  {
    QFile kf(keyPath), sf(sigPath), df(digPath);
    if (!kf.open(QIODevice::WriteOnly) || kf.write(trustedPublicKeyAsc) < 0 ||
        !sf.open(QIODevice::WriteOnly) || sf.write(sig) < 0 ||
        !df.open(QIODevice::WriteOnly) || df.write(digestHex) < 0)
      return Result::CannotVerify;
  }

  QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
  env.insert(QStringLiteral("GNUPGHOME"), home.path());
  const QStringList common{QStringLiteral("--homedir"), home.path(),
                           QStringLiteral("--batch"), QStringLiteral("--no-tty"),
                           QStringLiteral("--quiet")};

  auto run = [&](const QStringList &args) -> int {
    QProcess p;
    p.setProcessEnvironment(env);
    p.setProgram(gpg);
    p.setArguments(common + args);
    p.start();
    if (!p.waitForStarted(5000))
      return -1;
    if (!p.waitForFinished(20000)) {
      p.kill();
      return -1;
    }
    return p.exitStatus() == QProcess::NormalExit ? p.exitCode() : -1;
  };

  if (run({QStringLiteral("--import"), keyPath}) != 0)
    return Result::CannotVerify;

  const int rc = run({QStringLiteral("--verify"), sigPath, digPath});
  if (rc == 0)
    return Result::Good;
  if (rc < 0)
    return Result::CannotVerify; // gpg could not run to a verdict
  return Result::Bad;            // ran, and rejected the signature
}

} // namespace AppImageSignature
