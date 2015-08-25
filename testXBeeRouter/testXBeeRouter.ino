/*
*XBee APIフレーム用送受信プログラム
*随時更新していきます。
*現在できている項目
*  1: 宛先Addressの設定
*  2: 通常データの送受信
*  3: ATコマンドによるホストXBeeへの要求と応答の受け取り
*
*/

#define MEGA
//#define EDISON

// ヘッダーのインクルード
#ifdef MEGA
#include <SoftwareSerial.h>
#endif
#include "ev86XBee.h"   

#ifdef MEGA
int rx = 10;
int tx = 11;
SoftwareSerial myserial = SoftwareSerial(rx, tx);
#endif

// Coordinatorの情報
typedef struct {
  uint32_t h64Add;
  uint32_t l64Add;
  String nodeName;
  String startReq;
  String startAck;
  boolean transmit;
} XBeeNode;

// Coordinator情報の初期化
XBeeNode coor = { 0x0013A200, 0x40B77090, "Coordinator", "startReq", "startAck", false };  

// XBeeをRouterとして起動
EV86XBeeR router = EV86XBeeR();

String request = "request";    // コールバック用変数

#ifdef MEGA
String startAck = "startAck1"; // コネクション許可応答
String senData = "MEGAsensor"; // センサー値(仮)
#else
String startAck = "startAck2"; // コネクション許可応答
String senData = "EDISO_yahoo!!!"; // センサー値(仮)
#endif

void setup() {
  Serial.begin(9600);                          // Arduino-PC間の通信
#ifdef MEGA
  myserial.begin(9600);
  myserial.flush();
  router.begin(myserial);
#endif

#ifdef EDISON
  Serial1.begin(9600);                         // Arduino-XBee間の通信
  Serial1.flush();
  router.begin(Serial1);
#endif
  delay(5000); 
  
  // ホストXBeeの内部受信バッファをフラッシュする
  router.bufFlush();                         
  delay(1000);
  
//  // ホストXBeeの設定確認用メソッド
//  router.hsXBeeStatus();                     
//  delay(2000);
  
//  // リモートXBeeの情報を確認
//  router.setDstAdd64(coor.h64Add, coor.l64Add);
//  router.rmXBeeStatus();
  Serial.println("Finish checking destination xbee node parameters");
  delay(2000);
}

void loop() {
  Serial.println("-----------------------------");
  // 受信データの初期化
  router.clearData();
  
  // 受信パケットの確認
  Serial.println("[get Packet]");
  router.getPacket();
   
  // 受信データが接続要求だった場合 
  if (router.checkData(coor.startReq)) {
   Serial.println("get [startReq]");
   
   // 接続応答を送信
   Serial.println("send startAck");
   router.sendData(startAck);
   
   // コーディネータからの接続応答を確認
   Serial.println("[get Packet]");
   for (int apiID, i = 0; ((apiID = router.getPacket()) != ZB_RX_RESPONSE) && !router.checkData(coor.startAck); i++) {
     Serial.print("timeout : ");
     Serial.println(i);
     
    // 受信データの初期化
    router.clearData();
    delay(100); 
   }
   coor.transmit = true;
   Serial.println("Connected with coordinator"); 
  } // 受信データがリクエストだった場合 
  else if (router.checkData(request)) {
    // センサーデータを送信する
    Serial.print("send : ");
    Serial.println(senData);
    router.sendData(senData);
    
    while(router.getPacket() != ZB_TX_STATUS_RESPONSE) {
      delay(20);
    }; 
    coor.transmit = true;
  } // データが来ていない場合
  else {
    Serial.println("Not send");
    coor.transmit = false;
  }
 
  // 接続状態の確認
  Serial.print("Transmit Status : ");
  Serial.println(coor.transmit);
  
  // 受信データの初期化
  router.clearData();
  delay(300);
}
