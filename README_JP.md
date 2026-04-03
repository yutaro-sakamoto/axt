# axt

[English version](README.md)

GNU autotest 代替テスト実行器。

## ビルド

### Linux / macOS

```sh
make
```

flex と bison で C コードを生成し、gcc でコンパイルします。

必要なツール: gcc, make, flex, bison

### Windows

amalgamation された単一ソースファイルから MSVC でビルドします。

```cmd
cl /W3 /D_CRT_SECURE_NO_WARNINGS axt.c /Fe:axt.exe
```

必要なツール: Visual Studio (MSVC)

## シェルの動作

Unix では、AT_CHECK のコマンドは `/bin/sh -c` で実行されます。  
Windows では、AT_CHECK のコマンドは一時 `.bat` ファイルに書き出され、`cmd.exe /C` で実行されます。

そのため、AT_CHECK 内のコマンドは実行プラットフォームのシェル構文に従う必要があります:
- Unix: 標準シェル構文（パイプ、`grep`、`sed` 等）
- Windows: `cmd.exe` 構文（`echo`、`type`、`exit /b` 等）

## テスト

```sh
make test
```

## Dev Container

`.devcontainer/` に AlmaLinux 9 ベースの Dev Container 設定があります。
VS Code や GitHub Codespaces で利用できます。

## ライセンス

このプロジェクトは GPL-3.0 と MIT のデュアルライセンスです。
いずれかのライセンスを選択して利用できます。

- [GPL-3.0](LICENSE-GPL-3.0)
- [MIT](LICENSE-MIT)

SPDX-License-Identifier: `GPL-3.0-or-later OR MIT`
