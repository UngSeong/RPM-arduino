#include "KLS.h"
#include "U8g2lib.h"

#define ALIGN_LEFT 0                    //0 나머지로 체크
#define ALIGN_CENTER_HORIZONTAL 1       //1 나머지로 체크
#define ALIGN_RIGHT 2                   //2 나머지로 체크
#define ALIGN_TOP 0                     //0 몫으로 체크
#define ALIGN_CENTER_VERTICAL 3         //3 몫으로 체크
#define ALIGN_BOTTOM 6                  //6 몫으로 체크
#define ALIGN_CENTER 4                  //4

#define MOTOR_SIDE "R"

FlexCAN canBus(250000, 2);
static CAN_message_t canMessage;

KLS klsController(0x05);

//U8g2lib display 변수 정의
U8G2_SSD1306_128X64_NONAME_1_HW_I2C display(U8G2_R0, SCL, SDA, 8);

float voltageHigh = 54.00;
float voltageLow = 40.00;
float voltageNow = 0;
float voltageFixed = 0;

unsigned int frameSpacing = 100;
unsigned int frameStartTime = 0;
unsigned int frameEndTime = 0;
unsigned int frameCount = 0;


//함수 선언
void updateCanData();

void updateFrameData();

void draw();

void printText(int8_t align, int16_t x, int16_t y, const String &text);

void printText(int8_t align, int16_t x, int16_t y, const char *text);

void drawMainDrivingMonitor();


void setup() {
    // put your setup code here, to run once:
    Serial.begin(9600);

    //initialize u8g2 display
    display.begin();
    display.setFontPosTop();
    display.setColorIndex(1);

    canBus.begin();

    pinMode(13, INPUT);
}

void loop() {
    // put your main code here, to run repeatedly:

    //update Can bus data
    updateCanData();
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

  // 고정 전압 드로잉
  display.setFont(u8g2_font_inb27_mn);
  printText(ALIGN_CENTER_HORIZONTAL + ALIGN_BOTTOM, 64, 52, String(voltageFixed));


  // 고정 전압 게이지 드로잉
  uint8_t gageValue = map(voltageFixed, voltageLow, voltageHigh, 0, 118);
  if (gageValue > 118) gageValue = 118;
  else if (gageValue < 0) gageValue = 0;
  display.drawFrame(4, 4, 120, 8);
  display.drawBox(5, 5, gageValue, 6);


  // 실제 전압 드로잉
  display.setFont(u8g_font_7x13);
  printText(ALIGN_LEFT + ALIGN_BOTTOM, 16, 62, String(voltageNow));


  // 컨트롤러 온도 드로잉
  display.setFont(u8g_font_7x13);
  printText(ALIGN_RIGHT + ALIGN_BOTTOM, 123, 62, String(klsController.status.controller_temp));

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

    voltageNow = klsController.status.voltage - 0.4f;
    if (klsController.status.throttle < 1.5f) {
      voltageFixed = voltageNow;
    }
    
  }
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
