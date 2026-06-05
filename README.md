# AMWA-01用Arduinoライブラリ<!-- omit in toc -->

## 目次<!-- omit in toc -->

- [概要](#概要)
- [重要: example の UART baud 前提](#重要-example-の-uart-baud-前提)
- [ライブラリ説明](#ライブラリ説明)
- [デモ説明](#デモ説明)
- [使用ガイド](#使用ガイド)
- [著者](#著者)
- [ライセンス情報](#ライセンス情報)

## 概要

"Wi-Fi HaLow ArduinoシールドAMWA-01"をArduino Uno R4から操作するためのライブラリです。<br>
AMWA-01 ATコマンドマニュアルとあわせて読んでいただくと、わかりやすいです。

## 重要: example の UART baud 前提

このライブラリの example は、AMWA-01 側の保存 UART baud が `115200` であることを前提にしています。<br>
AMWA-01 は reset 後、`AT+UARTW=<baud>` で不揮発保存されている baudrate で起動します。<br>
通常 example を使う前に、保存 baud が `115200` であることを確認してください。baud 実験などで保存 baud を変更した場合は、`AT+UARTW=115200` に戻してから example を使用してください。<br>
example 内の `AT_SERIAL.begin(115200)` はこの前提に合わせて固定しています。

## ライブラリ説明

* **AMWA**<br>
クラスです。

* **WaitResult**<br>
WaitResult構造体です。<br>
    bool result : 文字列待ちの結果<br>
    String restr : 来た文字列<br>

* **AMWA(bool on, Stream *amwa_serial, Stream *arduino_serial)**<br>
  コンストラクタです。<br>
  ・bool on : trueでシリアルモニタにUART RXDが表示されるようになる(デバッグ用)<br>
  ・Stream *amwa_serial : AMWA-01と通信するためのUART<br>
  ・Stream *arduino_serial : シリアルモニタに使用するUART<br>

* **void AT_Send(String atcmd, String para)**<br>
ATコマンドを送信する関数です。<br>
引数<br>
・String atcmd : 送信するコマンド<br>
・String para : 送信するパラメータ<br>
返り値<br>
・なし<br>

* **bool ipaddr_set(String ipaddr, String subnet, String gateway)**<br>
IPアドレスを設定する関数です。(AT+WIPADDRコマンド)<br>
引数<br>
・String ipaddr : デバイスのIPv4アドレス<br>
・String subnet : デバイスのネットマスク<br>
・String gateway : デバイスのデフォルトゲートウェイ<br>
返り値<br>
・bool : 処理が正常終了したらtrue

* **bool dhcp_on(int enable)**<br>
DHCP有効設定をする関数です。(AT+WDHCPコマンド)<br>
引数<br>
・int enable : DHCP状態 0=DHCP無効, 1=DHCP有効<br>
返り値<br>
・bool : 処理が正常終了したらtrue

* **void AMWA_init( void )**<br>
AMWA-01 をハードウェアリセットする関数です。<br>
現仕様では、reset 後の chip は `AT+UARTW` で不揮発保存されている baudrate で起動します。<br>
そのため host 側シリアルは、reset 前から保存済み baudrate に合わせておく必要があります。<br>
この関数自体は baudrate を変更せず、reset pin のトグルだけを行います。<br>
引数<br>
・なし<br>
返り値<br>
・なし<br>

* **WaitResult waitResponse(String res, int timeout_ms, int mode=ALLMATCH)**<br>
AMWA-01からのUARTレスポンスを待つ関数です。<br>
引数<br>
・String res : 待ちたいレスポンス文字列<br>
・int timeout_ms : タイムアウト時間[msec]<br>
・int mode : 待ちモード ALLMATCH=完全一致, STARTWITH=前方一致, ENDWITH=後方一致<br>
返り値<br>
・WaitResult : 処理が正常終了したらWaitResult.resultがtrue


* **bool wifiConnect(String ssid, String security , String pass, int timeout_ms)**<br>
アクセスポイントに接続する関数です。(AT+WCONNコマンド)<br>
引数<br>
・String ssid : アクセスポイントの識別名<br>
・String security : アクセスポイントの暗号化方式、“sae”、“owe”、“open”のいずれかを入力してください。<br>
・String pass : アクセスポイントに接続するためのパスワード<br>
・int timeout_ms : タイムアウト時間[msec]<br>
返り値<br>
・bool : 処理が正常終了したらtrue

* **int UDP_Open(uint16_t port)**<br>
UDPソケットを作成する関数です。(AT+SOPENコマンド)<br>
引数<br>
・uint16_t port : 通信に使用するポート番号<br>
返り値<br>
・int : ソケットID, 失敗したら-1

* **bool UDP_Send(int id, String ipaddr, uint16_t port, String sendstr)**<br>
UDP送信をする関数です。(AT+SSENDコマンド)<br>
引数<br>
・int id : <br>
・String ipaddr : 通信先IPv4アドレス<br>
・uint16_t port : 通信先ポート番号<br>
・String  sendstr : 送信文字列<br>
返り値<br>
・bool : 処理が正常終了したらtrue

* **int TCP_Server_Open(uint16_t port)**<br>
TCPサーバソケットを作成する関数です。(AT+SOPENコマンド)<br>
引数<br>
・uint16_t port : クライアントを待つポート番号<br>
返り値<br>
・int : ソケットID, 失敗したら-1

* **int TCP_Client_Open(String ipaddr, uint16_t port)**<br>
TCPクライアントソケットを作成する関数です。(AT+SOPENコマンド)<br>
引数<br>
・String ipaddr : 接続先TCPサーバのIPv4アドレス<br>
・uint16_t port : 接続先TCPサーバのポート番号<br>
返り値<br>
・int : ソケットID, 失敗したら-1

* **bool TCP_Send(int id, String sendstr)**<br>
TCP送信をする関数です。(AT+SSENDコマンド)<br>
引数<br>
・int id : ソケットID<br>
・String sendstr : 送信文字列<br>
返り値<br>
・bool : 処理が正常終了したらtrue

* **bool Socket_Close(uint16_t id)**<br>
ソケット終了関数です。(AT+SCLOSEコマンド)<br>
引数<br>
・uint16_t id : ソケットID<br>
返り値<br>
・bool : 処理が正常終了したらtrue

* **int available(int id)**<br>
データ受信を確認する関数です。(AT+SRECVコマンド)<br>
引数<br>
・int id : ソケットID<br>
返り値<br>
・int : 正常終了したら受信バイト数, 失敗したら-1

* **String passive_recv(int id, int len)**<br>
パッシブモード時にデータ受信を行う関数です。(AT+SRECVコマンド)<br>
引数<br>
・int id : ソケットID<br>
・int len : 受信するバイト数<br>
返り値<br>
・String : 受信文字列。受信失敗時は空文字列

* **bool recvmode_set(int mode, int event)**<br>
データ受信モードを設定する関数です。(AT+SRECVMODEコマンド)<br>
引数<br>
・int mode : 受信モード切り替え 0:アクティブ 1:パッシブ<br>
・int event : 受信完了イベント設定 0:イベント無効 1:イベント有効<br>
返り値<br>
・bool : 処理が正常終了したらtrue

* **bool mode_set(String mode)**<br>
次回起動モードを設定する関数です。(AT+WMODEコマンド)<br>
即時切替ではなく configstore への予約となります。実際に切り替えるには settings_save() + reboot() が必要です。<br>
引数<br>
・String mode : "AP" または "STA"<br>
返り値<br>
・bool : 処理が正常終了したらtrue

* **bool ap_config_set(String ssid, String security, String password, uint16_t channel)**<br>
AP モード用アクセスポイント設定をする関数です。(AT+WAPCFGコマンド)<br>
security が "open" の場合は password 引数は無視されます。"sae" / "open" 以外を渡すと AT 送信せず即 false を返します。<br>
引数<br>
・String ssid : AP の SSID<br>
・String security : 暗号化方式 "sae" または "open"（owe は不可）<br>
・String password : パスワード（sae 時のみ使用）<br>
・uint16_t channel : S1G チャンネル番号（0 = country list 先頭の自動選択）<br>
返り値<br>
・bool : 処理が正常終了したらtrue。security が不正なら false

* **bool ap_ip_set(String ipaddr, String netmask, String gateway)**<br>
AP モード用 IP アドレス設定をする関数です。(AT+WAPIPコマンド)<br>
引数<br>
・String ipaddr : AP 自身の IPv4 アドレス<br>
・String netmask : サブネットマスク<br>
・String gateway : ゲートウェイ<br>
返り値<br>
・bool : 処理が正常終了したらtrue

* **int apsta_get(String *mac_list, int max_count)**<br>
AP モードに接続中の STA の MAC アドレス一覧を取得する関数です。(AT+WAPSTA? コマンド)<br>
`+WAPSTA:xx:xx:xx:xx:xx:xx` の応答を順に読み取り、取得した MAC アドレスを `mac_list` に格納します。<br>
`max_count` を超えて書き込まないため、受け取り配列の容量を必ず指定してください。<br>
引数<br>
・String *mac_list : 取得した MAC アドレス文字列を格納する配列<br>
・int max_count : `mac_list` に格納してよい最大件数<br>
戻り値<br>
・`-1` : コマンド失敗 / timeout / 不正引数<br>
・`0` : 接続中の STA なし (`+WAPSTA:NONE`)<br>
・`1以上` : 取得できた STA 数

* **bool sta_ap_set(String ssid, String security, String password)**<br>
STA 接続先 AP 設定をする関数です。(AT+WAPコマンド)<br>
wifiConnect() と異なり接続は行わず、credentials の保存のみを行います。security が "open"/"owe" の場合は password 引数は無視されます。"sae"/"owe"/"open" 以外を渡すと AT 送信せず即 false を返します。<br>
引数<br>
・String ssid : 接続先 AP の SSID または BSSID<br>
・String security : 暗号化方式 "sae" / "owe" / "open"<br>
・String password : パスワード（sae 時のみ使用）<br>
返り値<br>
・bool : 処理が正常終了したらtrue。security が不正なら false

* **bool settings_save()**<br>
設定を不揮発メモリに保存する関数です。(AT+WSAVEコマンド)<br>
STA/AP 両方の設定と次回起動モードを一括保存します。<br>
引数<br>
・なし<br>
返り値<br>
・bool : 処理が正常終了したらtrue

* **void reboot()**<br>
AMWA-01 を再起動する関数です。(ATZコマンド)<br>
ATZ は応答せず即チップリセットされるため、応答待ちはしません。<br>
引数・返り値ともになし<br>

* **bool baudrate_setting_set(int baudrate)**<br>
不揮発メモリに保存する UART baudrate を設定する関数です。(AT+UARTW コマンド)<br>
この設定は即時反映されず、保存設定だけが更新されます。<br>
引数<br>
・int baudrate : 保存する baudrate<br>
戻り値<br>
・bool : 正常終了したら true

* **int baudrate_setting_get(void)**<br>
不揮発メモリに保存されている UART baudrate を取得する関数です。(AT+UARTW? コマンド)<br>
引数<br>
・なし<br>
戻り値<br>
・int : 保存されている baudrate、取得失敗時は -1

注意点: 不揮発メモリへ書き込むと寿命が減るため、`baudrate_setting_set()` は何度も繰り返し書き込みすぎないようにしてください。<br>

* **bool auto_udp_set(uint16_t local_port, String remote_ip, uint16_t remote_port)**<br>
AutoUDP を有効化する関数です。(AT+SAUDPコマンド)<br>
設定は settings_save() で保存後、reboot() で次回起動から AutoUDP モードが動作します。<br>
AutoUDP の baud はこの関数では変更しません。UART baud は chip 全体の不揮発設定 `AT+UARTW=<baud>` で扱います。<br>
引数<br>
・uint16_t local_port : ローカルポート (1〜65535。0 は不可)<br>
・String remote_ip : 相手 IP アドレス<br>
・uint16_t remote_port : 相手ポート (1〜65535。0 は不可)<br>
返り値<br>
・bool : 処理が正常終了したらtrue

* **bool auto_udp_disable()**<br>
AutoUDP を無効化する関数です。(AT+SAUDP=0コマンド)<br>
引数<br>
・なし<br>
返り値<br>
・bool : 処理が正常終了したらtrue

* **bool auto_udp_escape(unsigned long timeout_ms)**<br>
AutoUDP モード起動直後（"AutoUDP" 出力後 5 秒以内）に AT* を送って AT モードへ抜ける関数です。<br>
detect_boot_state() で BOOT_AUTOUDP が返ったあと、再設定したい場合に呼び出します。<br>
引数<br>
・unsigned long timeout_ms : "exit" 応答待ちタイムアウト（firmware の exit ウィンドウは 5 秒固定なので 6000 程度推奨）<br>
返り値<br>
・bool : AT モード復帰時 true（"exit" 応答受信）


* **AMWA::BootState detect_boot_state(unsigned long timeout_ms)**<br>
AMWA-01 の起動状態を判別する関数です。<br>
"AutoUDP" 出力を待つことで設定済み判別を行い、検出しなかった場合は "AT" を送って "OK" 応答を確認します。<br>
引数<br>
・unsigned long timeout_ms : "AutoUDP" 出力検出に費やす時間（5000ms 以上推奨）<br>
返り値<br>
・BOOT_AUTOUDP : AutoUDP モードで起動した（設定済み）<br>
・BOOT_AT_MODE : 通常 AT モードで起動した（未設定）<br>
・BOOT_TIMEOUT : 応答なし

* **bool wait_autoudp_started(unsigned long timeout_ms)**<br>
AutoUDP モード突入後 socket オープン完了まで待つ関数です。<br>
"start" や "+WEVENT:*"（CONNECT_SUCCESS / LINK_UP / APSTART_SUCCESS など）はログ出力として通過させ、"+SOPEN:" を検出したら成功とします。<br>
途中で "exit"（AT* で AutoUDP を抜けた場合）や "ERROR:" を検出した場合は早期に false を返します。<br>
引数<br>
・unsigned long timeout_ms : 全体タイムアウト（AP は 30000、STA は 60000 程度を推奨）<br>
返り値<br>
・bool : "+SOPEN:" 受信時 true / "exit" or "ERROR:" 受信時 false / タイムアウト時 false

## デモ説明
 
* **AMWA_ATcommand_demo**<br>
AT_Send関数を使用してUDPエコーを行うサンプルです。

* **AMWA_Serial_echo**<br>
AMWA-01との通信に使用するUARTと、シリアルモニタに使用するUARTをソフト的に直接接続します。<br>
シリアルモニタから直接ATコマンド操作ができるようになります。

* **AMWA_BaudTest**<br>
AMWA-01 の UART baud 設定に対する応答性を確認するサンプルです。<br>
`AT` と `AT+SAUDP?` を送信し、各 baud rate での timeout / 不一致回数を集計します。

* **AMWA_TCP_client_echo**<br>
AMWA-01がTCPクライアントになり、指定したTCPサーバーから受信したデータをエコーします。

* **AMWA_TCP_server_echo**<br>
AMWA-01がTCPサーバにーなり、TCPクライアントから受信したデータをエコーします。

* **AMWA_UDP_echo**<br>
UDPエコーを行うサンプルです。

* **AMWA_UDP_AP_DEMO**<br>
AMWA-01 を AP モードで起動し、UDP 通信を行うサンプルです。<br>
AMWA_UDP_STA_DEMO と組み合わせて使用します。<br>
シリアルモニタから入力した文字列を UDP 送信し、受信データを表示します。

* **AMWA_UDP_STA_DEMO**<br>
AMWA-01 を STA モードで起動し、UDP 通信を行うサンプルです。<br>
AMWA_UDP_AP_DEMO と組み合わせて使用します。<br>
シリアルモニタから入力した文字列を UDP 送信し、受信データを表示します。

* **AMWA_AUTOUDP_AP_DEMO**<br>
AMWA-01 を AP モード + AutoUDP で起動するサンプルです。<br>
AMWA_AUTOUDP_STA_DEMO と組み合わせて使用します。<br>
シリアルモニタ入力を AutoUDP 送信し、受信データを表示します。

* **AMWA_AUTOUDP_STA_DEMO**<br>
AMWA-01 を STA モード + AutoUDP で起動するサンプルです。<br>
AMWA_AUTOUDP_AP_DEMO と組み合わせて使用します。<br>
シリアルモニタ入力を AutoUDP 送信し、受信データを表示します。

* **AMWA_TCP_SERVER_AP_DEMO**<br>
AMWA-01 を AP モードで起動し、TCP server として動作するサンプルです。<br>
AMWA_TCP_CLIENT_STA_DEMO と組み合わせて使用します。<br>
シリアルモニタから入力した文字列を TCP 送信し、受信データを表示します。

* **AMWA_TCP_CLIENT_STA_DEMO**<br>
AMWA-01 を STA モードで起動し、TCP client として動作するサンプルです。<br>
AMWA_TCP_SERVER_AP_DEMO と組み合わせて使用します。<br>
シリアルモニタから入力した文字列を TCP 送信し、受信データを表示します。

* **AMWA_TCP_CLIENT_AP_DEMO**<br>
AMWA-01 を AP モードで起動し、TCP client として動作するサンプルです。<br>
AMWA_TCP_SERVER_STA_DEMO と組み合わせて使用します。<br>
シリアルモニタから入力した文字列を TCP 送信し、受信データを表示します。

* **AMWA_TCP_SERVER_STA_DEMO**<br>
AMWA-01 を STA モードで起動し、TCP server として動作するサンプルです。<br>
AMWA_TCP_CLIENT_AP_DEMO と組み合わせて使用します。<br>
シリアルモニタから入力した文字列を TCP 送信し、受信データを表示します。

## 使用ガイド

### 用意するもの<!-- omit in toc -->

ハードウェア
* Wi-Fi HaLow ArduinoシールドAMWA-01
* Arduino Uno R4 minima または Wi-Fi
* Wi-Fi Halow アクセスポイント

ソフトウェア
* Arduino IDE

### ライブラリの追加<!-- omit in toc -->

このリポジトリをダウンロードし、以下のガイドに従ってArduino IDEにライブラリを追加します。<br>
https://docs.arduino.cc/software/ide-v1/tutorials/installing-libraries/

### デモのコンパイル/書き込み<!-- omit in toc -->

Arduino IDEのツールバーFile->Examples->AMWA_Library->AMWA_UDP_echoを選択します。
これはUDP受信をエコー送信するデモです。<br>
コード上部でdefineされているIPアドレス設定、アクセスポイント設定、UDP設定を自身の環境にあわせて変更してください。<br>
Arduino IDEの左上にあるチェックボタンを押してコンパイルし、コンパイルが通ることを確認します。<br>
コンパイルに問題がなければ、Arduino Uno R4(HaLowシールド装着済み)をPCに接続して、矢印ボタンを押して書き込みます。

### 動作確認<!-- omit in toc -->

シリアルモニタを確認し、以下のような表示になっていればアクセスポイントに接続成功しています。<br>
```
AMWA UDP DEMO Start
init start
wi-fi connect to ekh01-cb95
connect success
udp open, port:4098
open succsess, id = 0
```
PCなどからUDP送信をすると、エコーが返ってくることを確認します。また、シリアルモニタに以下のような表示が出ることを確認します。(以下では、hello udpを送信しています)
```
udp rcv: hello udp
udp send
send succsess
```

## 著者

アニモテック株式会社

## ライセンス情報
