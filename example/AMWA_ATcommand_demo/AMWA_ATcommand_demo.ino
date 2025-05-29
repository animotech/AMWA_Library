#include <AMWA_LIB.h>
// シリアル設定
#define AT_SERIAL Serial1
#define INFO_SERIAL Serial

// IPアドレス設定
static String LOCAL_IP = "192.168.1.24";
static String SUBNET = "225.255.255.0";
static String GATEWAY = "192.168.1.1";
// アクセスポイント設定
static String SSID = "cs_11ah_ap";
static String SEC = "sae";
static String PASS = "12345678";
// TCP server設定
static int LOCAL_PORT = 4100;
static String REMOTE_IP = "192.168.1.13";
static int REMOTE_PORT = 4105;

AMWA wifihalow(false, &AT_SERIAL, &INFO_SERIAL);
int udpid;
int connectedClientId = -1;

void setup()
{
  bool init_finish = false;

  INFO_SERIAL.begin(115200);
  AT_SERIAL.begin(115200);

  delay(1000);

  INFO_SERIAL.println("AMWA ATcommand DEMO Start");
  while (!init_finish)
  {
    INFO_SERIAL.println("initialize start.");
    wifihalow.AMWA_init();
    delay(1000);

    // DHCPをON
    INFO_SERIAL.println("set DHCP mode : DHCP mode=" + String(DHCP_ENABLE));
    wifihalow.AT_Send("+WDHCP=", String(DHCP_ENABLE));
    AMWA::WaitResult res = wifihalow.waitResponce("OK", 1000, ALLMATCH);
    if (!res.result)
    {
      INFO_SERIAL.println("failed. retry initialize.");
      continue;
    }
    INFO_SERIAL.println("succeed.");

    INFO_SERIAL.println("set ip address : {LocalIP,Subnet,Gateway}={" + LOCAL_IP + "," + SUBNET + "," + GATEWAY + "}");
    // IPアドレスを設定
    wifihalow.AT_Send("+WIPADDR=", LOCAL_IP + "," + SUBNET + "," + GATEWAY);
    res = wifihalow.waitResponce("OK", 1000, ALLMATCH);
    if (!res.result)
    {
      INFO_SERIAL.println("failed. retry initialize.");
      continue;
    }
    INFO_SERIAL.println("succeed.");

    INFO_SERIAL.println("set receive mode : {ReceiveMode,EventFlag}={" + String(PASSIVEMODE) + "," + String(EVENT_ENABLE) + "}");
    // 受信モードをパッシブ、イベント有効に設定
    wifihalow.AT_Send("+SRECVMODE=", String(PASSIVEMODE) + "," + String(EVENT_ENABLE));
    res = wifihalow.waitResponce("OK", 1000, ALLMATCH);
    if (!res.result)
    {
      INFO_SERIAL.println("failed. retry initialize.");
      continue;
    }
    INFO_SERIAL.println("succeed.");


    // アクセスポイント接続
    INFO_SERIAL.println("connect to access point : {SSID,security,passphrase}={" + SSID + "," + SEC + "," + PASS + "}");
    wifihalow.AT_Send("+WCONN=", SSID + "," + SEC + "," + PASS);
    res = wifihalow.waitResponce("+WEVENT:LINK_UP", 50000, STARTWITH);
    if (!res.result)
    {
      INFO_SERIAL.println("failed. retry initialize.");
      continue;
    }
    INFO_SERIAL.println("succeed.");

    //UDPオープン
    INFO_SERIAL.println("open udp socket : LocalPort=" + String(LOCAL_PORT));
    wifihalow.AT_Send("+SOPEN=", "udp,"+String(LOCAL_PORT));
    res = wifihalow.waitResponce("+SOPEN:", 1000, STARTWITH);
    if (!res.result)
    {
      INFO_SERIAL.println("failed. retry initialize.");
      continue;
    }
    // OKだった場合、socket idを取得
    udpid = res.restr.substring(7).toInt();
    INFO_SERIAL.println("succeed. socket id:"+String(udpid));

    INFO_SERIAL.println("all initialize completed. enter loop.");
    
    //初期化終了
    init_finish = true;
  }
}

void loop() {

  int receivedBytes=0;
  //UDP受信チェック
  wifihalow.AT_Send("+SRECV?","");
  AMWA::WaitResult res = wifihalow.waitResponce("+SRECV:"+String(udpid), 1000, STARTWITH);
  if (res.result)
  {
    receivedBytes = res.restr.substring(9).toInt();
  }

  if(receivedBytes>0)
  {
    INFO_SERIAL.println("udp received bytes : "+String(receivedBytes));

    //UDPデータ取得
    wifihalow.AT_Send("+SRECV=",String(udpid)+","+String(receivedBytes));
    res = wifihalow.waitResponce("+RXD:" + String(udpid), 1000, STARTWITH);
    res = wifihalow.waitResponce("", 1000, STARTWITH);
    String receivedString = res.restr;

    INFO_SERIAL.println("udp received string : " +receivedString);

    //UDPデータ送信
    wifihalow.AT_Send("+SSEND=", String(udpid) + "," + REMOTE_IP + "," + String(REMOTE_PORT) + "," + String(receivedBytes) + ",0");
    res = wifihalow.waitResponce("OK", 1000);
    if (res.result)
    {
      wifihalow.at_serial->write(receivedString.c_str());
      wifihalow.at_serial->flush();
    }
  }

  delay(100);
}
