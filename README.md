# AMWA-01用Arduinoライブラリ<!-- omit in toc -->

## 目次<!-- omit in toc -->

- [概要](#概要)
- [ライブラリ説明](#ライブラリ説明)
- [デモ説明](#デモ説明)
- [使用ガイド](#使用ガイド)
- [著者](#著者)
- [ライセンス情報](#ライセンス情報)

## 概要

"Wi-Fi HaLow ArduinoシールドAMWA-01"をArduino Uno R4から操作するためのライブラリです。<br>
AMWA-01 ATコマンドマニュアルとあわせて読んでいただくと、わかりやすいです。

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
初期化およびリセットをする関数です。<br>
引数<br>
・なし<br>
返り値<br>
・なし<br>

* **WaitResult waitResponce(String res, int timeout_ms, int mode=ALLMATCH)**<br>
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
・int : 受信文字列

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

* **int apsta_get(String *mac_list)**<br>
AP モードに接続中の STA の MAC アドレス一覧を取得する関数です。(AT+WAPSTA?コマンド)<br>
+WAPSTA:xx:xx:xx:xx:xx:xx の応答を順に読み取り、取得した MAC アドレスを `mac_list` に格納します。接続中の STA が無い場合や取得に失敗した場合は `0` を返します。<br>
引数<br>
・String *mac_list : 取得した MAC アドレス文字列の格納先配列<br>
戻り値<br>
・int : 取得できた MAC アドレス数

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
引数<br>
・uint16_t local_port : ローカルポート<br>
・String remote_ip : 相手 IP アドレス<br>
・uint16_t remote_port : 相手ポート<br>
返り値<br>
・bool : 処理が正常終了したらtrue

* **bool auto_udp_set(uint16_t local_port, String remote_ip, uint16_t remote_port, uint32_t baud)**<br>
AutoUDP を有効化し、AutoUDP モード突入時に切り替える UART baud も同時に設定する関数です。(AT+SAUDPコマンド)<br>
設定は `settings_save()` で保存後、`reboot()` で次回起動から AutoUDP モードが動作します。baud を変更する場合は host 側シリアルも同じ baud に切り替える必要があります。<br>
引数<br>
・uint16_t local_port : ローカルポート<br>
・String remote_ip : 送信先 IP アドレス<br>
・uint16_t remote_port : 送信先ポート<br>
・uint32_t baud : AutoUDP モード突入時に切り替える UART baud (9600〜921600)<br>
戻り値<br>
・bool : 実行が正常終了したらtrue

* **bool auto_udp_set(uint16_t local_port, String remote_ip, uint16_t remote_port, uint32_t baud, uint8_t rx_format)**<br>
AutoUDP を有効化し、UART baud と AutoUDP 受信出力形式も同時に設定する関数です。(AT+SAUDPコマンド)<br>
設定は `settings_save()` で保存後、`reboot()` で次回起動から AutoUDP モードが動作します。`rx_format` には `AUTOUDP_RX_FORMAT_HEADER` または `AUTOUDP_RX_FORMAT_RAW` を指定します。<br>
引数<br>
・uint16_t local_port : ローカルポート<br>
・String remote_ip : 送信先 IP アドレス<br>
・uint16_t remote_port : 送信先ポート<br>
・uint32_t baud : AutoUDP モード突入時に切り替える UART baud (9600〜921600)<br>
・uint8_t rx_format : `AUTOUDP_RX_FORMAT_HEADER` = `+RXD` ヘッダ付き / `AUTOUDP_RX_FORMAT_RAW` = payload のみ<br>
戻り値<br>
・bool : 実行が正常終了したらtrue

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

* **void set_baud_switch_callback(BaudSwitchCallback cb)**<br>
AutoUDP モード突入時に chip から `+UART_SWITCH:<baud>` が出力されたとき、host 側シリアル設定を切り替える callback を登録する関数です。<br>
115200 以外の baud を使う AutoUDP を利用する場合に使用します。`AMWA_init()` の中でも host 側を 115200 に戻すため callback が呼ばれます。`NULL` を渡すと callback を解除します。<br>
引数<br>
・BaudSwitchCallback cb : 新しい baud 値 (`uint32_t`) を受け取る callback 関数ポインタ<br>
戻り値<br>
・なし

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

* **AMWA_AutoUDP_AP**<br>
AMWA-01 を AP（アクセスポイント）モードで起動し、AutoUDP 通信を自動開始するサンプルです。<br>
AMWA_AutoUDP_STA と組み合わせて使用します。<br>
シリアルモニタから入力した文字が STA 側に送信され、STA 側からの受信データが表示されます。

* **AMWA_AutoUDP_STA**<br>
AMWA-01 を STA（ステーション）モードで起動し、AutoUDP 通信を自動開始するサンプルです。<br>
AMWA_AutoUDP_AP と組み合わせて使用します。<br>
シリアルモニタから入力した文字が AP 側に送信され、AP 側からの受信データが表示されます。

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

### AP-STA AutoUDP サンプルの使い方<!-- omit in toc -->

AMWA_AutoUDP_AP と AMWA_AutoUDP_STA を使って 2 台の Arduino 間で Wi-Fi HaLow UDP 通信を行います。

#### 用意するもの<!-- omit in toc -->

* AMWA-01 × 2 台
* Arduino Uno R4 × 2 台
* USB ケーブル × 2 本

#### 手順<!-- omit in toc -->

1. AP 側 Arduino に AMWA_AutoUDP_AP を書き込む
2. STA 側 Arduino に AMWA_AutoUDP_STA を書き込む
3. コード上部の `#define` 設定値を確認する（`AP_SSID` / `AP_PASS` / IP アドレス等）
4. AP 側を先に電源 ON する
5. STA 側を電源 ON する
6. 双方のシリアルモニタ（115200bps）を開き、`=== AutoUDP started ===` が表示されることを確認する
7. シリアルモニタに文字を入力して送信ボタンを押すと、相手側のシリアルモニタに表示される

#### デフォルト設定値<!-- omit in toc -->

| 項目 | AP 側 | STA 側 |
|---|---|---|
| SSID | `testap` | `testap` |
| 暗号化 | `sae` | `sae` |
| パスワード | `12345678` | `12345678` |
| 自身の IP | `192.168.1.123` | `192.168.1.33` |
| サブネット | `255.255.255.0` | `255.255.255.0` |
| ゲートウェイ | `192.168.1.1` | `192.168.1.1` |
| ローカルポート | 4099 | 4099 |
| 相手 IP | `192.168.1.33` | `192.168.1.123` |
| 相手ポート | 4099 | 4099 |
| AP チャンネル | 13 | - |

ゲートウェイは孤立した AP-STA 構成では実通信に使われないため、`192.168.1.1` のままで動作します。
わかりやすくしたい場合は AP 側 IP を `192.168.1.1`、STA 側 IP を `192.168.1.2` に変更しても問題ありません。

#### 設定を変更したいとき<!-- omit in toc -->

一度 AutoUDP 設定が AMWA-01 の不揮発メモリに保存されると、Arduino リセットだけでは設定を変更できません。
設定を変更したい場合は、以下のいずれかの方法を取ってください。

**方法 A: スケッチの `FORCE_RECONFIG` を使う（推奨）**

1. .ino 上部の `#define FORCE_RECONFIG 0` を `1` に変更する
2. 設定値も同時に変更する
3. Arduino にビルド & 書き込む
4. 1 回起動して新しい設定が書き込まれたことをシリアルモニタで確認
5. `#define FORCE_RECONFIG 1` を `0` に戻して再ビルド & 書き込み（任意。次回起動を高速化するため）

**方法 B: 手動で AT* を送る**

1. シリアルモニタを開いた状態で AMWA-01 の電源を入れ直す（または Arduino をリセット）
2. `AutoUDP` の出力が表示された直後（5 秒以内）に `AT*` と入力して送信
3. `exit` の応答が出れば AT モードに復帰しているので、自由に AT コマンドで再設定可能

#### 制約事項<!-- omit in toc -->

* AutoUDP モード突入後は AT コマンドが受け付けられません（UART 入力はそのまま UDP 送信に流れます）
* AT モードへ復帰するには起動後 5 秒以内に `AT*` を送る必要があります
* AP モードでは DHCP サーバが動作しないため、STA 側は固定 IP を設定する必要があります
* AP モードでは AP 経由でも子機（STA）同士の通信はできません
* AP モードに接続できる STA は最大 2 台までです
* このサンプルは AP 1 台 + STA 1 台の 1 対 1 通信を前提としています


## 著者

アニモテック株式会社

## ライセンス情報


