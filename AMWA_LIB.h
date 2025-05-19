
#ifndef _AMWA_AT_H_
#define _AMWA_AT_H
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


using namespace std;
class AMWA
{
  public:
  typedef  struct waitresult {
    bool result;
    String restr;
  }WaitResult;
  bool logon;
  Stream* at_serial;
  Stream* log_serial;
  AMWA(bool on, Stream *amwa_serial,Stream *arduino_serial);
  void AT_Send(String atcmd,String para);
  bool ipaddr_set(String  ipaddr,String  subnet, String  gateway);
  bool dhcp_on(int enable);
  void AMWA_init( void );
  WaitResult waitResponce(String res, int timeout_ms ,int mode=ALLMATCH);
  bool wifiConnect(String ssid, String security , String pass, int timeout_ms);

  int UDP_Open(uint16_t port);
  bool UDP_Send(int id, String ipaddr,uint16_t port,String  sendstr);
  int available(int id);
  String passive_recv(int id,int len);
  bool recvmode_set(int mode,int event);
 
};


 
#endif //_AMWA_AT_H_