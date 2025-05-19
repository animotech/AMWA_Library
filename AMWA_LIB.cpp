
#include <AMWA_LIB.h>
#define TERM '\r'
#define AMWA_RESET_pin 9
#define  RESBUFSIZE 256

/// @brief コンストラクタ
/// @param on レスポンス待ち中に受信したシリアルデータを表示するかどうか
AMWA::AMWA(bool on, Stream *amwa_serial,Stream *arduino_serial){
  logon = on;
  at_serial = amwa_serial;
  log_serial = arduino_serial;
}

/// @brief 初期化及びリセット
/// @param  
void AMWA::AMWA_init( void ){
  pinMode( AMWA_RESET_pin, OUTPUT );
  digitalWrite( AMWA_RESET_pin, HIGH );
  delay(100);
  digitalWrite( AMWA_RESET_pin, LOW );
}

/// @brief ATコマンド送信
/// @param atcmd 送信するコマンド
/// @param para  送信するパラメータ
void AMWA::AT_Send(String  atcmd,String  para){
  String  sendstr ="AT" + atcmd + para +TERM;
  while (at_serial->available())at_serial->read();
  //送信
  at_serial->print(sendstr);
  at_serial->flush();
  delay(10);
}

/// @brief AMWAのIPアドレスをセット
/// @param ipaddr IPアドレス
/// @param subnet サブネットマスク
/// @param gateway ゲートウェイアドレス
/// @return 正常終了時　true
bool AMWA::ipaddr_set(String  ipaddr,String  subnet, String  gateway){
  String  para =ipaddr + "," + subnet + "," + gateway;
  AT_Send("+WIPADDR=",para);
  return waitResponce("OK" ,1000).result;
}
/// @brief DHCPをONOFF設定
/// @param enable DHCP_DISABLE or DHCP_ENABLE
/// @return 正常終了時　true
bool AMWA::dhcp_on(int enable){
  AT_Send("+WDHCP=",String(enable));
  return waitResponce("OK" ,1000).result;
}

/// @brief 指定文字列待ち受け
/// @param res         待ち受けする文字列
/// @param timeout_ms   タイムアウト[mesec]
/// @param mode       //ALLMATCH:全て一致、STARTWITH:初めだけ一致、ENDWITH:最後だけ一致
/// @return      {結果, 受信した文字列またはTIMEOUT}
AMWA::WaitResult AMWA::waitResponce(String res, int timeout_ms ,int mode){
  char rcvbyte[RESBUFSIZE];
  bool result = false;
  int cnt = 0;
  int incomingByte;
  uint32_t startMillis = millis();
  do {
    //受信が来るまでループ
    if(at_serial->available()>0){
      //データをバッファに格納
      incomingByte= at_serial->read();
      rcvbyte[cnt] = (char)incomingByte;
      //logonがtrue時に受信したデータを表示
      if(logon){
        log_serial->write(rcvbyte[cnt]);
      }
      //改行文字が来たら解析
      if( rcvbyte[cnt] == TERM){
        //文字列にするため改行文字をNULLに置き換え
        rcvbyte[cnt] = '\0';
        //文字列に変換
        String rcvstr = String(rcvbyte);
        //全て
        switch (mode)
        {
        case ALLMATCH:
        //全て一致するかをチェック
          if(rcvstr == res){
            return {true, rcvstr};;
          }
          break;
        case STARTWITH:
         //初めの文字だけが一致するかをチェック 
          if(rcvstr.startsWith(res)){
            return {true, rcvstr};
          }  
          break;
               break;
        case ENDWITH:
         //最後の文字だけが一致するかをチェック 
          if(rcvstr.endsWith(res)){
            return {true, rcvstr};
          }  
          break;
        default:
          break;
        }
        //エラーが返って来ていたらNGとしてループを抜ける。
        if(rcvstr.startsWith("ERROR:")){
          return {false, rcvstr};
        }else{
          cnt = 0;
        }
      }else{
        cnt++;
        //BUFFサイズを超えたら0にする
        if(cnt >= RESBUFSIZE){
          cnt = 0;
        }
      }
    }
  } while (( millis() - startMillis ) < timeout_ms);
  //タイムアウトで抜けたらNGとして戻る
  return {false, "TIMEOUT"};
}

/// @brief 
/// @param ssid 
/// @param security 
/// @param pass 
/// @param timeout_ms 
/// @return 
bool AMWA::wifiConnect(String ssid, String security , String pass, int timeout_ms){
  String para = ssid + "," + security + ","  + pass;
  AT_Send("+WCONN=",para);
  return waitResponce("+WEVENT:LINK_UP",timeout_ms,STARTWITH).result; 
}

/// @brief UDPオープンコマンド
/// @param port AMWA側のLOCALポート
/// @return OK時はid、NG時は-1
int AMWA::UDP_Open(uint16_t port){
  int id = 0;
  String para = "udp," + String(port);
  AT_Send("+SOPEN=",para);
  WaitResult res= waitResponce("+SOPEN:",1000,STARTWITH);
  if(res.result){
    //OKだった場合、idを取得
    String idstr = res.restr.substring(7);
    id = idstr.toInt();
  }else{
    //NGだった場合
    id = -1;
  }
  return id;
}

bool AMWA::UDP_Send(int id, String ipaddr,uint16_t port,String  sendstr){
  int len = sendstr.length();
  String para = String(id) + "," + ipaddr +","+ String(port) +"," +  String(len);
  AT_Send("+SSEND=",para);
  WaitResult  res= waitResponce("OK",1000);
  if(res.result){
    at_serial->write(sendstr.c_str());
    at_serial->flush();
    return "OK";
  }else{
    return res.result;
  }
}

int AMWA::available(int id){
  AT_Send("+SRECV?","");
  WaitResult res= waitResponce("+SRECV:" +String(id),1000,STARTWITH);
  String numstr= res.restr.substring(9); 
  return numstr.toInt();
}

String AMWA::passive_recv(int id,int len){
  String para = String(id) + "," + String(len);
  AT_Send("+SRECV=",para);
  WaitResult res= waitResponce("+RXD:" +String(id),1000,STARTWITH);
  res= waitResponce("",1000,STARTWITH);
  return res.restr;
}
//AT+SRECVMODE=<mode>[,<event>][CR]

bool AMWA::recvmode_set(int mode,int event){
  String para = String(mode) + "," + String(event);
  AT_Send("+SRECVMODE=",para);
  return waitResponce("OK" ,1000).result;
}
