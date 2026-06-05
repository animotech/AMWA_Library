/*
 * AMWA_AUTOUDP_STA_DEMO
 *
 * AMWA-01 を STA モード + AutoUDP で起動するサンプルです。
 * 対向には AMWA_AUTOUDP_AP_DEMO を使用してください。
 *
 * 役割:
 *   - 自分: STA 側（AutoUDP）
 *   - 相手: AP 側（AutoUDP）
 *
 * Serial Monitor 設定:
 *   - baud rate: 115200
 *   - line ending: CRLF
 *   - 通信が不安定な場合は USB 直結を推奨（ハブ経由は不安定化する場合あり）
 *
 * メモ:
 *   - AutoUDP 動作中は、Serial Monitor で入力したデータをそのまま UDP 送信します。
 *   - 受信データは Serial Monitor に表示されます。
 */

#include <AMWA_LIB.h>

#define AT_SERIAL Serial1
#define INFO_SERIAL Serial

#define AMWA_IPADDR "192.168.11.12"
#define SUBNET "255.255.255.0"
#define GATEWAY "192.168.11.1"

#define SSID "AMWx_AP"
#define SEC "sae"
#define PASS "12345678"

#define AUTOUDP_LOCAL_PORT 4098
#define AUTOUDP_REMOTE_IP "192.168.11.17"
#define AUTOUDP_REMOTE_PORT 4105

#define BRIDGE_BUDGET 64
// INFO_SERIAL <-> AT_SERIAL の 1 ループ処理量。環境に応じて 32〜128 程度で調整。
// 小さくすると応答性重視、大きくするとスループット重視。

AMWA wifihalow(false, &AT_SERIAL, &INFO_SERIAL);

static void halt_with_reset(const char *msg) {
  INFO_SERIAL.println(msg);
  NVIC_SystemReset();
}

static void ensure_at_mode() {
  // 1) まず AMWA-01 をハードウェアリセットして既知状態から開始
  INFO_SERIAL.println("[BOOT] Resetting AMWA-01...");
  wifihalow.AMWA_init();
  delay(1000);

  // 2) 起動状態を判別（AT モード / AutoUDP モード / 応答なし）
  INFO_SERIAL.println("[BOOT] Detecting boot state...");
  AMWA::BootState state = wifihalow.detect_boot_state(40000);

  // 3-a) すでに AT モードならそのまま設定フェーズへ進む
  if (state == AMWA::BOOT_AT_MODE) {
    INFO_SERIAL.println("[BOOT] AT mode.");
    return;
  }

  // 3-b) AutoUDP モード起動時は AT* で抜けて AT モードへ戻す
  if (state == AMWA::BOOT_AUTOUDP) {
    INFO_SERIAL.println("[BOOT] AutoUDP detected. Escaping...");
    if (!wifihalow.auto_udp_escape(6000)) {
      halt_with_reset("[BOOT] AutoUDP escape failed.");
    }
    INFO_SERIAL.println("[BOOT] AT mode.");
    return;
  }
  halt_with_reset("[BOOT] No response from AMWA-01.");
}

static void configure_and_reboot_autoudp_sta() {
  // 1) 次回起動モードを STA に設定
  INFO_SERIAL.println("[CFG] Setting STA mode...");
  if (!wifihalow.mode_set("STA")) {
    halt_with_reset("[CFG] Failed to switch to STA mode.");
  }

  // 2) STA 側の固定 IP 情報を設定
  if (!wifihalow.ipaddr_set(AMWA_IPADDR, SUBNET, GATEWAY)) {
    halt_with_reset("[CFG] Failed to set STA IP address.");
  }

  // 3) 接続先 AP の credentials を設定
  if (!wifihalow.sta_ap_set(SSID, SEC, PASS)) {
    halt_with_reset("[CFG] Failed to set STA AP credentials.");
  }

  // 4) DHCP を無効化（固定 IP 運用）
  if (!wifihalow.dhcp_on(DHCP_DISABLE)) {
    halt_with_reset("[CFG] Failed to disable DHCP.");
  }

  // 5) ここまでの設定を不揮発領域へ保存
  if (!wifihalow.settings_save()) {
    halt_with_reset("[CFG] Failed to save settings.");
  }

  // 6) AutoUDP のローカル/相手先情報を設定
  if (!wifihalow.auto_udp_set(AUTOUDP_LOCAL_PORT, AUTOUDP_REMOTE_IP, AUTOUDP_REMOTE_PORT)) {
    halt_with_reset("[CFG] Failed to set AutoUDP.");
  }

  // 7) 再起動して AutoUDP モードへ遷移
  INFO_SERIAL.println("[CFG] Rebooting to AutoUDP mode...");
  wifihalow.reboot();
}

void setup() {
  // Arduino 側シリアル初期化（Monitor / AMWA UART）
  INFO_SERIAL.begin(115200);
  AT_SERIAL.begin(115200);
  delay(1000);

  INFO_SERIAL.println("AMWA AUTOUDP STA DEMO Start");

  ensure_at_mode();
  configure_and_reboot_autoudp_sta();

  // AutoUDP socket 起動完了まで待機（+SOPEN 到達を待つ）
  if (!wifihalow.wait_autoudp_started(60000)) {
    halt_with_reset("[BOOT] AutoUDP start timeout.");
  }
  INFO_SERIAL.println("[BOOT] AutoUDP started.");

  // ブリッジ内部状態を初期化
  wifihalow.at_receive_begin();
}

void loop() {
  // 1) AT受信を内部FIFOへ取り込み
  wifihalow.at_receive_poll();

  // 2) CR/LF終端または強制フラッシュ条件でまとめてSerial Monitorへ出力
  wifihalow.at_output_block(INFO_SERIAL);

  // 3) AutoUDPでは受信中でも送信を止めない
  int budget = BRIDGE_BUDGET;
  while (budget-- > 0 && INFO_SERIAL.available() > 0) {
    int c = INFO_SERIAL.read();
    if (c >= 0) {
      AT_SERIAL.write((uint8_t)c);
    }
  }
}
