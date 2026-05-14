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

/**
 * @brief WIFI read 타이머 주기별로 받는 함수
 */
void WifiIntervalFunc(){
    has2wifi.Loop(DataChanged);
    CommnunicationBeetle();         // Sub Beetle
    CommnunicationMainBeetle();     // Main Beetle
}

/**
 * @brief 다중 태그를 인식하기 위한 타이머 함수
 */
void GameTimerFunc(){
    DebugSerial.println("GameTimer");
    if (ptrGameTimer != nullptr) ptrGameTimer();
}

/**
 * @brief 반대쪽 SUB 태그머신(Beetle)에서 데이터를 타이머 주기별로 받는 함수
 */
void SubSerialTimerFunc(){
    SubSerialTimer.deleteTimer(subSerialTimerId);
    SubSerialTimerStart = false;
    if (ptrRfidFail != nullptr) ptrRfidFail();
    while(toSubSerial.available())
      toSubSerial.read();
    while(toMainSerial.available())
      toMainSerial.read();
}
void DebuffTimerFunc(){
    DebuffTimer.deleteTimer(debuffTimerId);
    DebugSerial.println("debuff time end");
    has2wifi.Send((String)(const char*)my["device_name"], "device_state", "activate");
    ReturnNormalState();
}

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

    SubSerialFlush();
    MainSerialFlush();
    DebugSerial.println("Cancel Tag Progress");
    ApplyPendingDeviceState();
}

/**
 * @brief 일반 상태로 돌아가는 함수
 */
void ReturnNormalState(){           
    ptrRfidMain = CommnunicationMainBeetle;
    ptrRfidSub = CommnunicationBeetle;
    ptrRfidMode = Login;
    ptrRfidFail = WaitFunc;
    gameTimerCnt = 0;

    loginDone = false;
    GameTimer.deleteTimer(gameTimerId); 
    SubSerialTimer.deleteTimer(subSerialTimerId);   
    WifiTimer.deleteTimer(wifiTimerId);                                          //게임 타이머 종료
    wifiTimerId = WifiTimer.setInterval(2000,WifiIntervalFunc);
    
    DebugSerial.println("Return Normal State");
    SubSerialFlush();                                                               //시리얼 통신 버퍼 flush
    MainSerialFlush();                                                              //Main Beetle 시리얼 버퍼 flush
}

/** 
 * @brief 잠기지 않은 도어를 플레이어가 도어가 잠금을 하기위한 함수
 */
void PlayerLockTimerFunc(){
    gameTimerCnt++;
    RoundNeoToggle(GREEN,gameTimerCnt);
    LineNeoUp(GREEN, YELLOW, map(gameTimerCnt,0,playerLockTime,0,NumPixels[LINE]));
    DebugSerial.println(map(gameTimerCnt,0,playerLockTime,0,NumPixels[LINE]));
    if(gameTimerCnt == 1)                                                         // 3번마다 "도어잠금 효과음" 나오게 하기
        Mp3PlayLargeFolder(1, VD11);
    if(gameTimerCnt > (playerLockTime))
    {
        has2wifi.ReceiveMine();
        DataChanged();
        // DebugSerial.println("strCurState:" + String(strCurState));
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
        }                                                     //시리얼 통신 버퍼 flush
    }
}

// ── Player Unlock ────────────────────────────────────────────
void PlayerUnlockSuccess() {
    DebugSerial.println("DOOR UNLOCK!");
    Mp3PlayLargeFolder(1, VD7);
    ReturnNormalState();
    digitalWrite(RELAY_PIN, HIGH);
    has2wifi.Send((String)(const char*)my["device_name"], "device_state", "open");
    RoundNeoEffect(YELLOW);
    DoorOpen();
    has2wifi.ReceiveMine();
    SubSerialFlush();
    MainSerialFlush();
}

void NewbiePlayerSuccess() {
    DebugSerial.println("DOOR UNLOCK (Newbie Player)!");
    Mp3PlayLargeFolder(1, VD7);
    NewbiePlayerOpen();
}

void PlayerUnlockTimerBody(void (*onSuccess)()) {
    gameTimerCnt++;
    RoundNeoToggle(GREEN, gameTimerCnt);
    LineNeoDown(YELLOW, GREEN, map(gameTimerCnt, 0, playerUnlockTime, 0, NumPixels[LINE]));
    if (gameTimerCnt == 1) Mp3PlayLargeFolder(1, VD11);
    if (gameTimerCnt > playerUnlockTime) {
        has2wifi.ReceiveMine();
        DataChanged();
        if (strCurState != "lock") { DebugSerial.println("debuff on"); CancelTagProgress(); }
        else { onSuccess(); }
    }
}

void PlayerUnlockTimerFunc()       { PlayerUnlockTimerBody(PlayerUnlockSuccess); }
void NewbiePlayerUnlockTimerFunc() { PlayerUnlockTimerBody(NewbiePlayerSuccess); }

// ── Tagger Unlock ─────────────────────────────────────────────
void TaggerUnlockSuccess() {
    Mp3PlayLargeFolder(1, VD1);
    DebugSerial.println("DOOR UNLOCK!");
    ReturnNormalState();
    digitalWrite(RELAY_PIN, HIGH);
    has2wifi.Send((String)(const char*)my["device_name"], "device_state", "open");
    RoundNeoEffect(PURPLE);
    DoorOpen();
    SubSerialFlush();
    MainSerialFlush();
}

void NewbieTaggerSuccess() {
    Mp3PlayLargeFolder(1, VD1);
    DebugSerial.println("DOOR UNLOCK (Newbie)!");
    ReturnNormalState();
    digitalWrite(RELAY_PIN, HIGH);
    has2wifi.Send((String)(const char*)my["device_name"], "device_state", "open");
    RoundNeoEffect(PURPLE);
    GhostDoorOpen();
    has2wifi.Send((String)(const char*)my["device_name"], "device_state", "lock");
    AllNeoOn(GREEN);
    SubSerialFlush();
    MainSerialFlush();
    delay(1000);
    has2wifi.Loop(DataChanged);
}

void TaggerUnlockTimerBody(void (*onSuccess)()) {
    gameTimerCnt++;
    RoundNeoToggle(PURPLE, gameTimerCnt);
    if (gameTimerCnt%3 == 1 && gameTimerCnt < (taggerUnlockTime - 2))
        Mp3PlayLargeFolder(1, VD10);
    LineNeoDown(PURPLE, GREEN, map(gameTimerCnt, 0, taggerUnlockTime, 0, NumPixels[LINE]));
    if (gameTimerCnt > taggerUnlockTime) {
        has2wifi.ReceiveMine();
        DataChanged();
        if (strCurState != "lock") { DebugSerial.println("debuff on"); CancelTagProgress(); }
        else { onSuccess(); }
    }
}

void TaggerUnlockTimerFunc()       { TaggerUnlockTimerBody(TaggerUnlockSuccess); }
void NewbieTaggerUnlockTimerFunc() { TaggerUnlockTimerBody(NewbieTaggerSuccess); }

// ── Ghost Unlock ──────────────────────────────────────────────
void GhostUnlockSuccess() {
    Mp3PlayLargeFolder(1, VD1);
    DebugSerial.println("GHOST OPEN");
    ReturnNormalState();
    digitalWrite(RELAY_PIN, HIGH);
    has2wifi.Send((String)(const char*)my["device_name"], "device_state", "open");
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
    NewbieGhostOpen();
}

void GhostUnlockTimerBody(void (*onSuccess)()) {
    gameTimerCnt++;
    RoundNeoUp(BLUE, GREEN, map(gameTimerCnt, 0, ghostOpenTime, 0, NumPixels[ROUND]/2));
    if (gameTimerCnt > ghostOpenTime) {
        has2wifi.ReceiveMine();
        DataChanged();
        if (strCurState != "lock") { DebugSerial.println("debuff on"); CancelTagProgress(); }
        else { onSuccess(); }
    }
}

void GhostUnlockTimerFunc()       { GhostUnlockTimerBody(GhostUnlockSuccess); }
void NewbieGhostUnlockTimerFunc() { GhostUnlockTimerBody(NewbieGhostSuccess); }

/**
 * @brief 잠겨있지 않은 도어 유령이 잠금해제를 하기위한 함수
 */
void GhostLockTimerFunc(){      
    gameTimerCnt++;
    // RoundNeoToggle(BLUE,gameTimerCnt);
    // LineNeoUp(BLUE, YELLOW, map(gameTimerCnt,0,ghostOpenTime,0,NumPixels[LINE]));
    RoundNeoUp(BLUE, YELLOW, map(gameTimerCnt,0,ghostOpenTime,0,NumPixels[ROUND]/2));
    if(gameTimerCnt > (ghostOpenTime))
    {
        has2wifi.ReceiveMine();
        DataChanged();
        // DebugSerial.println("strCurState:" + String(strCurState));
        if(strCurState != "activate"){
            DebugSerial.println("debuff on");
            CancelTagProgress();
        }
        else {
            Mp3PlayLargeFolder(1, VD1);
            DebugSerial.println("GHOST OPEN");
            ReturnNormalState();
            digitalWrite(RELAY_PIN, HIGH);
            has2wifi.Send((String)(const char*)my["device_name"], "device_state", "open");
            RoundNeoEffect(BLUE);
            GhostDoorOpen();
            has2wifi.Send((String)(const char*)my["device_name"], "device_state", "activate");
            AllNeoOn(YELLOW);
            SubSerialFlush();
            MainSerialFlush();
            delay(1000);
            has2wifi.Loop(DataChanged); //LOCK -> ACTIVATE 바뀐것을 업데이트 받기 위함
        }
    }
}
