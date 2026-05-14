 ;/**
 * @file updated_tagmachine_main.ino
 * @author 김병준 (you@domain.com)
 * @brief
 * @version 1.0
 * @date 2022-11-29
 *
 * @copyright Copyright (c) 2022
 *
 */

#define FIRMWARE_VER 12
#include "updated_tagmachine_main.h"

void setup() {
    DebugSerial.begin(115200);
    toSubSerial.begin(9600, SERIAL_8N1, SUB_BEETLE_RX_PIN, SUB_BEETLE_TX_PIN);
    toMainSerial.begin(9600, SERIAL_8N1, MAIN_BEETLE_RX_PIN, MAIN_BEETLE_TX_PIN);
    NeopixelInit();
    TimerInit();
    Mp3_Setup();
    pinMode(RELAY_PIN, OUTPUT);
//  has2wifi.Setup("city");
    has2wifi.Setup("badland_ruins", "Code3824@");
    DebugSerial.printf("MAC: %s\r\n", WiFi.macAddress().c_str());
    TelnetInit();
    ota.setLogStream(DebugSerial);
    ota.setOnSuccess([]() {
        has2wifi.Send((String)(const char*)my["device_name"], "device_state", "setting");
    });
    ota.setOnSkip([]() {
        has2wifi.Send((String)(const char*)my["device_name"], "device_state", "setting");
    });
    DataChanged();
    GameSetting();

    // Main Beetle 핸드셰이크 테스트 (최대 10초)
    DebugSerial.println("=== Main Beetle Handshake Start ===");
    unsigned long hsStart = millis();
    bool hsSuccess = false;
    while(millis() - hsStart < 10000) {
        // Main Beetle에서 들어오는 'W' 확인 후 응답
        if(toMainSerial.available() > 0) {
            String cmd = toMainSerial.readStringUntil('\n');
            DebugSerial.println("Main Beetle RX: " + cmd);
            if(cmd[0] == 'W') {
                toMainSerial.println("W");
                DebugSerial.println("Main Beetle TX: W sent");
            }
            while(toMainSerial.available()) toMainSerial.read();
        }
        delay(100);
    }
    DebugSerial.println("=== Main Beetle Handshake End ===");

    // setup() 완료 시점에도 ptrCurrentMode가 설정되지 않았으면 WaitFunc로 안전하게 초기화
    if (ptrCurrentMode == nullptr) {
        ptrCurrentMode = WaitFunc;
        DebugSerial.println("[WARN] ptrCurrentMode was nullptr → set to WaitFunc");
    }
}
void loop() {
    if (ptrCurrentMode != nullptr) ptrCurrentMode();
    TimerRun();
    TelnetRun();
}
