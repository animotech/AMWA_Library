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
#define CHANNEL 13
//UDP設定
#define LOCAL_PORT 4105
#define REMOTE_IP "192.168.11.12"
#define REMOTE_PORT 4098

AMWA wifihalow(false,&AT_SERIAL,&INFO_SERIAL);

int udpid = -1;
String sendStr = "";

void setup() {
  INFO_SERIAL.begin(115200);
  AT_SERIAL.begin(115200);
  delay(1000);
  INFO_SERIAL.println("AMWA UDP AP DEMO Start");

  INFO_SERIAL.println("Initialization started.");
  wifihalow.AMWA_init();
  delay(1000);

  /*APとして再起動する
  AT+WMODE=AP
  AT+WAPCFG=SSID,SEC(,PASS),CHANNEL
  AT+WAPIP=LOCAL_IP,SUBNET,GATEWAY
  AT+WSAVE
  ATZ
  +WEVENT:APSTART_SUCCESSを待つ
  */
  if(!wifihalow.mode_set("AP")){
    INFO_SERIAL.println("Failed to switch to AP mode.");
    while(1){ delay(1000); }
  }
  if(!wifihalow.ap_config_set(SSID, SEC, PASS, CHANNEL)){
    INFO_SERIAL.println("Failed to set AP config.");
    while(1){ delay(1000); }
  }
  if(!wifihalow.ap_ip_set(AMWA_IPADDR, SUBNET, GATEWAY)){
    INFO_SERIAL.println("Failed to set AP IP address.");
    while(1){ delay(1000); }
  }
  if(!wifihalow.settings_save()){
    INFO_SERIAL.println("Failed to save AP mode.");
    while(1){ delay(1000); }
  }
  INFO_SERIAL.println("Rebooting AMWA-01.");
  wifihalow.reboot();

  AMWA::WaitResult res = wifihalow.waitResponce("+WEVENT:APSTART_SUCCESS",10000,STARTWITH);
  if(!res.result){
    INFO_SERIAL.println("Failed to start AP mode.");
    while(1){ delay(1000); }
  }
  INFO_SERIAL.println("AP mode started.");

  //受信モードをパッシブ、イベント無効に設定
  if(!wifihalow.recvmode_set(PASSIVEMODE,EVENT_DISABLE)){
    INFO_SERIAL.println("Failed to set receive mode.");
    while(1){ delay(1000); }
  }

  /*staからの接続を待つ
    +WEVENT:STA_CONNECTEDを待つ
  */
  res = wifihalow.waitResponce("+WEVENT:STA_CONNECTED",10000,STARTWITH);
  if(!res.result){
    INFO_SERIAL.println("STA connection timeout.");
    while(1){ delay(1000); }
  }
  
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
