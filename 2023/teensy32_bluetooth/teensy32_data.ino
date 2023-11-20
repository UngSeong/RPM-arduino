#include "KLS.h"
#include "U8g2lib.h"

#define ALIGN_LEFT 0                    //0 나머지로 체크
#define ALIGN_CENTER_HORIZONTAL 1       //1 나머지로 체크
#define ALIGN_RIGHT 2                   //2 나머지로 체크
#define ALIGN_TOP 0                     //0 몫으로 체크
#define ALIGN_CENTER_VERTICAL 3         //3 몫으로 체크
#define ALIGN_BOTTOM 6                  //6 몫으로 체크
#define ALIGN_CENTER 4                  //4

#define MOTOR_SIDE "DATA"

FlexCAN canBus(250000, 1);
static CAN_message_t canMessage;

KLS klsController(0x05);

//U8g2lib display 변수 정의
U8G2_SSD1306_128X64_NONAME_1_HW_I2C display(U8G2_R0, SCL, SDA, U8X8_PIN_NONE);

uint8_t kphSpeedLimit = 90;
uint8_t kphSpeed = 0;
uint8_t kphMaxSpeed = 0;


//함수 선언
void updateCanData();

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

}

void loop() {
    // put your main code here, to run repeatedly:

    //update Can bus data
    updateCanData();
    //draw display
    draw();

    delay(100);

}

void draw() {
    display.firstPage();
    do {
        //draw speed
        drawMainDrivingMonitor();
        // drawWhichSide();
    } while (display.nextPage());
}

void drawMainDrivingMonitor() {
  display.setColorIndex(1);
  
  display.setFont(u8g2_font_6x10_mr);

  printText(ALIGN_TOP + ALIGN_LEFT, 5, 10, String("RPM: ") + klsController.status.rpm);
  printText(ALIGN_TOP + ALIGN_LEFT, 5, 30, String("AMP: ") + klsController.status.current);
  printText(ALIGN_TOP + ALIGN_LEFT, 5, 50, String("VOL: ") + klsController.status.voltage);
  printText(ALIGN_TOP + ALIGN_LEFT, 69, 10, String("THR: ") + klsController.status.throttle);
  printText(ALIGN_TOP + ALIGN_LEFT, 69, 30, String("CON: ") + klsController.status.controller_temp);
  printText(ALIGN_TOP + ALIGN_LEFT, 69, 50, String("MOT: ") + klsController.status.rpm);

}

void drawWhichSide() {
  printText(ALIGN_LEFT + ALIGN_BOTTOM, 3, 60, MOTOR_SIDE);
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
  }
}
