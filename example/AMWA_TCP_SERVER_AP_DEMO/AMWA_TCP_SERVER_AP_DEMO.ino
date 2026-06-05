/*
 * AMWA_TCP_SERVER_AP_DEMO
 *
 * AMWA-01 を AP モードで起動し、TCP server として動作するサンプルです。
 * 対向には AMWA_TCP_CLIENT_STA_DEMO を使用してください。
 *
 * 役割:
 *   - 自分: AP / TCP server
 *   - 相手: STA / TCP client
 *
 * 起動順:
 *   1. AMWA_TCP_SERVER_AP_DEMO を書き込んだ Arduino を起動
 *   2. AP mode started. が表示されたことを確認
 *   3. AMWA_TCP_CLIENT_STA_DEMO を書き込んだ Arduino を起動
 *
 * Serial Monitor 設定:
 *   - baud rate: 115200
 *   - line ending: CRLF
 *   - input limit: 128 characters per line
 *
 * IP / Port 対応表:
 *   - AP local IP      : AMWA_IPADDR  = 192.168.11.17
 *   - AP listen port   : LOCAL_PORT   = 4099
 *   - STA local IP     : 192.168.11.12
 *   - STA connect port : 4099
 *
 * メモ:
 *   - Serial Monitor から文字列を入力して CRLF を送ると TCP 送信します。
 *   - RECV: に受信データ、SEND: に送信データを表示します。
 */

#include <AMWA_LIB.h>
#define AT_SERIAL  Serial1
#define INFO_SERIAL Serial
#define AMWA_IPADDR "192.168.11.17"
#define SUBNET "255.255.255.0"
#define GATEWAY "192.168.11.1"
#define SSID "AMWx_AP"
#define SEC "sae"
#define PASS "12345678"
#define CHANNEL 13
#define LOCAL_PORT 4099
#define MAX_INPUT_LEN 128

AMWA wifihalow(false,&AT_SERIAL,&INFO_SERIAL);

int tcpsid = -1;
int connectedClientId = -1;
String sendStr = "";

void setup() {
  INFO_SERIAL.begin(115200);
  AT_SERIAL.begin(115200);
  sendStr.reserve(MAX_INPUT_LEN);
  delay(1000);
  INFO_SERIAL.println("AMWA TCP SERVER AP DEMO Start");

  INFO_SERIAL.println("Initialization started.");
  wifihalow.AMWA_init();
  delay(1000);

  // APモード設定
  if(!wifihalow.mode_set("AP")){
    INFO_SERIAL.println("Failed to switch to AP mode.");
    NVIC_SystemReset();
  }

  // AP設定
  if(!wifihalow.ap_config_set(SSID, SEC, PASS, CHANNEL)){
    INFO_SERIAL.println("Failed to set AP config.");
    NVIC_SystemReset();
  }

  // APのIP設定
  if(!wifihalow.ap_ip_set(AMWA_IPADDR, SUBNET, GATEWAY)){
    INFO_SERIAL.println("Failed to set AP IP address.");
    NVIC_SystemReset();
  }

  // 設定保存
  if(!wifihalow.settings_save()){
    INFO_SERIAL.println("Failed to save AP mode.");
    NVIC_SystemReset();
  }

  //再起動
  INFO_SERIAL.println("Rebooting AMWA-01.");
  wifihalow.reboot();

  // AP起動待ち
  AMWA::WaitResult res = wifihalow.waitResponse("+WEVENT:APSTART_SUCCESS",40000,STARTWITH);
  if(!res.result){
    INFO_SERIAL.println("Failed to start AP mode.");
    NVIC_SystemReset();
  }
  INFO_SERIAL.println("AP mode started.");

  // 受信モードをパッシブ、イベント有効に設定
  if(!wifihalow.recvmode_set(PASSIVEMODE,EVENT_ENABLE)){
    INFO_SERIAL.println("Failed to set receive mode.");
    NVIC_SystemReset();
  }

  // TCP server オープン
  INFO_SERIAL.print("Opening TCP server on port: ");
  INFO_SERIAL.println(LOCAL_PORT);
  tcpsid = wifihalow.TCP_Server_Open(LOCAL_PORT);
  if(tcpsid >= 0){
    INFO_SERIAL.print("Open success, id = ");
    INFO_SERIAL.println(tcpsid);
  }else{
    INFO_SERIAL.println("Failed to open socket.");
    NVIC_SystemReset();
  }
}

void loop() {
  // client接続待ち
  if (connectedClientId == -1)
  {
    //tcp client 接続確認
    AMWA::WaitResult res = wifihalow.waitResponse("+SEVENT:CONNECT,", 1000, STARTWITH);
    if (res.result)
    {
      // OKだった場合、idを取得
      String idstr = res.restr.substring(16);
      connectedClientId = idstr.toInt();
      INFO_SERIAL.println("Client connected.");
    }
  }
  else
  {
    // Serial Monitorから文字列を受信して、CRLFが来たら送信
    while(INFO_SERIAL.available() > 0){
      char c = (char)INFO_SERIAL.read();
      sendStr += c;
      if(sendStr.endsWith("\r\n")){
        sendStr.remove(sendStr.length() - 2);
        if(sendStr.length() > MAX_INPUT_LEN){
          INFO_SERIAL.println("Input too long. Cleared.");
          sendStr = "";
          continue;
        }
        if(sendStr.length() > 0){
          if(wifihalow.TCP_Send(connectedClientId, sendStr)){
            INFO_SERIAL.println(String("SEND:") + sendStr);
          }else{
            INFO_SERIAL.println("TCP send failed.");
          }
        }
        sendStr = "";
      }else if(sendStr.length() > (MAX_INPUT_LEN + 1)){
        INFO_SERIAL.println("Input too long. Cleared.");
        sendStr = "";
      }
    }

    // TCP受信チェック
    int rlen = wifihalow.available(connectedClientId);
    if(rlen > 0){
      // 受信があったらデータ読出し
      String rcvStr = wifihalow.passive_recv(connectedClientId,rlen);
      INFO_SERIAL.print("RECV:");
      INFO_SERIAL.println(rcvStr);
    }else if(rlen == -1){
      // 切断されたら接続待ちへ戻る
      if(!wifihalow.socket_exists(connectedClientId)){
        connectedClientId = -1;
        INFO_SERIAL.println("Client disconnected.");
      }
    }
    delay(100);
  }
}
