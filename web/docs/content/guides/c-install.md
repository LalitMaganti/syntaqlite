+++
title = "Installing the C libraries"
description = "Download and link libsyntaqlite and libsyntaqlite-parser on Linux, macOS, and Windows."
weight = 3
+++

# Installing the C libraries

syntaqlite has two C libraries:

| Library | Contains | Use it when |
|---|---|---|
| **libsyntaqlite** | Parser, tokenizer, formatter, and analyzer | You need formatting or analysis |
| **libsyntaqlite-parser** | Parser and tokenizer | You only need to parse SQL |

libsyntaqlite-parser is libsyntaqlite's parser and tokenizer on their own, with
the same functions. Every release includes three archives:

| Archive | Contents |
|---|---|
| `libsyntaqlite-<version>.tar.gz` | `syntaqlite.h` and a shared library per platform |
| `libsyntaqlite-parser-<version>.tar.gz` | `libsyntaqlite_parser.h` and a static and shared library per platform |
| `libsyntaqlite-parser-<version>-src.tar.gz` | `libsyntaqlite_parser.h` and `libsyntaqlite_parser.c` |

Each release's notes list every file with its SHA-256.

## libsyntaqlite

```bash
curl -L https://github.com/LalitMaganti/syntaqlite/releases/download/v0.13.5/libsyntaqlite-0.13.5.tar.gz | tar xz
```

```
syntaqlite.h
linux-x64/libsyntaqlite.so
linux-arm64/libsyntaqlite.so
macos-arm64/libsyntaqlite.dylib
macos-x64/libsyntaqlite.dylib
windows-x64/syntaqlite.dll
windows-x64/syntaqlite.dll.lib
windows-arm64/syntaqlite.dll
windows-arm64/syntaqlite.dll.lib
```

Link against the library for your platform:

```bash
# Linux
cc -o my_program my_program.c -I. -Llinux-x64 -lsyntaqlite -Wl,-rpath,'$ORIGIN/linux-x64'

# macOS
cc -o my_program my_program.c -I. -Lmacos-arm64 -lsyntaqlite -Wl,-rpath,@executable_path/macos-arm64
```

On Windows, link the import library from a Developer Command Prompt and put the
DLL next to your program:

```bat
cl /MD /std:c11 /I. my_program.c windows-x64\syntaqlite.dll.lib
copy windows-x64\syntaqlite.dll .
```

## libsyntaqlite-parser

### Prebuilt libraries

```bash
curl -L https://github.com/LalitMaganti/syntaqlite/releases/download/v0.13.5/libsyntaqlite-parser-0.13.5.tar.gz | tar xz
```

```
libsyntaqlite_parser.h
linux-x64/libsyntaqlite_parser.a
linux-x64/libsyntaqlite_parser.so
macos-arm64/libsyntaqlite_parser.a
macos-arm64/libsyntaqlite_parser.dylib
windows-x64/syntaqlite_parser.lib
windows-x64/syntaqlite_parser.dll
windows-x64/syntaqlite_parser.dll.lib
...
```

The archive also has `linux-arm64`, `macos-x64`, and `windows-arm64`
directories. Link the static library to build it into your program:

```bash
# Linux
cc -o my_program my_program.c -I. linux-x64/libsyntaqlite_parser.a

# macOS
cc -o my_program my_program.c -I. macos-arm64/libsyntaqlite_parser.a
```

```bat
:: Windows
cl /MD /std:c11 /I. my_program.c windows-x64\syntaqlite_parser.lib
```

Or link the shared library:

```bash
# Linux
cc -o my_program my_program.c -I. -Llinux-x64 -lsyntaqlite_parser -Wl,-rpath,'$ORIGIN/linux-x64'

# macOS
cc -o my_program my_program.c -I. -Lmacos-arm64 -lsyntaqlite_parser -Wl,-rpath,@executable_path/macos-arm64
```

```bat
:: Windows: put the DLL next to your program
cl /MD /std:c11 /I. my_program.c windows-x64\syntaqlite_parser.dll.lib
copy windows-x64\syntaqlite_parser.dll .
```

The Windows libraries use the dynamic C runtime, so compile your program with
`/MD`.

### C source

```bash
curl -L https://github.com/LalitMaganti/syntaqlite/releases/download/v0.13.5/libsyntaqlite-parser-0.13.5-src.tar.gz | tar xz
```

The archive contains `libsyntaqlite_parser.h` and `libsyntaqlite_parser.c`.
Compile the `.c` file with the rest of your program. It needs a C11 compiler
and nothing else:

```bash
cc -O2 -o my_program my_program.c libsyntaqlite_parser.c
```

On Windows, compile it with clang-cl, since MSVC's C compiler does not support
all the C11 features it uses.
