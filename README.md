# AMWA-01用Arduinoライブラリ

## 概要

"Wi-Fi HaLow ArduinoシールドAMWA-01"をArduino R4から操作するためのライブラリです。<br>
AMWA-01 ATコマンドマニュアルとあわせて読んでいただくと、わかりやすいです。

## ライブラリ説明

* **AMWA**<br>
クラスです。

* **AMWA(bool on, Stream *amwa_serial, Stream *arduino_serial)**<br>
  コンストラクタです。<br>
  ・bool on : trueでシリアルモニタにUART RXDが表示されるようになる(デバッグ用)<br>
  ・Stream *amwa_serial : AMWA-01と通信するためのUART<br>
  ・Stream *arduino_serial : シリアルモニタに使用するUART<br>

* **void AT_Send(String atcmd, String para)**<br>
ATコマンドを送信する関数です。<br>
・String atcmd : 送信するコマンド<br>
・String para : 送信するパラメータ<br>

* **bool ipaddr_set(String ipaddr, String subnet, String gateway)**<br>
IPアドレスを設定する関数です。(AT+IPADDRコマンド)<br>
・String ipaddr : デバイスのIPv4アドレス<br>
・String subnet : デバイスのネットマスク<br>
・String gateway : デバイスのデフォルトゲートウェイ<br>

* **bool dhcp_on(int enable)**<br>
DHCP有効設定をする関数です。(AT+DHCPコマンド)<br>
・int enable : DHCP状態 0=DHCP無効, 1=DHCP有効<br>

* **void AMWA_init( void )**<br>
初期化およびリセットをする関数です。<br>

* **WaitResult waitResponce(String res, int timeout_ms, int mode=ALLMATCH)**<br>
AMWA-01からのUARTレスポンスを待つ関数です。<br>
・String res : 待ちたいレスポンス文字列<br>
・int timeout_ms : タイムアウト時間[msec]<br>
・int mode : 待ちモード ALLMATCH=完全一致, STARTWITH=前方一致, ENDWITH=後方一致<br>

* **bool wifiConnect(String ssid, String security , String pass, int timeout_ms)**<br>
アクセスポイントに接続する関数です。(AT+WCONNコマンド)<br>
・String ssid : アクセスポイントの識別名<br>
・String security : アクセスポイントの暗号化方式、“sae”、“owe”、“open”のいずれかを入力してください。<br>
・String pass : アクセスポイントに接続するためのパスワード<br>
・int timeout_ms : タイムアウト時間[msec]<br>

* **int UDP_Open(uint16_t port)**<br>
UDPソケットを作成する関数です。(AT+SOPENコマンド)<br>
・uint16_t port : 通信に使用するポート番号<br>

* **bool UDP_Send(int id, String ipaddr, uint16_t port, String sendstr)**<br>
UDP送信をする関数です。(AT+SSENDコマンド)<br>
・int id : <br>
・String ipaddr : 通信先IPv4アドレス<br>
・uint16_t port : 通信先ポート番号<br>
・String  sendstr : 送信文字列<br>

* **int TCP_Server_Open(uint16_t port)**<br>
TCPサーバソケットを作成する関数です。(AT+SOPENコマンド)<br>
・uint16_t port : クライアントを待つポート番号<br>

* **int TCP_Client_Open(String ipaddr, uint16_t port)**<br>
TCPクライアントソケットを作成する関数です。(AT+SOPENコマンド)<br>
・String ipaddr : 接続先TCPサーバのIPv4アドレス<br>
・uint16_t port : 接続先TCPサーバのポート番号<br>

* **bool TCP_Send(int id, String sendstr)**<br>
TCP送信をする関数です。(AT+SSENDコマンド)<br>
・int id : ソケットID<br>
・String sendstr : 送信文字列<br>

* **bool Socket_Close(uint16_t id)**<br>
ソケット終了関数です。(AT+SCLOSEコマンド)<br>
・uint16_t id : ソケットID<br>

* **int available(int id)**<br>
データ受信を確認する関数です。(AT+SRECVコマンド)<br>
・int id : ソケットID<br>

* **String passive_recv(int id, int len)**<br>
パッシブモード時にデータ受信を行う関数です。(AT+SRECVコマンド)<br>
・int id : ソケットID<br>
・int len : 受信するバイト数<br>

* **bool recvmode_set(int mode, int event)**<br>
データ受信モードを設定する関数です。(AT+SRECVMODEコマンド)<br>
・int mode : 受信モード切り替え 0:アクティブ 1:パッシブ<br>
・int event : 受信完了イベント設定 0:イベント無効 1:イベント有効<br>

## 使用ガイド

### 用意するもの

### ライブラリの追加

### デモの書き込み

### 動作確認

## 著者

アニモテック株式会社

## ライセンス情報

This project is licensed under the [NAME HERE] License - see the LICENSE.md file for details

