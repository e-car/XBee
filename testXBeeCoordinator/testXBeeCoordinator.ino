// ヘッダーのインクルード
#include <signal.h> // you must write down this line to resolve problem between WiFiSocket and Serial communication
#include <WiFi.h>
#include "EV86.h"
#include "ev86XBee.h"

/* -------------------------------- Wifi Parameters  -------------------------------- */
char ssid[] = "BUFFALO-4C7A25"; // your network SSID (name), nakayama:506A 304HWa-84F1A0
char pass[] = "iebiu6ichxufg"; // your network password (use for WPA, or use as key for WEP), nakayama:12345678 11237204a
int keyIndex = 0; // your network key Index number (needed only for WEP)
int status = WL_IDLE_STATUS;
int timeoutCount = 0;
WiFiClient client;
WiFiClient lastClient;
WiFiServer server(3000); // 3000番ポートを指定
int timeOutConnect = 0;
boolean connectStatus;

// Routerの情報
typedef struct {
  uint32_t h64Add;
  uint32_t l64Add;
  String nodeName;
  String startAck;
  String sensorData; // 取得したセンサーデータ
  int timeout;
  boolean transmit;
} XBeeNode;

// ルーター情報の設定
XBeeNode router = { 0x0013A200, 0x40993791, "EDISON", "startAck1", "None", 50, false };
XBeeNode router2 = { 0x0013A200, 0x40707DF7, "MEGA", "startAck2", "None", 50, false };

// コーディネーター用のインスタンスを生成
EV86XBeeC coor = EV86XBeeC();

// EV86インスタンス生成
EV86 ev86 = EV86();

// プロトタイプ宣言
void connectProcess(XBeeNode& router);
void gettingData(XBeeNode& router);

// 接続パラメータ
String startReq = "startReq";
String startAck = "startAck";
String request =  "request";

void setup() {
  /* you must write down a following line */
  signal(SIGPIPE, SIG_IGN); // caution !! Please don't erase this line
  
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
  
  // set WiFi
  //setWiFi();
  
  // ホストXBeeの内部受信バッファをフラッシュする
  coor.bufFlush();
  delay(1000);
  
//  // ホストXBeeの設定確認
//  coor.hsXBeeStatus();                     
//  delay(2000);
  
//  // リモートXBeeのアドレス指定と設定情報の取得
//  coor.setDstAdd64(router.h64Add, router.l64Add);
//  coor.rmXBeeStatus();
//  delay(2000);
  
//  // リモートXBeeのアドレス指定と設定情報の取得
//  coor.setDstAdd64(router2.h64Add, router2.l64Add);
//  coor.rmXBeeStatus();
//  delay(2000);
  
  //コネクション確立のためのセッション
  connectProcess(router);
  connectProcess(router2);
}

void loop() {  
  Serial.println("[[[[[[[[[[[[[[[ loop start ]]]]]]]]]]]]]]]");
  Serial.println("-------------------------------------------------");
  //　サーバー(Edison)として指定したポートにクライアント(Android)からのアクセスがあるか確認。あれば、接続する 
//  client = server.available();
//  Serial.print("Client Status : ");
//  Serial.println(client);
  
//  // クライアント(Android)が存在する場合
//  if (client) { 
//    // クライアント(Android)とサーバー(Edison)
//    // 処理に約5秒かかる
//    connectStatus = client.connected();
//    Serial.print("Connect Status : ");
//    Serial.println(connectStatus);
//    
//    if (connectStatus) {
//      Serial.println("Connected to Client");
      
      gettingData(router);
      Serial.println("*******************************************");  
      gettingData(router2);  
//      client.println(router2.sensorData);
      Serial.println("-------------------------------------------------");
//      client.flush();
//      client.stop();
//    }
//  } else {
//    Serial.println("No Client");
//  }
 //delay(1000); 
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
  for (int apiID, i = 0; (apiID = coor.getPacket()) >= 0; i++) {
    
    if (apiID == ZB_RX_RESPONSE) {
      if (coor.checkData(router.startAck))
        break;
    }
    
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
  router.transmit = true;
  
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
  delay(500);
  
  // 受信パケットの確認
  int apiID;
  while ((apiID = coor.getPacket()) >= 0) {
    if (apiID == ZB_RX_RESPONSE) {
      // 受信したセンサーデータ確認
      router.sensorData = coor.getData();
      Serial.print("Sensor Data: "); 
      Serial.println(router.sensorData);
      Serial.println();
    } 
  };
  
  if (router.sensorData.equals("")) {
   router.transmit = false;
  } else {
   router.transmit = true;
  } 
  
  // 接続状態の確認
  Serial.print("Connect Status : ");
  Serial.println(router.transmit);
  
  // 受信データの初期化
  coor.clearData();
}
