/*
 * AMWA_AutoUDP_STA
 *
 * AMWA-01 を STA モードで起動し、AutoUDP 通信を自動開始するサンプルです。
 * AP 側と組み合わせて使用してください（AMWA_AutoUDP_AP を参照）。
 *
 * 【動作の流れ】
 *   1. 起動時に AMWA-01 の状態を判定する
 *      - すでに AutoUDP 設定済みなら → ブリッジモードへ直行
 *      - 未設定なら → STA 設定 + AutoUDP 設定 + 保存 + 再起動 → ブリッジモードへ
 *   2. ブリッジモードでは Serial Monitor と AMWA-01 の UART を双方向に中継
 *      - AP から受信したデータは Serial Monitor に表示
 *      - Serial Monitor に入力したデータは AP へ送信
 *
 * 【設定変更したいとき】
 *   一度 AutoUDP 設定が保存されると、Arduino リセットだけでは設定変更できません。
 *   下記 #define FORCE_RECONFIG を 1 に変更してビルドし直すと、
 *   既存設定を破棄して新しい設定で書き換えます。
 *   設定変更後は FORCE_RECONFIG を 0 に戻してください。
 */

#include <AMWA_LIB.h>

// ---- ユーザー設定 ----

// 接続先 AP 設定（AP 側と合わせること）
#define AP_SSID      "testap"
#define AP_SEC       "sae"        // "sae" or "open"（owe は不可）
#define AP_PASS      "12345678"   // open 時は無視される

// STA 自身の IP 設定（固定 IP）
#define STA_IP       "192.168.1.33"
#define STA_NETMASK  "255.255.255.0"
#define STA_GATEWAY  "192.168.1.1"   // 孤立 LAN なので実通信には使われない

// AutoUDP 設定
#define LOCAL_PORT   4099
#define REMOTE_IP    "192.168.1.123"  // AP 側の固定 IP
#define REMOTE_PORT  4099

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

// STA 設定を投入して保存する（AT モードであることが前提）
// コマンド順：WMODE → WDHCP → WIPADDR → WAP → WSAVE → SAUDP → ATZ
static bool configureAndSave() {
  INFO_SERIAL.println("[CONFIG] Setting STA mode (next boot)...");
  if (!wifihalow.mode_set("STA"))                                                   return failStep("WMODE=STA");

  // 固定 IP を使うので DHCP は無効化
  INFO_SERIAL.println("[CONFIG] Disabling DHCP...");
  if (!wifihalow.dhcp_on(DHCP_DISABLE))                                             return failStep("WDHCP=0");

  INFO_SERIAL.println("[CONFIG] Setting STA IP...");
  if (!wifihalow.ipaddr_set(STA_IP, STA_NETMASK, STA_GATEWAY))                      return failStep("WIPADDR");

  INFO_SERIAL.println("[CONFIG] Setting AP credentials...");
  if (!wifihalow.sta_ap_set(AP_SSID, AP_SEC, AP_PASS))                              return failStep("WAP");

  INFO_SERIAL.println("[CONFIG] Saving to flash...");
  if (!wifihalow.settings_save())                                                   return failStep("WSAVE");

  INFO_SERIAL.println("[CONFIG] Setting AutoUDP...");
  if (!wifihalow.auto_udp_set(LOCAL_PORT, REMOTE_IP, REMOTE_PORT))                  return failStep("SAUDP");

  return true;
}

// AT_SERIAL <-> INFO_SERIAL 双方向中継（戻ってこない）
static void bridgeSerial() {
  while (true) {
    while (AT_SERIAL.available()) {
      INFO_SERIAL.write(AT_SERIAL.read());
    }
    while (INFO_SERIAL.available()) {
      AT_SERIAL.write(INFO_SERIAL.read());
    }
  }
}

void setup() {
  INFO_SERIAL.begin(115200);
  AT_SERIAL.begin(115200);
  delay(1000);

  INFO_SERIAL.println("=== AMWA AutoUDP STA Sample ===");
  INFO_SERIAL.println("[BOOT] Resetting AMWA-01...");
  wifihalow.AMWA_init();

  while (true) {
    AMWA::BootState state = wifihalow.detect_boot_state(5000);

    if (state == AMWA::BOOT_AUTOUDP) {
      INFO_SERIAL.println("[BOOT] AutoUDP enabled.");

#if FORCE_RECONFIG
      INFO_SERIAL.println("[ESCAPE] Sending AT* to exit AutoUDP...");
      if (!wifihalow.auto_udp_escape(6000)) {
        INFO_SERIAL.println("[ESCAPE] Failed. Resetting and retrying...");
        wifihalow.AMWA_init();
        continue;
      }
      INFO_SERIAL.println("[ESCAPE] Now in AT mode.");
      if (!configureAndSave()) {
        INFO_SERIAL.println("[CONFIG] Failed. Resetting and retrying...");
        wifihalow.AMWA_init();
        continue;
      }
      INFO_SERIAL.println("[CONFIG] Done. Rebooting...");
      wifihalow.reboot();
#else
      INFO_SERIAL.println("[BOOT] Waiting for AutoUDP to be ready...");
      INFO_SERIAL.println("       (Make sure AP side is running first)");
      // STA は AP 接続を含むため AP 側より長めのタイムアウト
      if (!wifihalow.wait_autoudp_started(60000)) {
        INFO_SERIAL.println("[BOOT] AutoUDP ready timeout. Resetting...");
        wifihalow.AMWA_init();
        continue;
      }
      INFO_SERIAL.println("");
      INFO_SERIAL.println("=== AutoUDP started ===");
      INFO_SERIAL.println("Send text from Serial Monitor to transmit to AP.");
      INFO_SERIAL.println("Received data from AP will appear below.");
      INFO_SERIAL.println("");
      bridgeSerial();
#endif
    }
    else if (state == AMWA::BOOT_AT_MODE) {
      INFO_SERIAL.println("[BOOT] AT mode. Configuring...");
      if (!configureAndSave()) {
        INFO_SERIAL.println("[CONFIG] Failed. Resetting and retrying...");
        wifihalow.AMWA_init();
        continue;
      }
      INFO_SERIAL.println("[CONFIG] Done. Rebooting...");
      wifihalow.reboot();
    }
    else {
      INFO_SERIAL.println("[BOOT] No response from AMWA-01. Resetting and retrying...");
      wifihalow.AMWA_init();
    }
  }
}

void loop() {
  // setup() 内の while(true) で処理するため使用しない
}
