void TimerInit(){
    wifiTimerId = WifiTimer.setInterval(2000,WifiIntervalFunc);
    // gameTimerId = GameTimer.setInterval(500,GameTimerFunc);
    // GameTimer.deleteTimer(gameTimerId);
    // subSerialTimerId = SubSerialTimer.setInterval(1000,SubSerialTimerFunc);
    // SubSerialTimer.deleteTimer(subSerialTimerId);
    // debuffTimerId = DebuffTimer.setInterval(1000,DebuffTimerFunc);
    // DebuffTimer.deleteTimer(debuffTimerId);
}

void TimerRun(){
    WifiTimer.run();
    GameTimer.run();
    SubSerialTimer.run();
    DebuffTimer.run();
    ApplyPendingDeviceState();
}

// WiFi 상태 폴링 + 양쪽 Beetle 시리얼 수신을 2초 주기로 처리.
void WifiIntervalFunc(){
    has2wifi.Loop(DataChanged);
    CommnunicationBeetle();         // Sub Beetle
    CommnunicationMainBeetle();     // Main Beetle
}

// 1초 주기로 호출되어 ptrGameTimer(현재 진행 중인 타이머 함수)를 실행.
void GameTimerFunc(){
    DebugSerial.println("GameTimer");
    if (ptrGameTimer != nullptr) ptrGameTimer();
}

// 두 번째 카드 태그 대기 타임아웃 처리.
// SubSerialTimer가 만료되면 첫 번째 태그 후 두 번째 카드가 오지 않은 것으로 판단 → ptrRfidFail() 호출.
void SubSerialTimerFunc(){
    SubSerialTimer.deleteTimer(subSerialTimerId);
    SubSerialTimerStart = false;
    if (ptrRfidFail != nullptr) ptrRfidFail();
    while(toSubSerial.available())
      toSubSerial.read();
    while(toMainSerial.available())
      toMainSerial.read();
}

// debuff 지속 시간(60초) 만료 시 자동으로 activate 복귀.
void DebuffTimerFunc(){
    DebuffTimer.deleteTimer(debuffTimerId);
    DebugSerial.println("debuff time end");
    has2wifi.Send((String)(const char*)my["device_name"], "device_state", "activate");
    ReturnNormalState();
}

// 태그 진행을 완전히 취소하고 대기 상태로 돌아감.
// ReturnNormalState()와의 차이: loginDone도 false로 초기화해 1단계 태그부터 다시 시작.
void CancelTagProgress(){
    GameTimer.deleteTimer(gameTimerId);
    SubSerialTimer.deleteTimer(subSerialTimerId);
    SubSerialTimerStart = false;
    gameTimerCnt = 0;
    loginDone = false;

    ptrRfidMain = CommnunicationMainBeetle;
    ptrRfidSub = CommnunicationBeetle;
    ptrRfidMode = Login;
    ptrRfidFail = WaitFunc;

    WifiTimer.deleteTimer(wifiTimerId);
    wifiTimerId = WifiTimer.setInterval(2000, WifiIntervalFunc);
    SubSerialFlush();
    MainSerialFlush();
    DebugSerial.println("Cancel Tag Progress");
    ApplyPendingDeviceState();
}

// 타이머/포인터를 초기화하고 태그 대기 상태로 복귀.
// loginDone은 false로 초기화되어 다음 태그부터 1단계부터 다시 시작.
// 게임이 정상 완료(성공/실패)된 뒤 항상 이 함수를 거쳐 복귀.
void ReturnNormalState(){
    ptrRfidMain = CommnunicationMainBeetle;
    ptrRfidSub = CommnunicationBeetle;
    ptrRfidMode = Login;
    ptrRfidFail = WaitFunc;
    gameTimerCnt = 0;

    loginDone = false;
    GameTimer.deleteTimer(gameTimerId);
    SubSerialTimer.deleteTimer(subSerialTimerId);
    WifiTimer.deleteTimer(wifiTimerId);
    wifiTimerId = WifiTimer.setInterval(2000,WifiIntervalFunc); // WiFi 타이머 재시작

    DebugSerial.println("Return Normal State");
    SubSerialFlush();
    MainSerialFlush();
}

// activate 상태에서 생존자가 문을 잠그는 타이머.
void PlayerLockTimerFunc(){
    gameTimerCnt++;
    RoundNeoToggle(GREEN,gameTimerCnt);
    LineNeoUp(GREEN, YELLOW, map(gameTimerCnt,0,playerLockTime,0,NumPixels[LINE]));
    DebugSerial.println(map(gameTimerCnt,0,playerLockTime,0,NumPixels[LINE]));
    if(gameTimerCnt == 1)
        Mp3PlayLargeFolder(1, VD11);
    if(gameTimerCnt > (playerLockTime))
    {
        if(strCurState != "activate"){
            DebugSerial.println("debuff on");
            CancelTagProgress();
        }
        else {
            DebugSerial.println("debuff off");
            has2wifi.Send((String)(const char*)my["device_name"], "device_state", "lock");
            DebugSerial.println("DOOR LOCK!");
            Mp3PlayLargeFolder(1, VD4);
            has2wifi.ReceiveMine();
            ReturnNormalState();
            RoundNeoEffect(GREEN);
            SubSerialFlush();
            MainSerialFlush();
        }
    }
}

// ── unlock 타이머 구조 ───────────────────────────────────────────────────────
// 각 역할(생존자/술래/유령)마다 일반/뉴비 쌍이 존재.
// 공통 구조: XxxUnlockTimerBody(onSuccess) → 매 틱 네오픽셀 업데이트 + 시간 만료 시
//             debuff 감지 → CancelTagProgress()
//             정상 완료  → onSuccess() 콜백 실행
// 이 패턴 덕분에 타이머 바디 코드 중복 없이 일반/뉴비 성공 동작만 교체 가능.
// ─────────────────────────────────────────────────────────────────────────────

// ── Player Unlock ────────────────────────────────────────────
// 성공 시 문을 열고 activate 상태로 전환 (일반 모드).
void PlayerUnlockSuccess() {
    DebugSerial.println("DOOR UNLOCK!");
    Mp3PlayLargeFolder(1, VD7);
    ReturnNormalState();
    digitalWrite(RELAY_PIN, HIGH);
    // has2wifi.Send((String)(const char*)my["device_name"], "device_state", "open");
    RoundNeoEffect(YELLOW);
    DoorOpen();
    has2wifi.ReceiveMine();
    SubSerialFlush();
    MainSerialFlush();
}

// 성공 시 문을 열었다 즉시 lock으로 복귀 (뉴비 모드).
// 뉴비 모드는 역할 무관 항상 "도어 오픈"(VD1)만 재생 — VD7(잠금 해제)을 쓰면
// NewbieOpenBody의 VD1 명령이 DFPlayer에서 드랍될 때 VD7이 끝까지 들리는 문제가 있었음.
void NewbiePlayerSuccess() {
    DebugSerial.println("DOOR UNLOCK (Newbie Player)!");
    Mp3PlayLargeFolder(1, VD1);
    NewbiePlayerOpen();
}

void PlayerUnlockTimerBody(void (*onSuccess)(), bool withSound) {
    gameTimerCnt++;
    RoundNeoToggle(GREEN, gameTimerCnt);
    LineNeoDown(YELLOW, GREEN, map(gameTimerCnt, 0, playerUnlockTime, 0, NumPixels[LINE]));
    if (gameTimerCnt == 1 && withSound) Mp3PlayLargeFolder(1, VD11);
    if (gameTimerCnt > playerUnlockTime) {
        if (strCurState != "lock") { DebugSerial.println("debuff on"); CancelTagProgress(); }
        else { onSuccess(); }
    }
}

void PlayerUnlockTimerFunc()       { PlayerUnlockTimerBody(PlayerUnlockSuccess, true);  }
void NewbiePlayerUnlockTimerFunc() { PlayerUnlockTimerBody(NewbiePlayerSuccess, false); }

// ── Tagger Unlock ─────────────────────────────────────────────
// 성공 시 문을 열고 activate 상태로 전환 (일반 모드).
void TaggerUnlockSuccess() {
    Mp3PlayLargeFolder(1, VD1);
    DebugSerial.println("DOOR UNLOCK!");
    ReturnNormalState();
    digitalWrite(RELAY_PIN, HIGH);
    // has2wifi.Send((String)(const char*)my["device_name"], "device_state", "open");
    RoundNeoEffect(PURPLE);
    DoorOpen();
    SubSerialFlush();
    MainSerialFlush();
}

// 성공 시 문을 열었다 즉시 lock으로 복귀 (뉴비 모드).
// 일반 모드와 달리 DoorOpen() 대신 GhostDoorOpen() 후 직접 "lock" 전송.
void NewbieTaggerSuccess() {
    Mp3PlayLargeFolder(1, VD1);
    DebugSerial.println("DOOR UNLOCK (Newbie)!");
    ReturnNormalState();
    digitalWrite(RELAY_PIN, HIGH);
    // has2wifi.Send((String)(const char*)my["device_name"], "device_state", "open");
    RoundNeoEffect(PURPLE);
    GhostDoorOpen();
    has2wifi.Send((String)(const char*)my["device_name"], "device_state", "lock");
    AllNeoOn(GREEN);
    SubSerialFlush();
    MainSerialFlush();
    delay(1000);
    has2wifi.Loop(DataChanged);
}

void TaggerUnlockTimerBody(void (*onSuccess)(), bool withSound) {
    gameTimerCnt++;
    RoundNeoToggle(PURPLE, gameTimerCnt);
    LineNeoDown(PURPLE, GREEN, map(gameTimerCnt, 0, taggerUnlockTime, 0, NumPixels[LINE]));
    // NeoPixel show() 모두 끝낸 뒤 오디오(SoftwareSerial) 호출 → 인터럽트 충돌 회피
    // 3틱마다 침입 시도 효과음, 마지막 2틱 전까지만 재생(끝에서 짤리지 않도록)
    if (withSound && gameTimerCnt%3 == 1 && gameTimerCnt < (taggerUnlockTime - 2))
        Mp3PlayLargeFolder(1, VD10);
    if (gameTimerCnt > taggerUnlockTime) {
        if (strCurState != "lock") { DebugSerial.println("debuff on"); CancelTagProgress(); }
        else { onSuccess(); }
    }
}

void TaggerUnlockTimerFunc()       { TaggerUnlockTimerBody(TaggerUnlockSuccess, true);  }
void NewbieTaggerUnlockTimerFunc() { TaggerUnlockTimerBody(NewbieTaggerSuccess, true); }

// ── Ghost Unlock ──────────────────────────────────────────────
// 성공 시 문을 열었다 즉시 lock으로 복귀 (일반/뉴비 모두 같은 사이클).
// 유령은 일반 모드에서도 activate를 거치지 않고 lock으로 돌아오는 것이 원래 규칙.
void GhostUnlockSuccess() {
    Mp3PlayLargeFolder(1, VD1);
    DebugSerial.println("GHOST OPEN");
    ReturnNormalState();
    digitalWrite(RELAY_PIN, HIGH);
    // has2wifi.Send((String)(const char*)my["device_name"], "device_state", "open");
    RoundNeoEffect(BLUE);
    GhostDoorOpen();
    has2wifi.Send((String)(const char*)my["device_name"], "device_state", "lock");
    AllNeoOn(GREEN);
    SubSerialFlush();
    MainSerialFlush();
    delay(1000);
    has2wifi.Loop(DataChanged);
}

void NewbieGhostSuccess() {
    DebugSerial.println("GHOST OPEN (Newbie)!");
    Mp3PlayLargeFolder(1, VD1);
    NewbieGhostOpen(); // NewbieOpenBody(BLUE) 호출 — 내부에서 VD1을 한 번 더 재생함
}

void GhostUnlockTimerBody(void (*onSuccess)()) {
    gameTimerCnt++;
    RoundNeoUp(BLUE, GREEN, map(gameTimerCnt, 0, ghostOpenTime, 0, NumPixels[ROUND]/2));
    if (gameTimerCnt > ghostOpenTime) {
        if (strCurState != "lock") { DebugSerial.println("debuff on"); CancelTagProgress(); }
        else { onSuccess(); }
    }
}

void GhostUnlockTimerFunc()       { GhostUnlockTimerBody(GhostUnlockSuccess); }
void NewbieGhostUnlockTimerFunc() { GhostUnlockTimerBody(NewbieGhostSuccess); }

// activate 상태에서 유령이 문을 잠그는 타이머.
// GhostUnlockTimerFunc의 반대 방향(activate → lock).
void GhostLockTimerFunc(){
    gameTimerCnt++;
    RoundNeoUp(BLUE, YELLOW, map(gameTimerCnt,0,ghostOpenTime,0,NumPixels[ROUND]/2));
    if(gameTimerCnt > (ghostOpenTime))
    {
        if(strCurState != "activate"){
            DebugSerial.println("debuff on");
            CancelTagProgress();
        }
        else {
            Mp3PlayLargeFolder(1, VD1);
            DebugSerial.println("GHOST OPEN");
            ReturnNormalState();
            digitalWrite(RELAY_PIN, HIGH);
            // has2wifi.Send((String)(const char*)my["device_name"], "device_state", "open");
            RoundNeoEffect(BLUE);
            GhostDoorOpen();
            has2wifi.Send((String)(const char*)my["device_name"], "device_state", "activate");
            AllNeoOn(YELLOW);
            SubSerialFlush();
            MainSerialFlush();
            delay(1000);
            has2wifi.Loop(DataChanged);
        }
    }
}
