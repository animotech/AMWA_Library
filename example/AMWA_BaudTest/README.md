# AMWA_BaudTest

AMWA-01 と Arduino 間の UART baud rate ごとの応答安定性を確認するサンプルです。

このスケッチは AMWA-01 を AT モードに入れたあと、`AT` と `AT+SAUDP?` を複数回送信し、各 baud rate で応答 timeout / 不一致が起きた回数を集計します。

## 使い方

1. Arduino に AMWA-01 を接続します。
2. `AMWA_BaudTest.ino` を書き込みます。
3. Serial Monitor を開きます。
4. テスト完了後の `Summary` を確認します。

## 調整項目

`AMWA_BaudTest.ino` 先頭の `#define` で変更できます。

- `SIMPLE_ITERATIONS`: `AT` の試行回数
- `BURST_ITERATIONS`: `AT+SAUDP?` の試行回数
- `TEST_BAUDS`: テストする baud rate
- `ECHO_AT_RESPONSES`: `1` にすると AMWA-01 の応答行を Serial Monitor に echo します

## 注意

- AMWA_Library の通常 example は、AMWA-01 側の保存 UART baud が `115200` であることを前提にしています。
- `AT+UART` による baud 変更は runtime 設定です。flash には保存されません。
- スケッチは最後に AMWA-01 の UART を `115200` に戻します。
- テスト中に止めた場合、AMWA-01 が低速 baud のまま残ることがあります。その場合は AMWA-01 を電源再投入してください。
