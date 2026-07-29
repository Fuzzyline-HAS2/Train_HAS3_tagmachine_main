void WhichTagged()
{
    // loop()에서 매 프레임 호출. Main/Sub Beetle 양쪽 RFID 리더를 모두 폴링.
    if (ptrRfidMain != nullptr) ptrRfidMain();
    if (ptrRfidSub  != nullptr) ptrRfidSub();
}

void DoorOpen(){
    // 문을 열고 device_state를 "activate"로 서버에 보고.
    // debuff 상태에서 호출되면 서버 전송 없이 물리적으로만 열림(릴레이 LOW).
    digitalWrite(RELAY_PIN, LOW);
    if(strCurState == "debuff"){
        DebugSerial.println("DEBUFF OPEN");
    }
    else{
        esp_task_wdt_reset();
        has2wifi.Send((String)(const char*)my["device_name"], "device_state", "activate");
        RoundNeoEffectDown(BLACK);
        esp_task_wdt_reset();
        has2wifi.Loop(DataChanged); // activate 상태 업데이트 수신
        AllNeoOn(YELLOW);
    }
    digitalWrite(RELAY_PIN, LOW);
}

void GhostDoorOpen(){
    // 유령/뉴비 전용 열림. 문을 잠깐 열었다가 닫기만 함.
    // DoorOpen()과 달리 서버 상태를 activate로 바꾸지 않음.
    // 호출한 쪽에서 has2wifi.Send("lock") 등으로 다음 상태를 직접 처리해야 함.
    digitalWrite(RELAY_PIN, LOW);
    RoundNeoEffectDown(BLACK);
    // delay(3000);
}

// ======================== NEWBIE MODE ========================
// 뉴비 모드(easy)는 성공해도 항상 lock → 열림 → lock 사이클로 반복됨.
// 일반 모드와 달리 문이 activate 상태로 유지되지 않고 즉시 lock으로 복귀.

// 뉴비 성공 시 공통 처리를 담당하는 내부 헬퍼.
// NewbiePlayerOpen / NewbieGhostOpen은 색상(neoColor)만 다르게 이 함수를 호출.
void NewbieOpenBody(int neoColor) {
    ReturnNormalState();
    ptrRfidMode = Login;
    Mp3PlayLargeFolder(1, VD1);
    digitalWrite(RELAY_PIN, HIGH);
    // has2wifi.Send((String)(const char*)my["device_name"], "device_state", "open");
    RoundNeoEffect(neoColor);
    GhostDoorOpen();                                                                 // 물리적으로 열었다 닫음
    has2wifi.Send((String)(const char*)my["device_name"], "device_state", "lock");  // 즉시 lock으로 복귀
    AllNeoOn(GREEN);
    SubSerialFlush();
    MainSerialFlush();
    delay(1000);
    has2wifi.Loop(DataChanged);
}

void NewbiePlayerOpen() { NewbieOpenBody(GREEN); }  // 생존자 성공 → GREEN 연출
void NewbieGhostOpen()  { NewbieOpenBody(BLUE);  }  // 유령 성공 → BLUE 연출
