# axt

[English version](README.md)

GNU autotest 代替テスト実行器。

## ビルド

```sh
make
```

flex と bison で C コードを生成し、gcc でコンパイルします。

必要なツール: gcc, make, flex, bison

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
