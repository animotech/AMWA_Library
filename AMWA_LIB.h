
#ifndef AMWA_LIB_H
#define AMWA_LIB_H
#include <Arduino.h>
#include<string>

//waitResponce
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

// AutoUDP 受信出力形式
#define  AUTOUDP_RX_FORMAT_HEADER 0
#define  AUTOUDP_RX_FORMAT_RAW 1


using namespace std;
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
  void (*baud_switch_cb)(uint32_t new_baud) = nullptr;   // wait_autoudp_started 中の +UART_SWITCH 検出時に呼ばれる
  AMWA(bool on, Stream *amwa_serial,Stream *arduino_serial);
  void AT_Send(String atcmd,String para);
  bool ipaddr_set(String  ipaddr,String  subnet, String  gateway);
  bool dhcp_on(int enable);
  void AMWA_init( void );
  WaitResult waitResponce(String res, int timeout_ms ,int mode=ALLMATCH);
  bool wifiConnect(String ssid, String security , String pass, int timeout_ms);

  int UDP_Open(uint16_t port);
  bool UDP_Send(int id, String ipaddr,uint16_t port,String  sendstr);

  int TCP_Server_Open(uint16_t port);
  int TCP_Client_Open(String ipaddr, uint16_t port);
  bool TCP_Send(int id, String  sendstr);

  bool Socket_Close(uint16_t id);
  int available(int id);
  String passive_recv(int id,int len);
  bool recvmode_set(int mode,int event);

  // ---- モード設定 ----
  bool mode_set(String mode);                                                     // AT+WMODE=AP|STA（次回起動モード）

  // ---- AP モード設定 ----
  bool ap_config_set(String ssid, String security, String password, uint16_t channel); // AT+WAPCFG（open 時 password 省略）
  bool ap_ip_set(String ipaddr, String netmask, String gateway);                  // AT+WAPIP=ip,netmask,gw
  int apsta_get(String *mac_list);                                                // AT+WAPSTA? (connected STA MAC list)

  // ---- STA モード設定 ----
  bool sta_ap_set(String ssid, String security, String password);                 // AT+WAP（接続せず credentials のみ保存。open 時 password 省略）

  // ---- 設定保存・再起動 ----
  bool settings_save();                                                           // AT+WSAVE
  void reboot();                                                                  // ATZ（応答待ちなし、チップリセット）

  // ---- AutoUDP 制御 ----
  bool auto_udp_set(uint16_t local_port, String remote_ip, uint16_t remote_port); // AT+SAUDP=1,...（常に有効化、baud は変更しない）
  bool auto_udp_set(uint16_t local_port, String remote_ip, uint16_t remote_port,
                    uint32_t baud);                                               // AT+SAUDP=1,...,baud（baud も保存）
  bool auto_udp_set(uint16_t local_port, String remote_ip, uint16_t remote_port,
                    uint32_t baud, uint8_t rx_format);                            // AT+SAUDP=1,...,baud,rx_format（受信出力形式も保存）
  bool auto_udp_disable();                                                        // AT+SAUDP=0
  bool auto_udp_escape(unsigned long timeout_ms);                                 // AutoUDP モード起動直後に AT* で抜ける

  // ---- UART baud 自動切替 callback ----
  /// @brief AutoUDP 突入時にチップが "+UART_SWITCH:<baud>\r" を送ってきたとき
  ///        wait_autoudp_started から呼ばれる callback を登録する。
  ///        併せて AMWA_init() の中でも 115200 で呼ばれ、リセット時に
  ///        ホスト側 AT_SERIAL を default に戻すのに使われる。
  /// @note  callback は新しい baud (uint32_t) を受け取り、ホスト側で
  ///        AT_SERIAL.end() / AT_SERIAL.begin(<baud>) を実行してチップに
  ///        追従する責務がある。
  /// @note  callback を **登録しないまま** AutoUDP で baud 切替を使うと、
  ///        chip だけが新 baud になり host は旧 baud のまま残るため通信不能になる。
  ///        AutoUDP で 115200 以外を使う場合は必ず登録すること。
  bool baudrate_setting_set(int baudrate);                                        // AT+UARTW=<baud>
  int baudrate_setting_get(void);                                                 // AT+UARTW?
  typedef void (*BaudSwitchCallback)(uint32_t new_baud);
  void set_baud_switch_callback(BaudSwitchCallback cb);

  // ---- AutoUDP 起動シーケンス ----
  BootState detect_boot_state(unsigned long timeout_ms);                          // 起動直後に AT/AutoUDP を判別
  bool wait_autoudp_started(unsigned long timeout_ms);                            // "+SOPEN:" で成功 / "exit" or "ERROR:" で失敗 / "start" や "+WEVENT:*" は通過

};



#endif //AMWA_LIB_H
