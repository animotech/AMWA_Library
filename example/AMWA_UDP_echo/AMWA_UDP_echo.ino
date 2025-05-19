#include <AMWA_LIB.h>
//シリアル設定
#define AT_SERIAL  Serial1
#define INFO_SERIAL Serial
//IPアドレス設定
#define AMWA_IPADDR "192.168.11.12"
#define SUBNET "225.255.255.0"
#define GATEWAY "192.168.11.1"
//アクセスポイント設定
#define SSID "MegaChips"
#define SEC "sae"
#define PASS "12345678"
//UDP設定
#define LOCAL_PORT 4098
#define REMOTE_IP "192.168.11.17"
#define REMOTE_PORT 4105

AMWA wifihalow(false,&AT_SERIAL,&INFO_SERIAL);

int udpid;

void setup() {
  string restr;
  bool init_finish=false;;
  INFO_SERIAL.begin(115200);
  AT_SERIAL.begin(115200);
  delay(1000);
  INFO_SERIAL.println("AMWA UDP DEMO Start");
  while(!init_finish){
    
    INFO_SERIAL.println("init start");
    wifihalow.AMWA_init();
    delay(1000);
    //DHCPをOFF
    if(!wifihalow.dhcp_on(DHCP_DISABLE)){
      INFO_SERIAL.println("dhcp disalbe failed");
      continue;
    }
    //IPアドレスを設定
    if(!wifihalow.ipaddr_set( AMWA_IPADDR, SUBNET,GATEWAY)){
      INFO_SERIAL.println("ipaddr set failed");
      continue;
    }
    //受信モードをパッシブ、イベント無効に設定
    if(!wifihalow.recvmode_set(PASSIVEMODE,EVENT_DISABLE)){
      INFO_SERIAL.println("recvmode_set failed");
      continue;
    }

    //アクセスポイント接続
    INFO_SERIAL.print("wi-fi connect to ");
    INFO_SERIAL.println(SSID);
    if(wifihalow.wifiConnect(SSID,SEC,PASS,50000)){
      INFO_SERIAL.println("connect success");
    }else{      
      INFO_SERIAL.println("connect failed");
      continue;
    }
    
    //UDPオープン
    INFO_SERIAL.print("udp open, port:");
    INFO_SERIAL.println(LOCAL_PORT);
    int udpid = wifihalow.UDP_Open(LOCAL_PORT);
    if(udpid >= 0){
      INFO_SERIAL.print("open succsess, id = ");
      INFO_SERIAL.println(udpid);
    }else{
      INFO_SERIAL.println("open failed");
      continue;
    }
    //初期化終了
    init_finish = true;
  }
}

void loop() {

  //UDP受信チェック
  int rlen = wifihalow.available(udpid);
  if(rlen > 0){
    //受信があったらデータ取得
    String rcvStr = wifihalow.passive_recv(udpid,rlen);
    INFO_SERIAL.print("udp rcv: ");
    INFO_SERIAL.println(rcvStr);
    //受け取ったデータをUDPで送信
    INFO_SERIAL.println("udp send");
    if(wifihalow.UDP_Send(udpid,REMOTE_IP,REMOTE_PORT, rcvStr )){
      INFO_SERIAL.println("send succsess");
    }else{
      INFO_SERIAL.println("send fail");
    }
  }
  delay(100);
}
