
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

/// @brief AMWA-01 をハードウェアリセットする
/// @param
/// @note reset 後の chip は、AT+UARTW で不揮発保存されている baudrate で boot する。
///       そのため host 側の AT_SERIAL は、reset 前からその保存 baudrate に合わせておく必要がある。
///       AMWA_init() 自体は baudrate を変更せず、reset pin のトグルだけを行う。
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
  //UART受信のバッファを読み込んでクリア
  while (at_serial->available())at_serial->read();
  //送信
  at_serial->print(sendstr);
  at_serial->flush();
  //送信後、少し待つ（送信中に受信読み込みをすると失敗する場合があるため）
  //delay(10);
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
/// @param mode       //ALLMATCH:全て一致、STARTWITH:前方一致、ENDWITH:後方一致
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
      // 注: ここで per-byte に log_serial->write をすると、UNO R4 では USB CDC
      // 呼び出しの per-call オーバーヘッドで UART RX ISR レイテンシが
      // 87us (115200 baud) を超える瞬間があり、HW UART OVERRUN で
      // 文字落ちすることがある。logon の echo は行末で chunk 出力する。
      //改行文字が来たら解析
      if( rcvbyte[cnt] == TERM){
        // logon=true なら、ここで 1 行ぶんをまとめて echo する
        if(logon){
          log_serial->write((const uint8_t*)rcvbyte, cnt + 1);
        }
        //文字列にするため改行文字をNULLに置き換え
        rcvbyte[cnt] = '\0';
        //文字列に変換
        String rcvstr = String(rcvbyte);
        // 比較用に trim したコピー (前行 CRLF の LF 残留対策)
        String trimmed = rcvstr;
        trimmed.trim();

        // 戻り値の文字列：
        //   - res が空文字列 (passive_recv の payload 取得) のときは raw を返す
        //     （payload を改変しない。先頭/末尾の空白も payload の一部かもしれない）
        //   - それ以外 (AT 応答) は trimmed を返す
        //     （substring(7) で +SOPEN: の id を取り出す等、ホスト側の位置計算が
        //      LF 残留でズレるのを防ぐ）
        String returnStr = (res.length() == 0) ? rcvstr : trimmed;
        switch (mode)
        {
        case ALLMATCH:
        //文字列が全て一致するかをチェック
          if(trimmed == res){
            return {true, returnStr};
          }
          break;
        case STARTWITH:
         //文字列が前方一致するかをチェック
          if(trimmed.startsWith(res)){
            return {true, returnStr};
          }
          break;
        case ENDWITH:
         //文字列が後方一致するかをチェック
          if(trimmed.endsWith(res)){
            return {true, returnStr};
          }
          break;
        default:
          break;
        }
        //エラーが返って来ていたらNGとしてループを抜ける。
        if(trimmed.startsWith("ERROR:")){
          return {false, trimmed};   // エラー文字列は trimmed を返す
        }else{
          cnt = 0;
        }
      }else{
        cnt++;
        //BUFFサイズを超えたら0にする
        if(cnt >= RESBUFSIZE){
          // バッファ満杯時は溜まっているぶんを echo してから wrap
          if(logon){
            log_serial->write((const uint8_t*)rcvbyte, RESBUFSIZE);
          }
          cnt = 0;
        }
      }
    }
  } while (( millis() - startMillis ) < timeout_ms);
  //タイムアウトで抜けたらNGとして戻る
  return {false, "TIMEOUT"};
}

/// @brief AP 接続コマンド
/// @param ssid     SSID または BSSID
/// @param security "sae" / "owe" / "open"
/// @param pass     パスワード（sae 時のみ使用、open/owe 時は無視）
/// @param timeout_ms LINK_UP 待ちタイムアウト
/// @return LINK_UP 取得時 true
bool AMWA::wifiConnect(String ssid, String security , String pass, int timeout_ms){
  // firmware の WCONN は open/owe では 2 引数、sae では 3 引数を要求する
  String para;
  if (security == "sae") {
    para = ssid + "," + security + "," + pass;
  } else {
    para = ssid + "," + security;
  }
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
  WaitResult res= waitResponce("+SOPEN:",3000,STARTWITH);
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
    return true;
  }else{
    return false;
  }
}

/// @brief TCP server オープンコマンド
/// @param port AMWA側のLOCALポート
/// @return OK時はid、NG時は-1
int AMWA::TCP_Server_Open(uint16_t port){
  int id = 0;
  String para = "tcp," + String(port) + ",1";
  AT_Send("+SOPEN=",para);
  WaitResult res= waitResponce("+SOPEN:",3000,STARTWITH);
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

/// @brief TCP client オープンコマンド
/// @param ipaddr, port remote ipaddr, remote port
/// @return OK時はid、NG時は-1
int AMWA::TCP_Client_Open(String ipaddr, uint16_t port){
  int id = 0;
  String idstr = "";
  String para = "tcp," + ipaddr +","+ String(port) + ",1";
  AT_Send("+SOPEN=",para);
  WaitResult res= waitResponce("+SOPEN:",3000,STARTWITH);
  if(res.result){
    //OKだった場合、idを取得
    idstr = res.restr.substring(7);
    id = idstr.toInt();
  }else{
    //NGだった場合
    id = -1;
    return id;
  }
  res= waitResponce("+SEVENT:CONNECT,"+idstr,2000,STARTWITH);
  if(res.result){
    ;
  }else{
    //時間内に接続できなかったのでsocket開放しておく
    Socket_Close(id);
    id = -1;
  }
  return id;
}

bool AMWA::TCP_Send(int id, String  sendstr){
  int len = sendstr.length();
  String para = String(id) + "," +  String(len);
  AT_Send("+SSEND=",para);
  WaitResult  res= waitResponce("OK",1000);
  if(res.result){
    at_serial->write(sendstr.c_str());
    at_serial->flush();
    return true;
  }else{
    return false;
  }
}

/// @brief socket close コマンド
/// @param id socket id
/// @return OK時はtrue、NG時はfalse
bool AMWA::Socket_Close(uint16_t id){
  AT_Send("+SCLOSE=",String(id));
  WaitResult res= waitResponce("+SCLOSE:"+String(id),1000,STARTWITH);
  if(res.result){
    //OKだった場合
    return true;
  }else{
    //NGだった場合
    return false;
  }
}

/// @brief ソケット受信バイト取得 SRECV?コマンド
/// @param id socket id
/// @return 0以上:受信バイト数 -1:指定ソケットなし/応答タイムアウト
int AMWA::available(int id){
  AT_Send("+SRECV?","");
  WaitResult res= waitResponce("+SRECV:" +String(id),1000,STARTWITH);
  if (res.result==false)
  {
    return -1;
  }
  String numstr= res.restr.substring(9);
  return numstr.toInt();
}

/// @brief socketリスト取得 SLIST?コマンド
/// @param id socket id
/// @return 0以上:指定ソケットあり -1:指定ソケットなし
bool AMWA::socket_exists(int id){
  AT_Send("+SLIST?","");
  return waitResponce("+SLIST:" + String(id), 3000, STARTWITH).result;
}

String AMWA::passive_recv(int id,int len){
  String para = String(id) + "," + String(len);
  AT_Send("+SRECV=",para);
  WaitResult res= waitResponce("+RXD:" +String(id),1000,STARTWITH);
  // ヘッダ受信に失敗したらタイムアウトを payload と勘違いさせない
  if(!res.result){
    return String("");
  }
  res= waitResponce("",1000,STARTWITH);
  if(!res.result){
    return String("");
  }
  return res.restr;
}
//AT+SRECVMODE=<mode>[,<event>][CR]

bool AMWA::recvmode_set(int mode,int event){
  String para = String(mode) + "," + String(event);
  AT_Send("+SRECVMODE=",para);
  return waitResponce("OK" ,1000).result;
}

/// @brief 次回起動モード設定
/// @param mode "AP" or "STA"
/// @return 正常終了時 true
/// @note 即時切替ではなく configstore への予約。settings_save() + reboot() で確定する。
bool AMWA::mode_set(String mode){
  AT_Send("+WMODE=", mode);
  return waitResponce("OK", 1000).result;
}

/// @brief AP モード用アクセスポイント設定
/// @param ssid     SSID
/// @param security "sae" or "open"（owe は firmware 側で拒否されるため弾く）
/// @param password パスワード（sae 時のみ使用、open 時は無視）
/// @param channel  S1G チャンネル番号（0 を指定すると country list 先頭チャンネル）
/// @return 正常終了時 true。security が "sae"/"open" 以外の場合は AT 送信せず false
bool AMWA::ap_config_set(String ssid, String security, String password, uint16_t channel){
  // firmware の WAPCFG は open では 3 引数、sae では 4 引数を要求する
  String para;
  if(security == "sae"){
    para = ssid + "," + security + "," + password + "," + String(channel);
  }else if(security == "open"){
    para = ssid + "," + security + "," + String(channel);
  }else{
    // owe や typo を弾く
    return false;
  }
  AT_Send("+WAPCFG=", para);
  return waitResponce("OK", 1000).result;
}

/// @brief AP モード用 IP アドレス設定
/// @param ipaddr  AP 自身の IP アドレス
/// @param netmask サブネットマスク
/// @param gateway ゲートウェイ
/// @return 正常終了時 true
bool AMWA::ap_ip_set(String ipaddr, String netmask, String gateway){
  String para = ipaddr + "," + netmask + "," + gateway;
  AT_Send("+WAPIP=", para);
  return waitResponce("OK", 1000).result;
}

int AMWA::apsta_get(String *mac_list, int max_count){
  int mac_count = 0;

  if(max_count <= 0){
    return -1;
  }

  AT_Send("+WAPSTA?", "");
  WaitResult res = waitResponce("+WAPSTA:", 1000, STARTWITH);
  if(!res.result){
    return -1;
  }

  if(res.restr == "+WAPSTA:NONE"){
    return 0;
  }

  while(res.result && res.restr.startsWith("+WAPSTA:")){
    if(mac_count < max_count){
      mac_list[mac_count] = res.restr.substring(8);
      mac_count++;
    }

    res = waitResponce("+WAPSTA:", 200, STARTWITH);
    if(!res.result){
      break;
    }
    if(res.restr == "+WAPSTA:NONE"){
      break;
    }
  }

  return mac_count;
  }

/// @brief STA 接続先 AP 設定（接続せず credentials のみ保存）
/// @param ssid     SSID または BSSID
/// @param security "sae" / "owe" / "open"
/// @param password パスワード（sae 時のみ使用、open/owe 時は無視）
/// @return 正常終了時 true。security が "sae"/"owe"/"open" 以外の場合は AT 送信せず false
bool AMWA::sta_ap_set(String ssid, String security, String password){
  // firmware の WAP は open/owe では 2 引数、sae では 3 引数を要求する
  String para;
  if(security == "sae"){
    para = ssid + "," + security + "," + password;
  }else if(security == "open" || security == "owe"){
    para = ssid + "," + security;
  }else{
    // typo を弾く
    return false;
  }
  AT_Send("+WAP=", para);
  return waitResponce("OK", 1000).result;
}

/// @brief 設定を不揮発メモリに保存
/// @return 正常終了時 true
/// @note STA/AP 両方の設定と次回起動モードを一括保存する
bool AMWA::settings_save(){
  AT_Send("+WSAVE", "");
  return waitResponce("OK", 1000).result;
}

/// @brief AMWA-01 を再起動（ATZ）
/// @note ATZ は応答せず即チップリセットされる。応答待ちはしない。
void AMWA::reboot(){
  AT_Send("Z", "");
}

/// @brief AutoUDP 設定（常に有効化）
/// @param local_port  ローカルポート (1..65535)
/// @param remote_ip   リモート IP アドレス
/// @param remote_port リモートポート (1..65535)
/// @return 正常終了時 true
bool AMWA::auto_udp_set(uint16_t local_port, String remote_ip, uint16_t remote_port){
  if(local_port == 0 || remote_port == 0){
    return false;
  }
  String para = "1," + String(local_port) + "," + remote_ip + "," + String(remote_port);
  AT_Send("+SAUDP=", para);
  return waitResponce("OK", 1000).result;
}

/// @brief 不揮発メモリに保存する UART baudrate を設定する
/// @return 正常終了時 true
bool AMWA::baudrate_setting_set(int baudrate){
  AT_Send("+UARTW=", String(baudrate));
  return waitResponce("OK", 1000).result;
}

/// @brief 不揮発メモリに保存されている UART baudrate を取得する
/// @return 保存されている baudrate。取得失敗時は -1
int AMWA::baudrate_setting_get(void){
  AT_Send("+UARTW?", "");
  WaitResult res = waitResponce("+UARTW:", 1000, STARTWITH);
  if(!res.result){
    return -1;
  }
  return res.restr.substring(7).toInt();
}

/// @brief AutoUDP 無効化
/// @return 正常終了時 true
bool AMWA::auto_udp_disable(){
  AT_Send("+SAUDP=", "0");
  return waitResponce("OK", 1000).result;
}

/// @brief AutoUDP モード起動直後に AT* で AT モードへ抜ける
/// @param timeout_ms "exit" 応答待ちタイムアウト（firmware の exit 待ちウィンドウは 5 秒固定なので 6000 程度推奨）
/// @return AT モード復帰時 true
/// @note AMWA-01 起動後 5 秒以内（"AutoUDP" 出力後の exit 待ちウィンドウ内）に呼ぶこと
bool AMWA::auto_udp_escape(unsigned long timeout_ms){
  AT_Send("*", "");
  return waitResponce("exit", timeout_ms, STARTWITH).result;
}

/// @brief AMWA-01 の起動状態を判別
/// @param timeout_ms "AutoUDP" 出力検出に費やす時間
///        （AP 初回 boot は ap_init() の link up 待ちに最大 30 秒かかるので
///         40000ms 程度推奨。AT モード未設定の場合この時間まるまる待つことになる）
/// @return BOOT_AUTOUDP / BOOT_AT_MODE / BOOT_TIMEOUT
/// @note  FW_VERSION: 等での早期確定は不可能。chip は FW_VERSION 出力後に
///        ap_init/sta_init を実行してから "AutoUDP" を出すため、FW_VERSION
///        ベースの短縮判定は AutoUDP 設定済み時に "AutoUDP" を見逃して
///        誤判定する。timeout_ms 全体を素直に待つしかない。
AMWA::BootState AMWA::detect_boot_state(unsigned long timeout_ms){
  // "AutoUDP" 出力を待つ。検出できれば設定済み
  if(waitResponce("AutoUDP", timeout_ms, STARTWITH).result){
    return BOOT_AUTOUDP;
  }
  // 検出されなければ AT モードかどうか確認するため "AT" を送って "OK" を待つ
  AT_Send("", "");
  if(waitResponce("OK", 1000, ALLMATCH).result){
    return BOOT_AT_MODE;
  }
  return BOOT_TIMEOUT;
}

/// @brief AutoUDP モード突入後 socket オープン完了まで待つ
/// @param timeout_ms 全体タイムアウト
/// @return +SOPEN: or +RXD:<id>,0,... 受信時 true / "exit" or "ERROR:" 受信時 false / タイムアウト時 false
/// @note "start" や "+WEVENT:*" は内部でログ出力のみで継続。基本は +SOPEN: で成功確定。
///       AP 側では peer の zero-length prime packet (+RXD:<id>,0,...) が先に見える場合も ready と扱う。
///       "exit"（AT* 等で AutoUDP を抜けた場合）や "ERROR:" は失敗として早期 return する。
bool AMWA::wait_autoudp_started(unsigned long timeout_ms){
  char rcvbyte[RESBUFSIZE];
  int cnt = 0;
  uint32_t startMillis = millis();

  do {
    if(at_serial->available() > 0){
      char b = (char)at_serial->read();
      rcvbyte[cnt] = b;
      // logon の echo は行末で chunk 出力する。per-byte だと UNO R4 で
      // UART RX ISR レイテンシが 87us (115200 baud) deadline を超えて
      // HW OVERRUN を起こすため。

      if(b == TERM){
        // String 化のため TERM を '\0' で上書きするが、ログ echo は TERM を含めて
        // 出したいので、書き戻すために行長を保持しておく。
        int line_len = cnt + 1;   // TERM を含む長さ
        rcvbyte[cnt] = '\0';
        String rcvstr = String(rcvbyte);
        // 行頭/行末の空白・改行コード（将来 \r\n 混在時の \n 残留など）を除去
        rcvstr.trim();

        // 通常行: logon なら 1 行ぶんをまとめて echo してから判定
        if(logon){
          rcvbyte[cnt] = TERM;   // 書き戻して echo (NUL ではなく TERM を出すため)
          log_serial->write((const uint8_t*)rcvbyte, line_len);
        }

        // 成功: socket オープン完了
        if(rcvstr.startsWith("+SOPEN:")){
          return true;
        }
        // AP 側では +SOPEN: が見えないまま、STA の zero-length prime packet
        // (+RXD:<id>,0,...) が先に届くことがある。この時点で socket は
        // 受信可能なので ready とみなす。payload 付き +RXD は header だけを
        // 消費してしまうため、fallback ready には使わない。
        if(rcvstr.startsWith("+RXD:")){
          int firstComma = rcvstr.indexOf(',');
          int secondComma = (firstComma >= 0) ? rcvstr.indexOf(',', firstComma + 1) : -1;
          if(firstComma >= 0 && secondComma > firstComma + 1){
            bool len_valid = true;
            for(int i = firstComma + 1; i < secondComma; i++){
              char c = rcvstr.charAt(i);
              if(c < '0' || c > '9'){
                len_valid = false;
                break;
              }
            }
            if(len_valid && rcvstr.substring(firstComma + 1, secondComma).toInt() == 0){
              return true;
            }
          }
        }
        // 失敗: AutoUDP を抜けた / エラー
        if(rcvstr.startsWith("exit") || rcvstr.startsWith("ERROR:")){
          return false;
        }
        // start や +WEVENT:CONNECT_SUCCESS / LINK_UP / APSTART_SUCCESS などはログ扱いで継続
        cnt = 0;
      }else{
        cnt++;
        if(cnt >= RESBUFSIZE){
          // バッファ満杯時は溜まっているぶんを echo してから wrap
          if(logon){
            log_serial->write((const uint8_t*)rcvbyte, RESBUFSIZE);
          }
          cnt = 0;
        }
      }
    }
  } while ((millis() - startMillis) < timeout_ms);

  return false;
}
