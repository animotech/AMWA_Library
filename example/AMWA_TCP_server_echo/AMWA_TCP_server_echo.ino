#include <AMWA_LIB.h>
//シリアル設定
#define AT_SERIAL  Serial1
#define INFO_SERIAL Serial
//IPアドレス設定
#define AMWA_IPADDR "192.168.1.24"
#define SUBNET "225.255.255.0"
#define GATEWAY "192.168.1.1"
//アクセスポイント設定
#define SSID "cs_11ah_ap"
#define SEC "sae"
#define PASS "12345678"
//TCP server設定
#define LOCAL_PORT 4099

AMWA wifihalow(false,&AT_SERIAL,&INFO_SERIAL);

int tcpsid;
int connectedClientId = -1;

void setup() {
  string restr;
  bool init_finish=false;
  INFO_SERIAL.begin(115200);
  AT_SERIAL.begin(115200);
  delay(1000);
  INFO_SERIAL.println("AMWA TCP Server DEMO Start");
  while(!init_finish){
    
    INFO_SERIAL.println("init start");
    wifihalow.AMWA_init();
    delay(1000);
    //DHCPをON
    if(!wifihalow.dhcp_on(DHCP_ENABLE)){
      INFO_SERIAL.println("dhcp enalbe failed");
      continue;
    }
    //IPアドレスを設定
    // if(!wifihalow.ipaddr_set( AMWA_IPADDR, SUBNET,GATEWAY)){
    //   INFO_SERIAL.println("ipaddr set failed");
    //   continue;
    // }
    //受信モードをパッシブ、イベント無効に設定
    if(!wifihalow.recvmode_set(PASSIVEMODE,EVENT_ENABLE)){
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
    
    //TCP serverオープン
    INFO_SERIAL.print("tcp server open, port:");
    INFO_SERIAL.println(LOCAL_PORT);
    int tcpsid = wifihalow.TCP_Server_Open(LOCAL_PORT);
    if(tcpsid >= 0){
      INFO_SERIAL.print("open succsess, id = ");
      INFO_SERIAL.println(tcpsid);
    }else{
      INFO_SERIAL.println("open failed");
      continue;
    }
    //初期化終了
    init_finish = true;
  }
}

void loop() {

  if (connectedClientId == -1)
  {
    //tcp client 接続確認
    AMWA::WaitResult res = wifihalow.waitResponce("+SEVENT:CONNECT,", 1000, STARTWITH);
    if (res.result)
    {
      // OKだった場合、idを取得
      String idstr = res.restr.substring(16);
      connectedClientId = idstr.toInt();
      INFO_SERIAL.println("client connection detected. ready to echo.");
    }
  }
  else
  {
    // TCP受信チェック
    int rlen = wifihalow.available(connectedClientId);
    if (rlen > 0)
    {
      // 受信があったらデータ取得
      String rcvStr = wifihalow.passive_recv(connectedClientId, rlen);
      INFO_SERIAL.print("tcp rcv: ");
      INFO_SERIAL.println(rcvStr);
      // 受け取ったデータをTCPで送信
      INFO_SERIAL.println("tcp send");
      if (wifihalow.TCP_Send(connectedClientId, rcvStr))
      {
        INFO_SERIAL.println("send succsess");
      }
      else
      {
        INFO_SERIAL.println("send fail");
      }
    }
    else if (rlen == -1)
    {
      connectedClientId = -1;
      INFO_SERIAL.println("client disconnected. wait for client connection.");
    }
    delay(100);
  }
}
