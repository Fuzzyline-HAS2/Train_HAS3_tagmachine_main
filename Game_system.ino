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
        has2wifi.Send((String)(const char*)my["device_name"], "device_state", "activate");
        RoundNeoEffectDown(BLACK);
        has2wifi.Loop(DataChanged); // activate 상태 업데이트 수신
        AllNeoOn(YELLOW);
    }
    digitalWrite(RELAY_PIN, LOW);
}

void TaggerDeviceState(){
    // 서버에서 device_state="tagger"를 받으면 호출.
    // 현재 state를 기억 → 보라색/문 열림으로 10초 유지(블로킹이라 RFID 폴링이 멈춰
    // 술래/생존자/유령 태그가 무시됨) → 문 닫고 기억해둔 state로 복귀.
    String savedState = strCurState;                 // 지금 state 기억
    DebugSerial.println("[TAGGER] start, saved=" + savedState);

    AllNeoOn(PURPLE);                                // 네오픽셀 보라색
    digitalWrite(RELAY_PIN, LOW);                    // 자동문 열림

    // 술래 침입 알람음(VD10) 재생 후 10초 유지.
    // 블로킹 delay라 RFID 폴링이 멈추므로, 3초마다 알람을 반복해
    // 문이 열려 있는 동안 계속 알람이 들리도록 함(끝 2초 전까지만 재생).
    Mp3PlayLargeFolder(1, VD10);                      // 알람 시작
    unsigned long elapsed = 0;
    while (elapsed < 10000) {                         // 10초 유지(이 동안 태그 무시)
        delay(1000);
        elapsed += 1000;
        if (elapsed % 3000 == 0 && elapsed < 8000)
            Mp3PlayLargeFolder(1, VD10);             // 3초마다 알람 반복
    }

    digitalWrite(RELAY_PIN, HIGH);                   // 자동문 닫힘
    DebugSerial.println("[TAGGER] end, restore=" + savedState);

    // 서버 device_state를 이전 값으로 되돌리고 로컬 상태/연출 복원
    has2wifi.Send((String)(const char*)my["device_name"], "device_state", savedState);
    if (savedState == "lock" || savedState == "activate" || savedState == "debuff")
        ApplyDeviceState(savedState);                // 기억해둔 state로 복귀(연출 포함)
    else
        strCurState = savedState;
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
    has2wifi.Send((String)(const char*)my["device_name"], "device_state", "open");
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
