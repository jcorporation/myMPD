---
layout: page
permalink: /references/translating
title: Translating
---

myMPD has a simple translation framework integrated. It is inspired by polyglot.js.

Following translations are available and maintained:

* English (en-US) - default
* German (de-DE)

User contributed translations:

* Regulary updated:
  * Korean (ko-KR)
  * Dutch (nl-NL)
  * Simplified Chinese (cn-CHS)
* Outdated (not any longer included in build):
  * Finish (fi-FI)
  * French (fr-FR)
  * Italian (it-IT)
  * Spanish (es-VE)

## Translate

The translation files are imported to a public POEditor project. You can join the project at https://poeditor.com/join/project/Z54inZwdul and help translating.

GitHub discussion: https://github.com/jcorporation/myMPD/discussions/803

## Translation files

Translations are defined in one file per language. The translation files resides under `<srcdir>/src/i18n/` and are named by the language code, e.g. `en-US.txt` The perl script `tojson.pl` creates the json files from these files, do not edit the json files directly.

### Syntax

* First line of the file must be the friendly name of the language (optionally following a blank line)
* A phrase is in one line, in the next line the translation and then one blank line
* `%{smart_count}` are used for pluralization
  * ` |||| ` separates the pluralization forms
  * the phrase before is used for number one
  * the phrase after is user for numbers zero or greater than one
  * more pluralization forms can be easily added (please open an issue)
* all other `%{variables}` are replaced with values

### Example

```
English

Num playlists
%{smart_count} Playlist |||| %{smart_count} Playlists
```

Expands to `1 Playlist` or `5 Playlists` or `0 Playlists`

### Generating the translation file

You can generate the translation files manually. The translation files are written to `<srcdir>/i18n/json/`. The build process shows all missing or obsolete translations.

```
./build.sh translate
```

### Adding missing translations

This example is for the german translation (de-DE).

1. Show missing translation phrases for one language: `./build.sh translate 2>&1 | grep "de-DE"`
2. Add translation to file: `src/i18n/de-DE.txt`
