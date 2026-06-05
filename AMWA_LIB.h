
#ifndef AMWA_LIB_H
#define AMWA_LIB_H
#include <Arduino.h>
#include<string>

//waitResponse
#define  ALLMATCH 0
#define  STARTWITH 1
#define  ENDWITH 2

//RECVMODE
#define  ACTIVEMODE 0
#define  PASSIVEMODE 1
#define  EVENT_DISABLE 0
#define  EVENT_ENABLE 1

//WDHCP
#define  DHCP_DISABLE 0
#define  DHCP_ENABLE 1

class AMWA
{
  public:
  typedef  struct waitresult {
    bool result;
    String restr;
  }WaitResult;

  enum BootState {
    BOOT_AT_MODE,    // 通常 AT モードで起動した（AutoUDP 未設定）
    BOOT_AUTOUDP,    // AutoUDP モードで起動した（設定済み）
    BOOT_TIMEOUT     // 応答なし
  };

  bool logon;
  Stream* at_serial;
  Stream* log_serial;
  AMWA(bool on, Stream *amwa_serial,Stream *arduino_serial);
  void AT_Send(String atcmd,String para);
  bool ipaddr_set(String  ipaddr,String  subnet, String  gateway);
  bool dhcp_on(int enable);
  void AMWA_init( void );
  WaitResult waitResponse(String res, int timeout_ms ,int mode=ALLMATCH);
  bool wifiConnect(String ssid, String security , String pass, int timeout_ms);

  int UDP_Open(uint16_t port);
  bool UDP_Send(int id, String ipaddr,uint16_t port,String  sendstr);

  int TCP_Server_Open(uint16_t port);
  int TCP_Client_Open(String ipaddr, uint16_t port);
  bool TCP_Send(int id, String  sendstr);

  bool Socket_Close(uint16_t id);
  bool socket_exists(int id);
  int available(int id);
  String passive_recv(int id,int len);
  bool recvmode_set(int mode,int event);

  // ---- モード設定 ----
  bool mode_set(String mode);                                                     // AT+WMODE=AP|STA（次回起動モード）

  // ---- AP モード設定 ----
  bool ap_config_set(String ssid, String security, String password, uint16_t channel); // AT+WAPCFG（open 時 password 省略）
  bool ap_ip_set(String ipaddr, String netmask, String gateway);                  // AT+WAPIP=ip,netmask,gw
  int apsta_get(String *mac_list, int max_count);                                 // AT+WAPSTA? (-1: error, 0: none, 1+: connected STA count)

  // ---- STA モード設定 ----
  bool sta_ap_set(String ssid, String security, String password);                 // AT+WAP（接続せず credentials のみ保存。open 時 password 省略）

  // ---- 設定保存・再起動 ----
  bool settings_save();                                                           // AT+WSAVE
  void reboot();                                                                  // ATZ（応答待ちなし、チップリセット）

  // ---- AutoUDP 制御 ----
  bool auto_udp_set(uint16_t local_port, String remote_ip, uint16_t remote_port); // AT+SAUDP=1,...（port は 1..65535）
  bool auto_udp_disable();                                                        // AT+SAUDP=0
  bool auto_udp_escape(unsigned long timeout_ms);                                 // AutoUDP モード起動直後に AT* で抜ける

  bool baudrate_setting_set(int baudrate);                                        // AT+UARTW=<baud>
  int baudrate_setting_get(void);                                                 // AT+UARTW?

  // ---- AutoUDP 起動シーケンス ----
  BootState detect_boot_state(unsigned long timeout_ms);                          // 起動直後に AT/AutoUDP を判別
  bool wait_autoudp_started(unsigned long timeout_ms);                            // "+SOPEN:" で成功 / "exit" or "ERROR:" で失敗 / "start" や "+WEVENT:*" は通過

  // ---- AT専用 helper ----
  void at_receive_begin();                                                        // AT受信側内部状態を初期化
  void at_receive_poll();                                                         // AT受信をFIFOへ取り込む
  size_t at_output_block(Stream &out, size_t maxChunk = 256);                    // CR終端ブロックを指定出力へ送る
  bool at_receive_waiting_response();                                             // 応答待ち中か判定
  void at_send_byte(uint8_t b);                                                   // ATへ1byte送信
  size_t at_send_bytes(const uint8_t *data, size_t len);                          // ATへ複数byte送信

  // ---- LOG専用 helper ----
  void log_receive_line_begin(size_t reserveLen = 128);                           // LOG入力1行バッファ初期化
  bool log_receive_line(String &outLine, size_t maxLen = 128, int budget = 64);  // LOG入力をCRLFで1行取得

  String log_line_buf;

};



#endif //AMWA_LIB_H
