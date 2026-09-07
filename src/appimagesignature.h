#ifndef APPIMAGESIGNATURE_H
#define APPIMAGESIGNATURE_H

#include <QByteArray>
#include <QIODevice>
#include <QPair>
#include <QString>
#include <QVector>

// Verify the GPG signature embedded in a type-2 AppImage (issue #85), so the
// self-updater can refuse an image that does not verify against Whatly's own
// signing key before restarting into it.
//
// The scheme is appimagetool's, confirmed end to end against the 7.5.0 release:
// zero the `.sha256_sig` and `.sig_key` ELF sections, take the SHA-256 of the
// result rendered as a 64-character lower-case hex string, and store a detached,
// ASCII-armored GPG signature of that string in `.sha256_sig` (the public key
// goes in `.sig_key`). The trusted key is the one compiled into this binary from
// the committed `whatly-appimage-pubkey.asc`, never the one embedded in the
// incoming image, so a swapped key cannot pass.
namespace AppImageSignature {

enum class Result {
  Good,         // signed, and the signature verifies against the trusted key
  Bad,          // signed, but the signature does not verify (tampered or wrong key)
  Unsigned,     // no signature present (older or self-built images)
  CannotVerify, // signed, but verification could not run (no gpg, unreadable, not an ELF)
};

struct Sections {
  bool isElf = false;     // the prefix begins with a 64-bit little-endian ELF
  bool truncated = false; // a needed region lay beyond the parsed prefix
  bool found = false;     // both .sha256_sig and .sig_key were located
  qint64 sigOffset = 0, sigSize = 0;
  qint64 keyOffset = 0, keySize = 0;
};

// Locate `.sha256_sig` and `.sig_key` by parsing the ELF64 little-endian section
// header table in `elfPrefix` (the head of the file: the table, the
// section-name string table and both sections all live in the runtime at the
// front of an AppImage). `truncated` is set when the header says a needed region
// lies past what was provided. Pure; unit tested.
Sections findSections(const QByteArray &elfPrefix);

// SHA-256 of everything `dev` yields, with the bytes in `zeroRanges`
// (offset, length) treated as zero, rendered as lower-case hex. Streams `dev`,
// so a large image is not held in memory. Pure; unit tested.
QByteArray digestHexWithZeroedRanges(
    QIODevice &dev, const QVector<QPair<qint64, qint64>> &zeroRanges);

// Trim a raw `.sha256_sig` section (an armored signature followed by NUL
// padding) to just the "-----BEGIN/END PGP SIGNATURE-----" block. Empty when
// there is none. Pure; unit tested.
QByteArray armoredSignature(const QByteArray &sigSection);

// The trusted public key compiled into this binary (the committed
// `whatly-appimage-pubkey.asc`). Empty if the resource is missing.
QByteArray trustedPublicKey();

// Full check of the AppImage at `path` against `trustedPublicKeyAsc` (an
// ASCII-armored public key). Shells out to gpg into a throwaway keyring. See
// Result. `gpgProgram` overrides the gpg executable (for tests); empty picks
// gpg/gpg2 from PATH.
Result verify(const QString &path, const QByteArray &trustedPublicKeyAsc,
              const QString &gpgProgram = QString());

} // namespace AppImageSignature

#endif // APPIMAGESIGNATURE_H
