# axt

[日本語版 (Japanese)](README_JP.md)

An alternative test runner for GNU autotest.

## Build

```sh
make
```

Generates C code with flex and bison, then compiles with gcc.

Prerequisites: gcc, make, flex, bison

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
