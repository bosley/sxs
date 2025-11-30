# Syntax

These syntax files can be installed on the system for vs code or cursor.

The goal of these syntax files is to handle all CORE commands and to make working with paren language easier
parens, braces and brackets are all tracked, type symbols (:int :real, etc) symbols like "#" "@" "'" marked to show the variants.

## Installation

Install to VS Code:
```bash
./syntax.sh --install
```

Install to Cursor:
```bash
./syntax.sh --install --cursor
```

Uninstall from VS Code:
```bash
./syntax.sh --uninstall
```

Uninstall from Cursor:
```bash
./syntax.sh --uninstall --cursor
```

Reinstall:
```bash
./syntax.sh --reinstall [--cursor]
```

## Features

The syntax highlighter supports:
- Core commands: def, fn, if, match, reflect, try, recover, assert, eval, debug, export, apply
- Datum commands: load, import
- Type symbols: :int, :real, :str, :symbol, :list, :list-p, :list-b, :list-c, :none, :rune, :aberrant, :error, :some, :datum, :any, :fn (including variadic forms with ..)
- Variant prefixes: # @ ' ?
- Imported identifiers: kv/open-memory, alu/add_r, io/print, etc.
- Scoped identifiers: testmem:int, x:aaa, store:key:subkey (used for KV stores)
- Special variables: $error, $exception
- Brackets: () [] {}
- Comments: ; semicolon-style
- Literals: integers, reals, strings