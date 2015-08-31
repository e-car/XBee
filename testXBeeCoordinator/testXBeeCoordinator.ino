// ヘッダーのインクルード
#include "ev86XBee.h"

// Routerの情報
typedef struct {
  uint32_t h64Add;
  uint32_t l64Add;
  String nodeName;
  String startAck;
  String sensorData; // 取得したセンサーデータ
  int timeout;
  boolean transmit;
  boolean firstTrans;
} XBeeNode;

// ルーター情報の設定
XBeeNode router = { 0x0013A200, 0x40993791, "EDISON", "startAck1", "None", 50, false, false };
XBeeNode router2 = { 0x0013A200, 0x40707DF7, "MEGA", "startAck2", "None", 50, false, false };

// コーディネーター用のインスタンスを生成
EV86XBeeC coor = EV86XBeeC();

// プロトタイプ宣言
void connectProcess(XBeeNode& router);
void gettingData(XBeeNode& router);

// 接続パラメータ
String startReq = "startReq";
String startAck = "startAck";
String request =  "request";

void setup() {  
  /*
   * serial port document
   * Serial : /dev/ttyGS0 in Edison(Linux OS), thus your arduino IDE serial monitor
   * Serial1 : /dev/ttyMFD1 in Edison(Linux OS), thus Pin 0(Rx) and 1(Tx) in your arduino breakout board for Edison
   * Serial2 : /dev/ttyMFD2 in Edison(Linux OS), thus a terminal when you login to J3 in arduino breakout board for Edison with microUSB-B   
  */
  Serial.begin(9600);                          // Arduino-PC間の通信
  Serial1.begin(9600);                         // Arduino-XBee間の通信
  Serial1.flush();                             // serial bufferをクリア
  coor.begin(Serial1);                         // XBeeにSerialの情報を渡す
  delay(5000);                                 // XBeeの初期化のために5秒間待機
  
  // ホストXBeeの内部受信バッファをフラッシュする
  coor.bufFlush();
  delay(1000);
  
  // ホストXBeeの設定確認
  coor.hsXBeeStatus();                     
  delay(2000);
  
  // リモートXBeeのアドレス指定と設定情報の取得
  coor.setDstAdd64(router.h64Add, router.l64Add);
  coor.rmXBeeStatus();
  delay(2000);
  
  // リモートXBeeのアドレス指定と設定情報の取得
  coor.setDstAdd64(router2.h64Add, router2.l64Add);
  coor.rmXBeeStatus();
  delay(2000);
  
  //コネクション確立のためのセッション
  connectProcess(router);
  connectProcess(router2);
}

void loop() {  
  Serial.println("[[[[[[[[[[[[[[[ loop start ]]]]]]]]]]]]]]]");
  Serial.println("-------------------------------------------------");  

  // センサーデータ取得  
  gettingData(router);
  Serial.println("*******************************************");  
  gettingData(router2);  
  
  // 接続状態の確認
  // router firstTrans
  Serial.print(router.nodeName);
  Serial.print(" First Connect Status : ");
  Serial.println(router.firstTrans);
  // router
  Serial.print(router.nodeName);
  Serial.print(" Connect Status : ");
  Serial.println(router.transmit);
  
  // router2 firstTrans
  Serial.print(router2.nodeName);
  Serial.print(" First Connect Status : ");
  Serial.println(router2.firstTrans);
  // router2
  Serial.print(router2.nodeName);
  Serial.print(" Connect Status : ");
  Serial.println(router2.transmit);
  Serial.println("-------------------------------------------------");
  delay(1000);
}


void connectProcess(XBeeNode& router) {
  // コネクション確立のためのセッション
  /* ------------------------------------------------- */
  // coor →  router
  // coor ← router
  // coor →  router
  
  // コネクションを取るXBeeのノード名を確認
  Serial.print("[[[[[ CONNECTION with ");
  Serial.print(router.nodeName);
  Serial.println(" ]]]]]");
  
  // ルーターへの接続要求送信
  coor.setDstAdd64(router.h64Add, router.l64Add);
  coor.sendData(startReq);
  Serial.println("[[[ send startReq ]]]");
  
  // この遅延処理は重要！
  delay(200);
  
  // ルーターから接続応答が来ているかチェック
  // 受信パケットの確認
  Serial.print("[get Packet from ");
  Serial.print(router.nodeName);
  Serial.println("]");
  for (int apiID, i = 0; (apiID = coor.getPacket()) != ZB_RX_RESPONSE && !coor.checkData(router.startAck); i++) {
    
    // タイムアウトの確認
    Serial.print("timecount : ");
    Serial.println(i);
    
    // timeoutを超過したらルータへの接続要求を再送信する
    if (i > router.timeout) {
      coor.setDstAdd64(router.h64Add, router.l64Add);
      coor.sendData(startReq);
      i = 0;
      Serial.println("[[[ send startReq again ]]]\n");
    }
    
    // 受信データの初期化
    coor.clearData();
    delay(200);
  }
  
  // 接続応答を送信
  coor.setDstAdd64(router.h64Add, router.l64Add);
  coor.sendData(startAck);
  Serial.println("send startAck\n");
  
  delay(500);
  
  // 受信パケットの確認 
  Serial.println("[get Packet]"); 
  coor.getPacket();
    
  // コネクションの確立
  Serial.print("[[[ Connected with Router in ");
  Serial.print(router.nodeName);
  Serial.println(" ]]]");
  Serial.println("-------------------------------------------------------------");
  router.firstTrans = true;
  
  // 受信データの初期化
  coor.clearData();
  /* ------------------------------------------------- */
}




// ポーリングによる各XBeeへのリクエスト送信とレスポンス受信
void gettingData(XBeeNode& router) {
  // センサーデータ要求
  coor.setDstAdd64(router.h64Add, router.l64Add); // 宛先アドレスの指定
  coor.sendRequest(request);                      // リクエスト送信
  
  // 受信パケットの確認
  Serial.print("[get Packet from ");
  Serial.print(router.nodeName);
  Serial.println("]");
  
  // この遅延処理は重要！
  //delay(500);
  
  // 受信パケットの確認
  int count = 0;
  while (true) {  
    int apiID = coor.getPacket();
    
    if (count > router.timeout) {
      router.transmit = false;
      break;
    }
      
    if (apiID == ZB_RX_RESPONSE) {
      router.transmit = true;
      // 受信したセンサーデータ確認
      router.sensorData = coor.getData();
      Serial.print("Sensor Data: "); 
      Serial.println(router.sensorData);
      Serial.println();
      break;
    }  
      
    if (apiID < 0)
      count++;
    
    delay(20);  
  };
  
  // 受信データの初期化
  coor.clearData();
}
