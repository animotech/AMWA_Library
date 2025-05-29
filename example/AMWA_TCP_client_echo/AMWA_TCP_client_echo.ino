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
//TCP client設定
#define REMOTE_IP "192.168.1.13"
#define REMOTE_PORT 4099

AMWA wifihalow(false,&AT_SERIAL,&INFO_SERIAL);

int tcpcid;
int tcpConnectedid = -1;

void setup() {
  string restr;
  bool init_finish=false;
  INFO_SERIAL.begin(115200);
  AT_SERIAL.begin(115200);
  delay(1000);
  INFO_SERIAL.println("AMWA TCP Client DEMO Start");
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
    
    //初期化終了
    init_finish = true;
  }
}

void loop() {

  if(tcpConnectedid==-1)
  {
    //TCP clientオープン
    INFO_SERIAL.print("tcp clinet open, remote addr/port : ");
    INFO_SERIAL.print(REMOTE_IP);
    INFO_SERIAL.print("/");
    INFO_SERIAL.println(REMOTE_PORT);
    int tcpcid = wifihalow.TCP_Client_Open(REMOTE_IP,REMOTE_PORT);
    if(tcpcid >= 0){
      INFO_SERIAL.print("open succsess, id = ");
      INFO_SERIAL.println(tcpcid);
      tcpConnectedid=tcpcid;
    }else{
      INFO_SERIAL.println("open failed");
      tcpConnectedid=-1;
    }
  }
  else
  {
    // TCP受信チェック
    int rlen = wifihalow.available(tcpcid);
      if (rlen > 0)
      {
        // 受信があったらデータ取得
        String rcvStr = wifihalow.passive_recv(tcpcid, rlen);
        INFO_SERIAL.print("tcp rcv: ");
        INFO_SERIAL.println(rcvStr);
        // 受け取ったデータをTCPで送信
        INFO_SERIAL.println("tcp send");
        if (wifihalow.TCP_Send(tcpcid, rcvStr))
        {
          INFO_SERIAL.println("send succsess");
        }
        else
        {
          INFO_SERIAL.println("send fail");
        }
      }
      else if(rlen == -1)
      {
        tcpConnectedid=-1;
      }
  }

  delay(100);
}
