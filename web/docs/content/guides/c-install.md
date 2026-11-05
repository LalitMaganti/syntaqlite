+++
title = "Installing the C libraries"
description = "Download and link libsyntaqlite and libsyntaqlite-parser on Linux, macOS, and Windows."
weight = 5
+++

# Installing the C libraries

syntaqlite has two C libraries:

| Library | Contains | Use it when |
|---|---|---|
| **libsyntaqlite** | Parser, tokenizer, formatter, and analyzer | You need formatting or analysis |
| **libsyntaqlite-parser** | Parser and tokenizer | You only need to parse SQL |

If you only need to parse SQL, libsyntaqlite-parser has two advantages. It is
much smaller: a few hundred kilobytes, where libsyntaqlite is a megabyte or
more. And it is plain C, so you can build it from source with nothing but a C
compiler and add it to any build system. libsyntaqlite's formatter and analyzer
are written in Rust, so it is only released prebuilt.

libsyntaqlite has the same parser functions as libsyntaqlite-parser, so code
written for one builds against the other with a change of header.

Each release has three C archives:

| Archive | Contents |
|---|---|
| `libsyntaqlite-<version>.tar.gz` | `syntaqlite.h` and a static and shared library per platform |
| `libsyntaqlite-parser-<version>.tar.gz` | `libsyntaqlite_parser.h` and a static and shared library per platform |
| `libsyntaqlite-parser-<version>-src.tar.gz` | `libsyntaqlite_parser.h` and `libsyntaqlite_parser.c` |

The release notes give the SHA-256 of every file.

## libsyntaqlite

```bash
curl -L https://github.com/LalitMaganti/syntaqlite/releases/download/v0.13.5/libsyntaqlite-0.13.5.tar.gz | tar xz
```

```
syntaqlite.h
linux-x64/libsyntaqlite.a
linux-x64/libsyntaqlite.so
macos-arm64/libsyntaqlite.a
macos-arm64/libsyntaqlite.dylib
windows-x64/syntaqlite.lib
windows-x64/syntaqlite.dll
windows-x64/syntaqlite.dll.lib
...
```

There are also `linux-arm64`, `macos-x64`, and `windows-arm64` directories. To
link the shared library:

```bash
# Linux
cc -o my_program my_program.c -I. -Llinux-x64 -lsyntaqlite -Wl,-rpath,'$ORIGIN/linux-x64'

# macOS
cc -o my_program my_program.c -I. -Lmacos-arm64 -lsyntaqlite -Wl,-rpath,@executable_path/macos-arm64
```

On Windows, link against the import library from a Developer Command Prompt,
then copy the DLL next to your program:

```bat
cl /MD /std:c11 /I. my_program.c windows-x64\syntaqlite.dll.lib
copy windows-x64\syntaqlite.dll .
```

To build libsyntaqlite into your program instead, link the static library.
Because libsyntaqlite is partly written in Rust, on Linux and Windows it also
needs the system libraries Rust's standard library uses:

```bash
# Linux
cc -o my_program my_program.c -I. linux-x64/libsyntaqlite.a -lpthread -ldl -lm -lrt -lutil -lgcc_s

# macOS
cc -o my_program my_program.c -I. macos-arm64/libsyntaqlite.a
```

```bat
:: Windows
cl /MD /std:c11 /I. my_program.c windows-x64\syntaqlite.lib ntdll.lib userenv.lib ws2_32.lib dbghelp.lib
```

A program can only contain one static Rust library, because each brings its
own copy of Rust's standard library. If yours already links another one, use
the shared libsyntaqlite.

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

There are also `linux-arm64`, `macos-x64`, and `windows-arm64` directories. To
build the parser into your program, link the static library:

```bash
# Linux
cc -o my_program my_program.c -I. linux-x64/libsyntaqlite_parser.a -ldl

# macOS
cc -o my_program my_program.c -I. macos-arm64/libsyntaqlite_parser.a
```

```bat
:: Windows
cl /MD /std:c11 /I. my_program.c windows-x64\syntaqlite_parser.lib
```

On Linux with glibc older than 2.34, add `-ldl`. The parser uses `dlopen` to
load custom dialects, and older glibc versions keep it in a separate library.

To link the shared library instead:

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

The Windows libraries are built against the dynamic C runtime, so compile your
program with `/MD` too.

### C source

```bash
curl -L https://github.com/LalitMaganti/syntaqlite/releases/download/v0.13.5/libsyntaqlite-parser-0.13.5-src.tar.gz | tar xz
```

The archive has `libsyntaqlite_parser.h` and `libsyntaqlite_parser.c`. Add the
`.c` file to your build. It needs a C11 compiler:

```bash
cc -O2 -o my_program my_program.c libsyntaqlite_parser.c -ldl
```

Again, add `-ldl` on Linux with glibc older than 2.34. On Windows, use
clang-cl: MSVC's C compiler doesn't support all the C11 features the parser
uses.
