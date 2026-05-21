/*
 * AMWA_UDP_STA_DEMO
 *
 * AMWA-01 を STA モードで起動し、UDP 通信を行うサンプルです。
 * 対向には AMWA_UDP_AP_DEMO を使用してください。
 *
 * 【役割】
 *   - 自分: STA 側
 *   - 相手: AP 側
 *
 * 【起動順】
 *   1. AMWA_UDP_AP_DEMO を書き込んだ Arduino を先に起動
 *   2. AP mode started. が表示されたことを確認
 *   3. AMWA_UDP_STA_DEMO を書き込んだ Arduino を起動
 *
 * 【Serial Monitor 設定】
 *   - baud rate: 115200
 *   - line ending: CRLF
 *   - input limit: 128 characters per line
 *
 * 【IP / Port 対応表】
 *   - STA local IP     : AMWA_IPADDR  = 192.168.11.12
 *   - STA local port   : LOCAL_PORT   = 4098
 *   - STA send to IP   : REMOTE_IP    = 192.168.11.17
 *   - STA send to port : REMOTE_PORT  = 4105
 *
 *   - AP local IP      : 192.168.11.17
 *   - AP local port    : 4105
 *   - AP send to IP    : 192.168.11.12
 *   - AP send to port  : 4098
 *
 * 【メモ】
 *   - Serial Monitor から文字列を入力して CRLF を送ると UDP 送信します。
 *   - RECV: に受信データ、SEND: に送信データを表示します。
 */

#include <AMWA_LIB.h>
//シリアル設定
#define AT_SERIAL  Serial1
#define INFO_SERIAL Serial
//IPアドレス設定
#define AMWA_IPADDR "192.168.11.12"
#define SUBNET "255.255.255.0"
#define GATEWAY "192.168.11.1"
//アクセスポイント設定
#define SSID "AMWx_AP"
#define SEC "sae"
#define PASS "12345678"
//UDP設定
#define LOCAL_PORT 4098
#define REMOTE_IP "192.168.11.17"
#define REMOTE_PORT 4105
#define MAX_INPUT_LEN 128

AMWA wifihalow(false,&AT_SERIAL,&INFO_SERIAL);

int udpid = -1;
String sendStr = "";

void setup() {
  INFO_SERIAL.begin(115200);
  AT_SERIAL.begin(115200);
  sendStr.reserve(MAX_INPUT_LEN);
  delay(1000);
  INFO_SERIAL.println("AMWA UDP STA DEMO Start");

  INFO_SERIAL.println("Initialization started.");
  wifihalow.AMWA_init();
  delay(1000);

  //STAモードに設定して保存し、再起動
  if(!wifihalow.mode_set("STA")){
    INFO_SERIAL.println("Failed to switch to STA mode.");
    NVIC_SystemReset();
  }
  if(!wifihalow.settings_save()){
    INFO_SERIAL.println("Failed to save STA mode.");
    NVIC_SystemReset();
  }
  INFO_SERIAL.println("Rebooting AMWA-01.");
  wifihalow.reboot();
  
  //起動メッセージ待ち
  AMWA::WaitResult res = wifihalow.waitResponce("FW_VERSION:",40000,STARTWITH);
  if(!res.result){
    INFO_SERIAL.println("Failed to start STA mode.");
    NVIC_SystemReset();
  }
  INFO_SERIAL.println("STA mode started.");

  //DHCPをOFF
  if(!wifihalow.dhcp_on(DHCP_DISABLE)){
    INFO_SERIAL.println("Failed to disable DHCP.");
    NVIC_SystemReset();
  }
  //IPアドレスを設定
  if(!wifihalow.ipaddr_set(AMWA_IPADDR, SUBNET, GATEWAY)){
    INFO_SERIAL.println("Failed to set IP address.");
    NVIC_SystemReset();
  }
  //受信モードをパッシブ、イベント無効に設定
  if(!wifihalow.recvmode_set(PASSIVEMODE,EVENT_DISABLE)){
    INFO_SERIAL.println("Failed to set receive mode.");
    NVIC_SystemReset();
  }

  //アクセスポイントに接続
  INFO_SERIAL.print("Connecting to Wi-Fi: ");
  INFO_SERIAL.println(SSID);
  if(!wifihalow.wifiConnect(SSID, SEC, PASS, 60000)){
    INFO_SERIAL.println("Connection failed.");
    NVIC_SystemReset();
  }
  INFO_SERIAL.println("Connected successfully.");
  
  //UDPオープン
  INFO_SERIAL.print("Opening UDP socket on port: ");
  INFO_SERIAL.println(LOCAL_PORT);
  udpid = wifihalow.UDP_Open(LOCAL_PORT);
  if(udpid >= 0){
    INFO_SERIAL.print("Open success, id = ");
    INFO_SERIAL.println(udpid);
  }else{
    INFO_SERIAL.println("Failed to open socket.");
    NVIC_SystemReset();
  }
}

void loop() {
  //Serial Monitorから文字列を受信して、CRLFが来たら送信
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
        if(wifihalow.UDP_Send(udpid,REMOTE_IP,REMOTE_PORT, sendStr )){
        INFO_SERIAL.println(String("SEND:") + sendStr);
        }else{
          INFO_SERIAL.println("UDP send failed.");
        }
      }
      sendStr = "";
    }else if(sendStr.length() > (MAX_INPUT_LEN + 1)){
      INFO_SERIAL.println("Input too long. Cleared.");
      sendStr = "";
    }
  }

  //UDP受信チェック
  int rlen = wifihalow.available(udpid);
  if(rlen > 0){
    //受信があったらデータ読出し
    String rcvStr = wifihalow.passive_recv(udpid,rlen);
    INFO_SERIAL.print("RECV:");
    INFO_SERIAL.println(rcvStr);
  }
  delay(100);
}
