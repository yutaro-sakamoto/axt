# axt 実装指示

## 概要

`axt` は GNU autotest の代替テスト実行器です。
以下の設計方針に従って実装してください。

---

## 設計方針

- 実装言語は **C** とする
- パーサーは **flex** と **bison** を使って実装する
- 最終成果物は **単一の C ソースファイル (`axt.c`)** に amalgamation する
  - `axt.c` 単体を `cc axt.c -o axt` でビルドできること
  - flex/bison が生成した C コードも `axt.c` に含める
- **Unix 系および Windows 系の両プラットフォーム**で動作すること
  - プラットフォーム固有の処理は内部のOSアブストラクション層で吸収する
  - Unix: `fork`, `execve`, `waitpid`, `pipe`, `dup2`, `mkdtemp` を使用
  - Windows: `CreateProcess`, `WaitForSingleObject`, `CreatePipe`, `GetTempPath` を使用
  - 外部の非標準ライブラリは一切使用しない（ISO C標準 + OSのネイティブAPIのみ）

---

## 対応する .at ファイルの構文

以下のマクロのみをサポートする（GNU autotest の完全な再現は目的としない）。

| マクロ                                            | 意味                                     |
| ------------------------------------------------- | ---------------------------------------- |
| `AT_INIT([title])`                                | テストスイートの宣言                     |
| `AT_SETUP([description])`                         | テストケースの開始                       |
| `AT_DATA(filename, [content])`                    | テスト用ファイルを作業ディレクトリに生成 |
| `AT_CHECK([command], [exit], [stdout], [stderr])` | コマンド実行と結果の検証                 |
| `m4_include([file])`                              | 別の .at ファイルをインクルード          |
| `AT_CLEANUP`                                      | テストケースの終了                       |

AT_CHECK([command]) は AT_CHECK([command], [0], [], []) と同等とする。
AT_CHECK([command], [exit]) は AT_CHECK([command], [exit], [], []) と同等とする。
AT_CHECK([command], [exit], [stdout]) は AT_CHECK([command], [exit], [stdout], []) と同等とする。

### パースの注意事項

- `[` `]` のネストを正しく追跡すること
- `@<:@`と`@:@>`は`[`と`]`のエスケープシーケンスとして扱うこと
- `AT_SETUP` から `AT_CLEANUP` までを1つのテストケース単位とする
- `#` で始まる行はコメントとして無視する

---

## 変数展開

- ツール自身が `${VAR}`や`$VAR` 形式の変数展開を行う
- 展開のタイミングは **実行時**（コマンド実行直前）とする
- 展開対象は後述の環境変数設定ファイルで定義された変数と、実行時の環境変数とする
- `${VAR}` が未定義の場合は **そのテストケースは変数展開エラー**という特別なエラーとして、ログや最終集計で区別して表示すること
- ネスト展開（`${${VAR}}` 等）はサポートしない
- 展開後のコマンド文字列をシェル経由で実行する（シェルの種類はユーザー責任）

---

## 環境変数設定ファイル

- キー・バリュー形式のテキストファイル
- `#` で始まる行はコメント
- 形式:

```
# コメント
CC=clang
CFLAGS=-Wall -O2
LDFLAGS=-lm
```

- 実行時に `--env <file>` オプションで指定する
- 指定された変数は現在のプロセスの環境変数にマージされる（ユーザー定義が優先）
- AT_CHECK のコマンド実行時、この環境変数を子プロセスに渡す

---

## 並列実行

- `-j <N>` オプションで並列数を指定する（デフォルトは 2 = 逐次実行）
- **スレッドプール + タスクキュー**方式を採用する
  - メインスレッドが全テストケースをキューに積む
  - N 個のワーカースレッドがキューからテストケースを取り出して実行する
- 各テストケースは **独立した一時ディレクトリ**で実行する
  - 一時ディレクトリは成功したテストケースは削除、失敗したテストケースは残す
  - axt test_case.atを実行してtest_case.at(とtest_case.src/\*.at)に合計100個のテストケースが書かれていた場合、1つ目のテストケースの一時ディレクトリはtest_case.dir/001/、2つ目のテストケースの一時ディレクトリはtest_case.dir/002/、...、100個目のテストケースの一時ディレクトリはtest_case.dir/100/となるようにすること
  - axt test_case.atを実行してtest_case.at(とtest_case.src/\*.at)に合計1000個のテストケースが書かれていた場合、1つ目のテストケースの一時ディレクトリはtest_case.dir/0001/、2つ目のテストケースの一時ディレクトリはtest_case.dir/0002/、...、1000個目のテストケースの一時ディレクトリはtest_case.dir/1000/となるようにすること
- 並列実行時の出力は各テストの出力をバッファリングし、**テスト番号順**にまとめて出力する
  - ただし、テスト実行中はプログレスバーを表示して全テスト件数と完了件数・失敗件数をリアルタイムで更新する
  - 並列数の分だけプログレスバーの下に「Worker n: (テストケース名)」をリアルタイムで表示する。例えば-j 3ならば以下のような表示がテスト実行中に更新される。

```
[#####..............x]
Progress: 5/20 completed, 1 failed
Worker 1: test_case_1
Worker 2: test_case_2
Worker 3: test_case_3
```

> xは失敗したテストケースを示す。完了したテストケースは#、未完了のテストケースは.で表示する。
> プログレスバーはテストケースの数によらず一定の幅で表示すること。プログレスバーは各テストの状態の割合を示すことが目的で、絶対数を示すことが目的でない

- プログレスバー等のリアルタイム表示は標準エラーに出力し、テストケースの結果のまとめは標準出力に出力すること

---

## コマンドラインインターフェース

```
axt [options] <testfile.at>

Options:
  -j <N>          並列実行数（デフォルト: 2）
  --env <file>    環境変数設定ファイルを指定
  -v              詳細出力モード
  --help          ヘルプを表示
  --version       バージョンを表示
```

ただし、入力データは以下のような形式であることを前提とする。

test_case.at

```
AT_INIT([My Test Suite])

my_include([a.at])
my_include([b.at])
```

test_case.src/a.at

```
AT_SETUP([Test Case 1])
AT_DATA([input1.txt], [a1])
AT_CHECK([cat input1.txt], [0], [a1
])
AT_CLEANUP

AT_SETUP([Test Case 2])
AT_DATA([input2.txt], [a2])
AT_CHECK([cat input2.txt], [0], [a2
])
AT_CLEANUP
```

test_case.src/b.at

```
AT_SETUP([Test Case 1])
AT_DATA([input1.txt], [b1])
AT_CHECK([cat input1.txt], [0], [b1
])
AT_CLEANUP

AT_SETUP([Test Case 2])
AT_DATA([input2.txt], [b2])
AT_CHECK([cat input2.txt], [0], [b2
])
AT_CLEANUP
```

要するに、test_case.atにはAT_INITとmy_includeしか書かれておらず、実際のテストケースはtest_case.src/a.atとtest_case.src/b.atに書かれているということ

## 出力形式

### 通常出力（逐次・並列共通）

## テストスイートタイトル

```

 1: テストケース名                        ok
 2: テストケース名                        FAILED
 3: テストケース名                        ok

---
テスト数: 3  成功: 2  失敗: 1
```

## amalgamation の手順

開発は複数ファイルで行い、`amalgamate.sh`で単一ファイルを生成すること。

---

## 制約・注意事項

- 外部ライブラリは使用しない
- `axt.c` のビルドコマンドは以下で完結すること
  - Unix: `cc axt.c -lpthread -o axt`
  - Windows: `cl axt.c /Fe:axt.exe`
- Windows での `pthread` 相当は Win32 API で実装し、`pthreads-win32` 等のライブラリに依存しない
- テスト実行時のシェルは **プラットフォームに応じて自動選択** される
  - Unix: `/bin/sh -c` でコマンドを実行する
  - Windows: 一時 `.bat` ファイルに書き出し、`cmd.exe /C` で実行する
  - したがって、AT_CHECK 内のコマンドは実行プラットフォームのシェル構文に従う必要がある
