/**
 * @file main.cpp
 * @brief アレクサからTVの電源をオンできるようにする 
 * @author Toshiki Nagahama
 * @date 2020/8/15
 * @note 電源ON -> Wifi接続 -> Alexa呼び出しコールバック登録
 *       -> Alexaから呼ばれたとき赤外線リモコンでコマンド送信
 */

/************************************************
 * ライブラリ読み込み
 ************************************************/
//M5Atom
#include <M5Atom.h>
//アレクサ関連
#include <Espalexa.h>
//赤外線リモコン関連
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <IRutils.h>
//その他
#include "config.h"

/************************************************
 * グローバル変数宣言
 ************************************************/
//M5Atom関連
uint8_t buf_display[2 + 5 * 5 * 3]; //ディスプレイのLEDのバッファ
//Wifi関連
const char *SSID_WIFI = MY_SSID; //wifiのSSID（config.hに記載）
const char *PASS_WIFI = MY_PASS; //wifiのパスワード（config.hに記載）
bool isConnectedWifi = false;    //wifiに接続しているかどうか
//アレクサ関連
Espalexa espalexa;
//赤外線リモコン関連
const uint16_t PIN_IR_SEND = 26;                 //赤外線送信LEDのピン
IRsend irsend(PIN_IR_SEND);                      // 送信オブジェクト
const uint64_t COM_TV_POWER_CHANGE = 0x210704FB; //TVの電源切替コマンド（船井のTV）
const uint16_t SIZE_COM_TV_POWER_CHANGE = 32;    //TVの電源切替コマンドのサイズ
//状態
uint8_t state = 0; //今回は簡易的な状態

/************************************************
 * プロトタイプ宣言
 ************************************************/
bool connectWifi();                                        //wifiに接続
void tvPowerChanged(EspalexaDevice *device);               //TVの電源切替
void setBuff(uint8_t Rdata, uint8_t Gdata, uint8_t Bdata); //M5AtomのLEDの色を設定

/************************************************
 * 関数宣言
 ************************************************/
/**
 * @brief M5AtomのLEDの色を設定
 * 
 */
void setBuff(uint8_t Rdata, uint8_t Gdata, uint8_t Bdata)
{
  buf_display[0] = 0x05;
  buf_display[1] = 0x05;
  for (int i = 0; i < 25; i++)
  {
    buf_display[2 + i * 3 + 0] = Rdata;
    buf_display[2 + i * 3 + 1] = Gdata;
    buf_display[2 + i * 3 + 2] = Bdata;
  }
}

/**
 * @brief wifiに接続
 * 
 */
bool connectWifi()
{
  bool res = true;
  int i = 0;

  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID_WIFI, PASS_WIFI);
  Serial.println("");
  Serial.println("Connecting to WiFi");

  // Wait for connection
  Serial.print("Connecting...");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
    if (i > 20)
    {
      res = false;
      break;
    }
    i++;
  }
  Serial.println("");
  if (res)
  {
    Serial.print("Connected to ");
    Serial.println(SSID_WIFI);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  }
  else
  {
    Serial.println("Connection failed.");
  }
  return res;
}

/**
 * @brief TVの電源切替（コールバック関数）
 * 
 */
void tvPowerChanged(EspalexaDevice *d)
{
  //デバイスがNullなら何もしない
  if (d == nullptr)
    return;
  if (d->getValue())
  {
    irsend.sendNEC(COM_TV_POWER_CHANGE, SIZE_COM_TV_POWER_CHANGE); //コマンドの送信
    Serial.println("TV Power Changed!");

    //緑に点滅
    for (int i = 0; i < 5; i++)
    {
      setBuff(0x00, 0x40, 0x00);
      M5.dis.displaybuff(buf_display);
      delay(500);
      setBuff(0x00, 0x00, 0x00);
      M5.dis.displaybuff(buf_display);
      delay(100);
    }
    //青に点灯
    setBuff(0x00, 0x00, 0x40);
    M5.dis.displaybuff(buf_display);
  }
}

/**
 * @brief セットアップ
 * 
 */
void setup()
{
  //M5Atomの設定
  M5.begin(true, false, true);
  setBuff(0x40, 0x40, 0x40); //初期は白(255, 255, 255)
  M5.dis.displaybuff(buf_display);
  //赤外線リモコンの設定
  irsend.begin(); // 赤外線LEDの設定

  delay(100);

  while (!connectWifi())
  {
    Serial.println("Cannot connect to WiFi. Please check data and reset the ESP.");
    //ネットワーク一覧を表示する
    WiFi.disconnect();           //一度Wi-Fi切断
    int n = WiFi.scanNetworks(); //ネットワークをスキャンして数を取得
    if (n == 0)
    {
      //ネットワークが見つからないとき
      Serial.println("no networks found");
    }
    else
    {
      //ネットワークが見つかったとき
      Serial.print(n);
      Serial.println(" networks found\n");
      for (int i = 0; i < n; i++)
      {
        Serial.print(i + 1);
        Serial.print(": ");
        Serial.print(WiFi.channel(i)); //チャンネルを表示
        Serial.print("CH (");
        Serial.print(WiFi.RSSI(i)); //RSSI(受信信号の強度)を表示
        Serial.print(")");
        Serial.print((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? " " : "*"); //暗号化の種類がOPENか否か
        Serial.print("  ");
        Serial.print(WiFi.SSID(i)); //SSID(アクセスポイントの識別名)を表示
        Serial.println("");
        delay(10);
      }
    }
    //赤に点滅
    for (int i = 0; i < 5; i++)
    {
      setBuff(0x40, 0x00, 0x00);
      M5.dis.displaybuff(buf_display);
      delay(500);
      setBuff(0x00, 0x00, 0x00);
      M5.dis.displaybuff(buf_display);
      delay(100);
    }
  }

  //Wifiに接続されたら、青に点灯
  setBuff(0x00, 0x00, 0x40);
  M5.dis.displaybuff(buf_display);
  espalexa.addDevice("Smart TV", tvPowerChanged); //アレクサにデバイスを登録
  espalexa.begin();                               //スタート
}

/**
 * @brief ループ
 * 
 */
void loop()
{
  espalexa.loop(); //アレクサ用のループ
  delay(1);
}