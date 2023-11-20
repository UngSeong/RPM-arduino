#include <SoftwareSerial.h>
#include "KLS.h"
#include "U8g2lib.h"

#define ALIGN_LEFT 0                    //0 나머지로 체크
#define ALIGN_CENTER_HORIZONTAL 1       //1 나머지로 체크
#define ALIGN_RIGHT 2                   //2 나머지로 체크
#define ALIGN_TOP 0                     //0 몫으로 체크
#define ALIGN_CENTER_VERTICAL 3         //3 몫으로 체크
#define ALIGN_BOTTOM 6                  //6 몫으로 체크
#define ALIGN_CENTER 4                  //4

#define MOTOR_SIDE "L"

#define RXD 1
#define TXD 0

FlexCAN canBus(250000, 2);
KLS klsController(0x05);
static CAN_message_t canMessage;

SoftwareSerial bluetooth(TXD, RXD);

//U8g2lib display 변수 정의
U8G2_SSD1306_128X64_NONAME_1_HW_I2C display(U8G2_R0, SCL, SDA, 8);

uint8_t kphSpeedLimit = 90;
uint8_t kphSpeed = 0;
uint8_t kphMaxSpeed = 0;

bool isReceivingBluetoothData = false;
uint8_t receivedBluetoothDataVersion = 0;
uint8_t receiveDataBuffer[100];
String receivedBluetoothData;

unsigned int frameSpacing = 100;
unsigned int frameStartTime = 0;
unsigned int frameEndTime = 0;
unsigned int frameCount = 0;


//함수 선언
void updateCanData();

void sendBluetoothData();

void receiveBluetoothData();

void processReceivedData();

void updateFrameData();

void draw();

void printText(int8_t align, int16_t x, int16_t y, const String &text);

void printText(int8_t align, int16_t x, int16_t y, const char *text);

void drawMainDrivingMonitor();


void setup() {
    // put your setup code here, to run once:

    //initialize u8g2 display
    display.begin();
    display.setFontPosTop();
    display.setColorIndex(1);

    canBus.begin();

    bluetooth.begin(9600);
}

void loop() {
    // put your main code here, to run repeatedly:0

    //update Can bus data
    updateCanData();

    //send bluetooth data
    sendBluetooth();

    //draw display
    draw();
    
    updateFrameData();
    if(frameCount % 100 == 99) {
      display.begin();
    }
}

void draw() {
    display.firstPage();
    do {
        //draw speed
        drawMainDrivingMonitor();
        drawWhichSide();
    } while (display.nextPage());
}

void drawMainDrivingMonitor() {
  display.setColorIndex(1);

  // 속력 드로잉
  String speedStr = String(kphSpeed);
  if (kphSpeed < 10) {
      display.setFont(u8g2_font_inb38_mn);
      printText(ALIGN_RIGHT + ALIGN_BOTTOM, 80 - 5, 60 + 6, " " + speedStr);
  } else if (kphSpeed < 100) {
      display.setFont(u8g2_font_inb38_mn);
      printText(ALIGN_RIGHT + ALIGN_BOTTOM, 80 - 5, 60 + 6, speedStr);
  } else {
      display.setFont(u8g2_font_inb27_mn);
      printText(ALIGN_RIGHT + ALIGN_BOTTOM, 80 - 5, 60 + 5, speedStr);
  }


  // Km/h 드로잉
  display.setFont(u8g_font_7x13);
  printText(ALIGN_LEFT + ALIGN_BOTTOM, 80, 60, String("Km/h"));


  // 속력 게이지 드로잉
  uint8_t gageValue = map(kphSpeed, 0, kphSpeedLimit, 0, 118);
  if (gageValue > 118) gageValue = 118;
  else if (gageValue < 0) gageValue = 0;
  display.drawFrame(4, 4, 120, 8);
  display.drawBox(5, 5, gageValue, 6);


  // 속력 게이지 50마다 표시 드로잉
  display.setColorIndex(2);
  for (int i = 50; i < kphSpeedLimit; i += 50) {
    display.drawVLine(5 + map(i, 0, kphSpeedLimit, 0, 118), 4 - 1, 10);
  }


  // 최대속도 기록 드로잉
  if (kphMaxSpeed < kphSpeed) kphMaxSpeed = kphSpeed;
  display.setFont(u8g_font_7x13);
  printText(ALIGN_LEFT + ALIGN_TOP, 1, 36, String(kphMaxSpeed));


  // 최대 효율 커서 드로잉
  // if (showHighEfficiencySpeed) {
  //     display.setColorIndex(1);
  //     uint8_t x1 = 5 + map(getAvgHiEfSpeed(), 0, vehicleMaxSpeed, 0, 118);
  //     uint8_t y1 = 12;
  //     display.drawTriangle(x1, y1, x1 - 3, y1 + 5, x1 + 3, y1 + 5);
  // }
}

void drawWhichSide() {
  display.setFont(u8g_font_7x13);
  printText(ALIGN_LEFT + ALIGN_BOTTOM, 1, 62, MOTOR_SIDE);
}

void printText(int8_t align, int16_t x, int16_t y, const String &text) {
    char charSeq[100];
    text.toCharArray(charSeq, text.length() + 1);
    printText(align, x, y, charSeq);
}

void printText(int8_t align, int16_t x, int16_t y, const char *text) {
    display.setFontPosTop();
    uint8_t width = display.getStrWidth(text);
    uint8_t height = display.getFontAscent() - display.getFontDescent();

    align = ((align % 9) + 9) % 9;

    x -= align % 3 * width / 2;
    y -= align / 3 * height / 2;

    display.drawStr(x, y, text);
}

void updateCanData() {
  if (canBus.available()) {

    canBus.read(canMessage);
    klsController.parse(canMessage);

    kphSpeed = klsController.status.rpm * 60 * 0.000001 * 505 * 3.14;
    kphSpeed *= 1.1; // 계기판 속도 10% 증가
  }
}

void sendBluetooth() {
  float vol = klsController.status.voltage;

  String str = String(receivedBluetoothDataVersion) + ' ' + vol + ' ' + '\n';

  uint8_t len = str.length() + 1;
  uint8_t bytes[len];
  str.getBytes(bytes, len);

  for (int i = 0; i < len; i++) {
    bluetooth.write(bytes[i]);
  }
}

void receiveBluetoothData() {
  if(!bluetooth.available()) return;
  
  if(!isReceivingBluetoothData) {
    isReceivingBluetoothData = true;
    
  }

  int temp;

  for(uint8_t i = 0; i < 100; i++) {
    temp = bluetooth.read();
    if(temp == -1) break;

    receivedBluetoothData += (char) temp;

    if(temp == '\n') {
      processReceivedBluetoothData();
    }
  }

}

void processReceivedBluetoothData() {
  isReceivingBluetoothData = false;
}

void updateFrameData() {
  frameEndTime = millis();
  uint8_t error = frameEndTime - frameStartTime;

  if (error < frameSpacing) {
    delay(frameSpacing - error);
  }
  frameStartTime = millis();
  frameCount++;
}
