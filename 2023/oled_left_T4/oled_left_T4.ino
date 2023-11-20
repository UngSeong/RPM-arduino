#include <Arduino.h>
#include "FlexCAN_T4.h"

#include "KLS_T4.h"

#include "U8g2lib.h"

#define ALIGN_LEFT 0                    //0 나머지로 체크
#define ALIGN_CENTER_HORIZONTAL 1       //1 나머지로 체크
#define ALIGN_RIGHT 2                   //2 나머지로 체크
#define ALIGN_TOP 0                     //0 몫으로 체크
#define ALIGN_CENTER_VERTICAL 3         //3 몫으로 체크
#define ALIGN_BOTTOM 6                  //6 몫으로 체크
#define ALIGN_CENTER 4                  //4

FlexCAN_T4<CAN2> canBus;

// CAN struct to store received data
static CAN_message_t msg_rx;

// Left motor initialized with source address 0x05
KLS kls_r(0x05);

float r_rpm;
float r_current;
float r_voltage;
float r_throttle;
float r_moter_temp;
float r_controller_temp;


String canLoadResult;
int s = 0; //속도

//U8g2lib displayU8g2 변수 정의
U8G2_SSD1306_128X64_NONAME_1_HW_I2C display(U8G2_R0, SCL, SDA, U8X8_PIN_NONE);


//프레임 변수
boolean frameDelayed;
uint32_t frameCount;
uint32_t beforeFrameTime;
uint32_t currentFrameTime;
uint16_t deltaFrameTime;

uint8_t speedMeterLimit = 120;

int canStatus;


int updateCanData();

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
    canBus.setBaudRate(250000);
    canBus.enableFIFO(true);
    canBus.enableDMA(true);
    canBus.disableFIFOInterrupt();

}

void loop() {
    // put your main code here, to run repeatedly:

    //update Can bus data
    s = updateCanData();
    //draw display
    draw();

    delay(10);

}

void draw() {
    display.firstPage();
    do {
        //draw speed
        drawMainDrivingMonitor();
    } while (display.nextPage());
}

void drawMainDrivingMonitor() {
    display.setColorIndex(1);

    // 속력 드로잉
    uint8_t speed = s; //<<--- 여기에 속력 입력
    String speedStr = String(speed);
    if (speed < 10) {
        display.setFont(u8g2_font_inb38_mn);
        printText(ALIGN_RIGHT + ALIGN_BOTTOM, 80 - 5, 60 + 6, " " + speedStr);
    } else if (speed < 100) {
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
    uint8_t speedGageValue = map(speed, 0, speedMeterLimit, 0, 118);
    display.drawFrame(4, 4, 120, 8);
    display.drawBox(5, 5, speedGageValue, 6);


    // 속력 게이지 50마다 표시 드로잉
    display.setColorIndex(2);
    for (int i = 50; i < speedMeterLimit; i += 50) {
        display.drawVLine(5 + map(i, 0, speedMeterLimit, 0, 118), 4 - 1, 10);
    }


    // 최대 효율 커서 드로잉
    // if (showHighEfficiencySpeed) {
    //     display.setColorIndex(1);
    //     uint8_t x1 = 5 + map(getAvgHiEfSpeed(), 0, vehicleMaxSpeed, 0, 118);
    //     uint8_t y1 = 12;
    //     display.drawTriangle(x1, y1, x1 - 3, y1 + 5, x1 + 3, y1 + 5);
    // }
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

int updateCanData() {
    if (canBus.readFIFO(msg_rx)) {
        kls_r.parse(msg_rx);
        Serial.print("Right Motor:\n");
        kls_r.print();  //왼쪽 모터의 RPM, current, voltage, throttle, motor temp, controller temp 등 정보 출력

        r_rpm = kls_r.print_rpm();      //rpm
        r_current = kls_r.print_current();     //current
        r_voltage = kls_r.print_voltage();     //voltage
        r_throttle = kls_r.print_throttle();   //throttle
        r_moter_temp = kls_r.print_motor_temp();//motor temp
        r_controller_temp = kls_r.print_controller_temp(); //controlloer temp

        s = r_rpm * 60 * 0.000001 * 505 * 3.14;
        Serial.println("----------------------------");
        Serial.println("S: ");
    } else {
      Serial.println("can unavailable");
    }
    return s;
}
