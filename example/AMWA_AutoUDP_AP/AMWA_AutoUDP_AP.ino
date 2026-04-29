/*
 * AMWA_AutoUDP_AP
 *
 * AMWA-01 を AP モードで起動し、AutoUDP 通信を自動開始するサンプルです。
 * STA 側と組み合わせて使用してください（AMWA_AutoUDP_STA を参照）。
 *
 * 【動作の流れ】
 *   1. 起動時に AMWA-01 の状態を判定する
 *      - すでに AutoUDP 設定済みなら → ブリッジモードへ直行
 *      - 未設定なら → AP 設定 + AutoUDP 設定 + 保存 + 再起動 → ブリッジモードへ
 *   2. ブリッジモードでは Serial Monitor と AMWA-01 の UART を双方向に中継
 *      - STA から受信したデータは Serial Monitor に表示
 *      - Serial Monitor に入力したデータは STA へ送信
 *
 * 【設定変更したいとき】
 *   一度 AutoUDP 設定が保存されると、Arduino リセットだけでは設定変更できません。
 *   下記 #define FORCE_RECONFIG を 1 に変更してビルドし直すと、
 *   既存設定を破棄して新しい設定で書き換えます。
 *   設定変更後は FORCE_RECONFIG を 0 に戻してください。
 */

#include <AMWA_LIB.h>

// ---- ユーザー設定 ----

// AP 設定（STA 側と合わせること）
#define AP_SSID      "testap"
#define AP_SEC       "sae"        // "sae" or "open"（owe は不可）
#define AP_PASS      "12345678"   // open 時は無視される
#define AP_CHANNEL   13           // S1G チャンネル番号（0 = country list 先頭の自動選択）

// AP 自身の IP 設定
#define AP_IP        "192.168.1.123"
#define AP_NETMASK   "255.255.255.0"
#define AP_GATEWAY   "192.168.1.1"

// AutoUDP 設定
#define LOCAL_PORT     4099
#define REMOTE_IP      "192.168.1.33"   // STA 側の固定 IP
#define REMOTE_PORT    4099
#define AUTOUDP_BAUD   38400                   // AutoUDP 中の UART baud (115200/57600/38400 等)
                                        // チップに保存され、boot 後に "+UART_SWITCH:" 経由で
                                        // ホスト側 AT_SERIAL も追従する
#define AUTOUDP_SHOW_RXD 0              // 1: +RXD header + payload / 0: payload only
                                        // payload only では packet 境界・送信元 IP/port は見えない。

// 設定を強制的に書き換える場合は 1 にする
#define FORCE_RECONFIG  0

// ---- シリアル設定 ----
#define AT_SERIAL    Serial1
#define INFO_SERIAL  Serial

AMWA wifihalow(true, &AT_SERIAL, &INFO_SERIAL);

// 失敗ステップを統一フォーマットでログ出力するヘルパー
static bool failStep(const char *step) {
  INFO_SERIAL.print("[CONFIG] ");
  INFO_SERIAL.print(step);
  INFO_SERIAL.println(" FAILED.");
  return false;
}

// チップが "+UART_SWITCH:<baud>" を出したときに wait_autoudp_started から呼ばれる callback。
// ホスト側 AT_SERIAL を新 baud に貼り直す。
//
// 注意: チップは "+UART_SWITCH:" 行送出後 50ms で UART を再初期化する。
// その 50ms 窓内に host AT_SERIAL も切替えないと chip 出力が読めなくなる。
// USB CDC への INFO_SERIAL.print は Serial Monitor が遅いと数 ms ブロック
// しうるため、AT_SERIAL の貼り直しを最優先で実行し、ログ出力は切替後に回す。
static void on_baud_switch(uint32_t new_baud) {
  // ★ AT_SERIAL 切替を最優先
  AT_SERIAL.end();
  AT_SERIAL.begin(new_baud);
  delay(20);
  while (AT_SERIAL.available()) AT_SERIAL.read();
  // 切替完了後にログ
  INFO_SERIAL.print("[BAUD] Switched AT_SERIAL to ");
  INFO_SERIAL.println(new_baud);
}

// chip リセット前に host AT_SERIAL を default 115200 に戻すヘルパー。
// chip は default 115200 で boot するので、そこに合わせ直す。
// ライブラリ側 AMWA_init() でも内部的に同等処理が走るが、明示しておくことで
// 設計意図が読み取りやすくなる。
static void resetHostSerialAndChip() {
  AT_SERIAL.end();
  AT_SERIAL.begin(115200);
  delay(20);
  while (AT_SERIAL.available()) AT_SERIAL.read();
  wifihalow.AMWA_init();
}

// AP 設定を投入して保存する（AT モードであることが前提）
// コマンド順は v1 仕様準拠：WMODE → WAPCFG → WAPIP → WSAVE → SAUDP
// 保存後の reboot/reset は呼び出し側で行う。
static bool configureAndSave() {
  INFO_SERIAL.println("[CONFIG] Setting AP mode (next boot)...");
  if (!wifihalow.mode_set("AP"))                                                   return failStep("WMODE=AP");

  INFO_SERIAL.println("[CONFIG] Setting AP credentials...");
  if (!wifihalow.ap_config_set(AP_SSID, AP_SEC, AP_PASS, AP_CHANNEL))               return failStep("WAPCFG");

  INFO_SERIAL.println("[CONFIG] Setting AP IP...");
  if (!wifihalow.ap_ip_set(AP_IP, AP_NETMASK, AP_GATEWAY))                          return failStep("WAPIP");

  INFO_SERIAL.println("[CONFIG] Saving to flash...");
  if (!wifihalow.settings_save())                                                   return failStep("WSAVE");

  INFO_SERIAL.println("[CONFIG] Setting AutoUDP...");
  uint8_t rxFormat = AUTOUDP_SHOW_RXD ? AUTOUDP_RX_FORMAT_HEADER : AUTOUDP_RX_FORMAT_RAW;
  if (!wifihalow.auto_udp_set(LOCAL_PORT, REMOTE_IP, REMOTE_PORT, AUTOUDP_BAUD, rxFormat))    return failStep("SAUDP");

  return true;
}

// AT_SERIAL <-> INFO_SERIAL 双方向中継（戻ってこない）
//
// SW リングバッファで読み出しと書き込みを分離する。
//   AT_SERIAL  → atToPcBuf → INFO_SERIAL
//   INFO_SERIAL → pcToAtBuf → AT_SERIAL
//
// 設計の要点：
//   * 読み出しは available() > 0 の間 read() で 1 バイトずつ取り込む（非ブロッキング）。
//     readBytes() は内部で timedRead() ループに入り、稀に timeout 待ちで滞る可能性が
//     あるので避ける。
//   * 書き込みは availableForWrite() で書ける分だけに制限し、write() の戻り値
//     （実際に書けたバイト数）で tail を進める。書き込みが詰まっても読み出しは止まらない。
//   * AT→PC 方向 (USB CDC): availableForWrite() == 0 なら write しない。
//     書きにいくと SerialUSB::write() 内空き待ちで AT_SERIAL の drain が止まる。
//   * PC→AT 方向 (HW UART): availableForWrite() が常に 0 を返す前提。fallback
//     chunk size を 1B に絞り、ブロック中の drain 停止時間と境界由来の欠けを抑える。
//   * AT→PC 方向は AutoUDP の +RXD: ヘッダ増幅で流量が大きいので 1024B 確保。
//     PC→AT 方向はユーザ入力なので 256B で足りる。

#define AT_TO_PC_BUF_SIZE 1024
#define PC_TO_AT_BUF_SIZE 256

static uint8_t atToPcBuf[AT_TO_PC_BUF_SIZE];
static size_t  atToPcHead = 0;
static size_t  atToPcTail = 0;

static uint8_t pcToAtBuf[PC_TO_AT_BUF_SIZE];
static size_t  pcToAtHead = 0;
static size_t  pcToAtTail = 0;

static inline size_t rbCount(size_t head, size_t tail, size_t size) {
  return (head >= tail) ? (head - tail) : (size - tail + head);
}

static inline size_t rbFree(size_t head, size_t tail, size_t size) {
  // 1 スロットは「満杯」と「空」の判別のために予約
  return size - 1 - rbCount(head, tail, size);
}

static void bridgeSerial() {
  while (true) {
    // ---- AT_SERIAL → atToPcBuf ----
    // available() > 0 の間 1 バイトずつ read()。readBytes() は内部で timedRead()
    // 経由のループになり、稀に timeout 待ちで滞る可能性があるので避ける。
    while (AT_SERIAL.available() > 0) {
      size_t freeSlots = rbFree(atToPcHead, atToPcTail, AT_TO_PC_BUF_SIZE);
      if (freeSlots == 0) break;          // ring 満杯。次ループで write 側に流す
      int b = AT_SERIAL.read();
      if (b < 0) break;
      atToPcBuf[atToPcHead] = (uint8_t)b;
      atToPcHead = (atToPcHead + 1) % AT_TO_PC_BUF_SIZE;
    }

    // ---- INFO_SERIAL → pcToAtBuf ----
    while (INFO_SERIAL.available() > 0) {
      size_t freeSlots = rbFree(pcToAtHead, pcToAtTail, PC_TO_AT_BUF_SIZE);
      if (freeSlots == 0) break;
      int b = INFO_SERIAL.read();
      if (b < 0) break;
      pcToAtBuf[pcToAtHead] = (uint8_t)b;
      pcToAtHead = (pcToAtHead + 1) % PC_TO_AT_BUF_SIZE;
    }

    // ---- atToPcBuf → INFO_SERIAL ----
    // UNO R4 の SerialUSB は availableForWrite() を実装しているので、0 は
    // 「今は書けない」を意味する。書きにいくと SerialUSB::write() が
    // 空き待ちで block し、AT_SERIAL の受信 drain も止まる。
    // ⇒ writable <= 0 のときは何も書かず次ループに回す（リング側で吸収）。
    {
      size_t used = rbCount(atToPcHead, atToPcTail, AT_TO_PC_BUF_SIZE);
      int writable = INFO_SERIAL.availableForWrite();
      if (used > 0 && writable > 0) {
        size_t want = ((size_t)writable < used) ? (size_t)writable : used;
        size_t contig = AT_TO_PC_BUF_SIZE - atToPcTail;
        if (want > contig) want = contig;
        size_t actual = INFO_SERIAL.write(&atToPcBuf[atToPcTail], want);
        atToPcTail = (atToPcTail + actual) % AT_TO_PC_BUF_SIZE;
      }
    }

    // ---- pcToAtBuf → AT_SERIAL ----
    // Renesas core の UART は availableForWrite() を override していないため
    // 常に 0 が返る前提。一方 UART::write(buf,len) は送信完了まで block する。
    // AutoUDP の手入力用途では PC→AT は 1B ずつ流し、送信側/受信側双方の
    // drain 停止時間と chunk 境界由来の取りこぼしを最小化する。
    {
      size_t used = rbCount(pcToAtHead, pcToAtTail, PC_TO_AT_BUF_SIZE);
      if (used > 0) {
        int writable = AT_SERIAL.availableForWrite();
        size_t want;
        if (writable > 0) {
          want = ((size_t)writable < used) ? (size_t)writable : used;
          if (want > 1) want = 1;
        } else {
          want = 1;   // ブロック短縮・chunk 境界切り分け優先
        }
        size_t contig = PC_TO_AT_BUF_SIZE - pcToAtTail;
        if (want > contig) want = contig;
        size_t actual = AT_SERIAL.write(&pcToAtBuf[pcToAtTail], want);
        pcToAtTail = (pcToAtTail + actual) % PC_TO_AT_BUF_SIZE;
      }
    }
  }
}

void setup() {
  INFO_SERIAL.begin(38400);
  AT_SERIAL.begin(115200);
  delay(1000);

  // AutoUDP 突入時の baud 切替に追従する callback を登録
  wifihalow.set_baud_switch_callback(on_baud_switch);

  INFO_SERIAL.println("=== AMWA AutoUDP AP Sample ===");
  INFO_SERIAL.println("[BOOT] Resetting AMWA-01...");
  wifihalow.AMWA_init();

  while (true) {
    // AP モード初回 boot は ap_init() の link up 待ちに最大 30 秒かかるため、
    // detect_boot_state は 40 秒待つ。AutoUDP 設定済みの再 boot は数百ミリ秒で
    // "AutoUDP" が出るのですぐ抜ける。
    AMWA::BootState state = wifihalow.detect_boot_state(40000);

    if (state == AMWA::BOOT_AUTOUDP) {
      INFO_SERIAL.println("[BOOT] AutoUDP enabled.");

#if FORCE_RECONFIG
      INFO_SERIAL.println("[ESCAPE] Sending AT* to exit AutoUDP...");
      if (!wifihalow.auto_udp_escape(6000)) {
        INFO_SERIAL.println("[ESCAPE] Failed. Resetting and retrying...");
        resetHostSerialAndChip();
        continue;
      }
      INFO_SERIAL.println("[ESCAPE] Now in AT mode.");
      if (!configureAndSave()) {
        INFO_SERIAL.println("[CONFIG] Failed. Resetting and retrying...");
        resetHostSerialAndChip();
        continue;
      }
      // 設定保存はここで完了。
      // 自動 ATZ してそのまま AutoUDP に進めることもできるが、
      // FORCE_RECONFIG=1 のまま放置されると毎起動で flash 書き換えが繰り返される
      // ので、明示的に停止して「0 に戻して再書き込み」をユーザに促す。
      // 0 に戻して再書き込みすれば setup() 冒頭の AMWA_init() で chip も
      // 自動的にリセットされ、保存済み設定を読み直して AutoUDP に入る。
      INFO_SERIAL.println("");
      INFO_SERIAL.println("[CONFIG] Settings saved to AMWA-01 flash.");
      INFO_SERIAL.println("[CONFIG] Next step:");
      INFO_SERIAL.println("[CONFIG]   1) Change  #define FORCE_RECONFIG 1  ->  0");
      INFO_SERIAL.println("[CONFIG]   2) Re-upload this sketch.");
      INFO_SERIAL.println("[CONFIG] Halting here to prevent flash wear from repeated reconfig.");
      while (true) { delay(1000); }
#else
      INFO_SERIAL.println("[BOOT] Waiting for AutoUDP to be ready...");
      if (!wifihalow.wait_autoudp_started(30000)) {
        INFO_SERIAL.println("[BOOT] AutoUDP ready timeout. Resetting...");
        resetHostSerialAndChip();
        continue;
      }
      INFO_SERIAL.println("");
      INFO_SERIAL.println("=== AutoUDP started ===");
      INFO_SERIAL.println("Send text from Serial Monitor to transmit to STA.");
      INFO_SERIAL.println("Received data from STA will appear below.");
      INFO_SERIAL.println("");
      bridgeSerial();
#endif
    }
    else if (state == AMWA::BOOT_AT_MODE) {
      INFO_SERIAL.println("[BOOT] AT mode. Configuring...");
      if (!configureAndSave()) {
        INFO_SERIAL.println("[CONFIG] Failed. Resetting and retrying...");
        resetHostSerialAndChip();
        continue;
      }
#if FORCE_RECONFIG
      // 設定保存はここで完了。AutoUDP 既存設定から抜けた場合と同じく、
      // FORCE_RECONFIG=1 では自動 reboot せず停止する。
      INFO_SERIAL.println("");
      INFO_SERIAL.println("[CONFIG] Settings saved to AMWA-01 flash.");
      INFO_SERIAL.println("[CONFIG] Next step:");
      INFO_SERIAL.println("[CONFIG]   1) Change  #define FORCE_RECONFIG 1  ->  0");
      INFO_SERIAL.println("[CONFIG]   2) Re-upload this sketch.");
      INFO_SERIAL.println("[CONFIG] Halting here to prevent flash wear from repeated reconfig.");
      while (true) { delay(1000); }
#else
      INFO_SERIAL.println("[CONFIG] Done. Rebooting...");
      wifihalow.reboot();
      // ATZ 後はチップが再起動 → 次の iteration で BOOT_AUTOUDP として検出される
#endif
    }
    else {
      INFO_SERIAL.println("[BOOT] No response from AMWA-01. Resetting and retrying...");
      resetHostSerialAndChip();
    }
  }
}

void loop() {
  // setup() 内の while(true) で処理するため使用しない
}
