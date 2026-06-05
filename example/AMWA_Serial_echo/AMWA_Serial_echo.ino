#include <AMWA_LIB.h>

//シリアル設定
#define AT_SERIAL  Serial1
#define INFO_SERIAL Serial
#define INFO_BAUD 115200
#define AT_BAUD 115200

AMWA wifihalow(false,&AT_SERIAL,&INFO_SERIAL);

static void init_serial_ports() {
  // PC側USBシリアルとAMWA接続UARTを初期化
  INFO_SERIAL.begin(INFO_BAUD);
  AT_SERIAL.begin(AT_BAUD, SERIAL_8N1);
}

static void init_amwa_bridge() {
  // AMWAリセット後、AT受信側の内部状態を初期化
  INFO_SERIAL.println("init start");
  wifihalow.AMWA_init();
  wifihalow.at_receive_begin();
}

void setup() {
  init_serial_ports();
  init_amwa_bridge();
  delay(1000);
}

void loop() {
  // 1) AT側受信を先に吸い上げる
  wifihalow.at_receive_poll();
  // 2) CR終端でまとまった応答のみLOG側へ転送
  wifihalow.at_output_block(INFO_SERIAL);

  // 3) 応答中は逆方向転送を止める
  if (wifihalow.at_receive_waiting_response()) {
    return;
  }

  // 4) 応答待ちでないときだけLOG->ATを転送（ブリッジ制御は.ino側で実施）
  int budget = 64;
  while (budget-- > 0 && INFO_SERIAL.available() > 0) {
    int c = INFO_SERIAL.read();
    if (c >= 0) {
      wifihalow.at_send_byte((uint8_t)c);
    }
  }
}
