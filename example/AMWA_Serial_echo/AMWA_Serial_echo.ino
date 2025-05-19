#include <AMWA_LIB.h>

//シリアル設定
#define AT_SERIAL  Serial1
#define INFO_SERIAL Serial

AMWA wifihalow(false,&AT_SERIAL,&INFO_SERIAL);

void setup() {
  string restr;
  bool init_finish=false;;
  INFO_SERIAL.begin(115200);
  AT_SERIAL.begin(115200);
  INFO_SERIAL.println("init start");
  wifihalow.AMWA_init();
  delay(1000);
}

void loop() {
  char byte;

  //シリアルを受信した場合、もう片側のシリアルに受信データの送信を行う
  while(INFO_SERIAL.available() > 0){
    byte = INFO_SERIAL.read();
    AT_SERIAL.write(byte);
    AT_SERIAL.flush();
  }

  while(AT_SERIAL.available() > 0){
    byte = AT_SERIAL.read();
    INFO_SERIAL.write(byte);
    INFO_SERIAL.flush();
  }

}
