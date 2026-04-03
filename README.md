# axt

[日本語版 (Japanese)](README_JP.md)

An alternative test runner for GNU autotest.

## Build

### Linux / macOS

```sh
make
```

Generates C code with flex and bison, then compiles with gcc.

Prerequisites: gcc, make, flex, bison

### Windows

Build from the amalgamated single-file source using MSVC:

```cmd
cl /W3 /D_CRT_SECURE_NO_WARNINGS axt.c /Fe:axt.exe
```

Prerequisites: Visual Studio (MSVC)

## Shell Behavior

On Unix, AT_CHECK commands are executed via `/bin/sh -c`.  
On Windows, AT_CHECK commands are written to a temporary `.bat` file and executed via `cmd.exe /C`.

This means the commands inside AT_CHECK must use the syntax of the platform's shell:
- Unix: standard shell syntax (pipes, `grep`, `sed`, etc.)
- Windows: `cmd.exe` syntax (`echo`, `type`, `exit /b`, etc.)

## Test

```sh
make test
```

## Dev Container

An AlmaLinux 9-based Dev Container configuration is available in `.devcontainer/`.
It can be used with VS Code or GitHub Codespaces.

## License

This project is dual-licensed under GPL-3.0 and MIT.
You may choose either license at your option.

- [GPL-3.0](LICENSE-GPL-3.0)
- [MIT](LICENSE-MIT)

SPDX-License-Identifier: `GPL-3.0-or-later OR MIT`
