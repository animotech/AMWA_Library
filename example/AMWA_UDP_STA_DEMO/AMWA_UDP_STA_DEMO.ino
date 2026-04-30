#include <AMWA_LIB.h>
//シリアル設定
#define AT_SERIAL  Serial1
#define INFO_SERIAL Serial
//IPアドレス設定
#define AMWA_IPADDR "192.168.11.17"
#define SUBNET "225.255.255.0"
#define GATEWAY "192.168.11.1"
//アクセスポイント設定
#define SSID "MegaChips"
#define SEC "sae"
#define PASS "12345678"
//UDP設定
#define LOCAL_PORT 4098
#define REMOTE_IP "192.168.11.12"
#define REMOTE_PORT 4105

AMWA wifihalow(false,&AT_SERIAL,&INFO_SERIAL);

int udpid = -1;
String sendStr = "";

void setup() {
  INFO_SERIAL.begin(115200);
  AT_SERIAL.begin(115200);
  delay(1000);
  INFO_SERIAL.println("AMWA UDP STA DEMO Start");

  INFO_SERIAL.println("Initialization started.");
  wifihalow.AMWA_init();
  delay(1000);

  //STAモードに設定して保存し、再起動
  if(!wifihalow.mode_set("STA")){
    INFO_SERIAL.println("Failed to switch to STA mode.");
    while(1){ delay(1000); }
  }
  if(!wifihalow.settings_save()){
    INFO_SERIAL.println("Failed to save STA mode.");
    while(1){ delay(1000); }
  }
  INFO_SERIAL.println("Rebooting AMWA-01.");
  wifihalow.reboot();
  delay(3000);

  //アクセスポイント情報を設定
  if(!wifihalow.sta_ap_set(SSID, SEC, PASS)){
    INFO_SERIAL.println("Failed to set the Wi-Fi profile.");
    while(1){ delay(1000); }
  }

  //DHCPをOFF
  if(!wifihalow.dhcp_on(DHCP_DISABLE)){
    INFO_SERIAL.println("Failed to disable DHCP.");
    while(1){ delay(1000); }
  }
  //IPアドレスを設定
  if(!wifihalow.ipaddr_set(AMWA_IPADDR, SUBNET, GATEWAY)){
    INFO_SERIAL.println("Failed to set IP address.");
    while(1){ delay(1000); }
  }
  //受信モードをパッシブ、イベント無効に設定
  if(!wifihalow.recvmode_set(PASSIVEMODE,EVENT_DISABLE)){
    INFO_SERIAL.println("Failed to set receive mode.");
    while(1){ delay(1000); }
  }

  //アクセスポイントに接続
  INFO_SERIAL.print("Connecting to Wi-Fi: ");
  INFO_SERIAL.println(SSID);
  if(!wifihalow.wifiConnect(SSID, SEC, PASS, 50000)){
    INFO_SERIAL.println("Connection failed.");
    while(1){ delay(1000); }
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
    while(1){ delay(1000); }
  }
}

void loop() {
  //uart1から文字列を受信して、CRLFが来たら送信
  while(INFO_SERIAL.available() > 0){
    char c = (char)INFO_SERIAL.read();
    sendStr += c;
    if(sendStr.endsWith("\r\n")){
        sendStr.remove(sendStr.length() - 2);
        if(sendStr.length() > 0){
          if(sendStr == "AT*"){
            INFO_SERIAL.println("Restarting Arduino.");
            delay(100);
            NVIC_SystemReset();
          }else{
            if(wifihalow.UDP_Send(udpid,REMOTE_IP,REMOTE_PORT, sendStr )){
            INFO_SERIAL.println(String("SEND:") + sendStr);
          }else{
            INFO_SERIAL.println("UDP send failed.");
          }
        }
      }
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
