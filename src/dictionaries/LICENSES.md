# Spell-check dictionary licenses

These are [hunspell](https://github.com/hunspell/hunspell) dictionaries (a
`.dic` word list and its `.aff` affix rules) that the build converts to the
`.bdic` format Chromium's spell checker reads (`qwebengine_convert_dict`).

## Provenance

They come from the [LibreOffice dictionaries](https://wiki.documentfoundation.org/Language_support_of_LibreOffice)
project (as packaged by Debian and Ubuntu in `libreoffice-dictionaries`), except
where an individual file's own header states otherwise. Each dictionary keeps
the license its upstream gives it; they are not one work and not all the same
license.

## How they are distributed

The dictionaries are **separate data files**, not linked into the Whatly
executable (which is MIT). They are conveyed two ways, both as standalone files:

- optionally bundled into a package (`WHATLY_BUNDLE_DICTIONARIES`; the default is
  `NONE`, so packages ship none), and
- served from the `dictionaries` GitHub release for on-demand download (issue
  #46) and first-run fetch (issue #110).

Several are copyleft (GPL, LGPL, AGPL). They are aggregated with an MIT program
as independent data, not as a derivative of it. Their corresponding source form
(the `.dic` and `.aff` in this directory) is kept in the repository and
published, which is what satisfies the source-availability requirement for the
copyleft ones.

The full texts of the GNU copyleft licenses in use are included under
[`licenses/`](licenses): `GPL-2.0.txt`, `GPL-3.0.txt`, `LGPL-2.1.txt`,
`LGPL-3.0.txt` and `AGPL-3.0.txt`. `GPL-3.0.txt` is kept because `LGPL-3.0` is
written as a set of additional permissions on top of it (and `it_IT`/`es_ES` are
offered under GPL-3.0 directly). The remaining licenses that appear below
(MPL-1.1 and MPL-2.0, the BSD variants, Apache-2.0, the Creative Commons ones,
SISSL, and the SCOWL permissive terms for `en_US`) are named by their SPDX
identifier only; their standard texts are at spdx.org/licenses. Add any of them
under `licenses/` if a distribution channel requires the text delivered
alongside the binary.

## Per-dictionary license

Where a dictionary file's own header declares a license, that governs and is what
the table shows (`en_GB`, `en_AU`, `eo`); the rest follow the upstream
`libreoffice-dictionaries` copyright, matched by language.

| Code | Language | License |
|---|---|---|
| ca_ES | Catalan (Spain) | GPL-2.0-or-later |
| cs_CZ | Czech | GPL-2.0-only |
| da_DK | Danish | LGPL-2.1-only OR GPL-2.0-only OR MPL-1.1 |
| de_DE | German | GPL-2.0-only OR GPL-3.0-only |
| de_DE_neu | German (reformed spelling) | GPL-2.0-only OR GPL-3.0-only |
| el_GR | Greek | MPL-1.1 OR GPL-2.0-only OR LGPL-2.1-only |
| en_AU | English (Australia) | LGPL (declared in en_AU.aff) |
| en_GB | English (United Kingdom) | LGPL (declared in en_GB.aff) |
| en_US | English (United States) | SCOWL-based permissive (LibreOffice "custom0") |
| eo | Esperanto | GPL-2.0-or-later |
| es_ES | Spanish (Spain) | GPL-3.0-or-later OR LGPL-3.0-or-later OR MPL-1.1-or-later |
| fr_FR | French | MPL-2.0 |
| he_IL | Hebrew | AGPL-3.0-or-later |
| hi_IN | Hindi | GPL-2.0-or-later |
| hr_HR | Croatian | LGPL OR SISSL |
| id_ID | Indonesian | LGPL-3.0 |
| it_IT | Italian | GPL-3.0-only |
| lt_LT | Lithuanian | BSD-3-Clause |
| lv_LV | Latvian | LGPL-2.1-or-later |
| nb_NO | Norwegian Bokmål | GPL-2.0-only |
| nl_NL | Dutch | BSD-2-Clause OR CC-BY-3.0 |
| pl_PL | Polish | GPL OR LGPL OR MPL OR Apache-2.0 OR CC-SA-1.0 |
| pt_BR | Portuguese (Brazil) | LGPL-3.0 OR MPL |
| pt_PT | Portuguese (Portugal) | GPL-2.0-only OR LGPL-2.1-only OR MPL-1.1 |
| ro_RO | Romanian | GPL-2.0-only |
| ru_RU | Russian | BSD-4-Clause |
| sk_SK | Slovak | GPL-2.0-only OR LGPL-2.1-only OR MPL-1.1 |
| sl_SI | Slovenian | GPL OR LGPL |
| sv_SE | Swedish | LGPL-3.0 |
| vi_VN | Vietnamese | GPL-2.0-only |

A multi-license entry ("A OR B") is offered by upstream under any one of those,
the recipient's choice.

## Updating this file

When a dictionary is added or replaced (a new `.dic`/`.aff` in this directory),
add or update its row here from the source's own header first, then the
`libreoffice-dictionaries` copyright (`/usr/share/doc/hunspell-*/copyright` on
Debian and Ubuntu), and re-run the **Publish dictionaries** workflow.
