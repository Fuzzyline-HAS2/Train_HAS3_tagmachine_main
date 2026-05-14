void CheckingPlayers(String tagUser) // 어떤 카드가 들어왔는지 확인용
{
  DebugSerial.println("tag_user_data : " +
                 tagUser); // 1. 태그한 플레이어의 역할과 생명칩갯수,
                           // 최대생명칩갯수 등 읽어오기
  if (tagUser == "MMMM") { // 스태프카드
    digitalWrite(RELAY_PIN, HIGH);
    delay(500);
    digitalWrite(RELAY_PIN, LOW);
  } else {
    if (loginDone == false) // 로그인할때만 체크함
    {
      has2wifi.Receive(tagUser); // 2. 술래인지, 플레이어인지 구분
      if ((String)(const char *)tag["role"] ==
          "player") { // 3. 태그한 사용자가 플레이어고
        DebugSerial.println("Player Tagged");
        loginRole = 'P';
        if (ptrRfidMode != nullptr) ptrRfidMode('P');
      } else if ((String)(const char *)tag["role"] ==
                 "tagger") { // 3. 태그한 사용자가 플레이어고
        DebugSerial.println("Tagger Tagged");
        loginRole = 'T';
        if (ptrRfidMode != nullptr) ptrRfidMode('T');
      } else if ((String)(const char *)tag["role"] == "ghost" ||
                 (String)(const char *)tag["role"] == "revival") { // 3. 태그한
        DebugSerial.println("Ghost Tagged");
        loginRole = 'G';
        if (ptrRfidMode != nullptr) ptrRfidMode('G');
      } else {
        DebugSerial.println("Wrong TAG");
      }
    } else {
      if (strLastTagUser == tagUser) { // Login 진행한 태그와 같은지 확인
        DebugSerial.println("LoginRole: " + String(loginRole));
        if (ptrRfidMode != nullptr) ptrRfidMode(loginRole);
      } else {
        DebugSerial.println("Different TAG deteced");
        if (ptrRfidFail != nullptr) ptrRfidFail();
      }
    }
    strLastTagUser = tagUser;
  }
}

void Login(char role) {
  DebugSerial.println("LOGIN");
  loginDone = true;
  lightColor(pixels[ROUND], color[BLACK]);
  lightColor(pixels[ROUND_SUB], color[BLACK]);

  GameTimer.deleteTimer(gameTimerId);
  gameTimerId = GameTimer.setInterval(1000, GameTimerFunc);
  WifiTimer.deleteTimer(wifiTimerId);
  gameTimerCnt = 0;
  if (mainRfidTagged == true) {
    ptrRfidMain = CommnunicationMainBeetle;
    ptrRfidSub = WaitFunc;
    LoginTimerSelector(role);
  } else if (mainRfidTagged == false) {
    ptrRfidMain = WaitFunc;
    ptrRfidSub = CommnunicationBeetle;
    LoginTimerSelector(role);
  }
  if (loginDone && ptrGameTimer != nullptr) ptrGameTimer();
}

void WaitRfid(char role) { DebugSerial.println("WAIT RFID"); }
void LoginTimerSelector(char role) {
  DebugSerial.println("LoginTimerSelector");
  bool isNewbie = ((String)(const char*)my["mode"] == "easy" &&
                   (String)(const char*)my["device_state"] == "lock");
  if ((String)(const char *)my["device_state"] == "lock") {
    if (role == 'P') {
      ptrGameTimer = isNewbie ? NewbiePlayerUnlockTimerFunc : PlayerUnlockTimerFunc;
      ptrRfidFail  = isNewbie ? NewbieUnlockFail : UnlockFail;
      ptrRfidMode  = WaitRfid;
    } else if (role == 'G') {
      ptrGameTimer = isNewbie ? NewbieGhostUnlockTimerFunc : GhostUnlockTimerFunc;
      ptrRfidFail  = GhostOpenFailLock;
      ptrRfidMode  = WaitRfid;
    } else if (role == 'T') {
      ptrGameTimer = isNewbie ? NewbieTaggerUnlockTimerFunc : TaggerUnlockTimerFunc;
      ptrRfidFail  = isNewbie ? NewbieTaggerFail : UnlockFail;
      ptrRfidMode  = WaitRfid;
    }
  } else if ((String)(const char *)my["device_state"] == "debuff") {
    AllNeoOn(PURPLE);
    if (role == 'P') {
      NeoBlink(ROUND, RED, 2, 400);
      AllNeoOn(PURPLE);
      ReturnNormalState();
    } else if (role == 'G') {
      NeoBlink(ROUND, RED, 2, 400);
      AllNeoOn(PURPLE);
      ReturnNormalState();
    } else if (role == 'T') {
      Mp3PlayLargeFolder(1, VD1);
      DebugSerial.println("Tagger Door Open");
      digitalWrite(RELAY_PIN, HIGH);
      RoundNeoEffect(PURPLE);
      AllNeoOn(PURPLE);
      RoundNeoEffectDown(BLACK);
      DoorOpen();
      ReturnNormalState();
      AllNeoOn(PURPLE);
    }
  } else {
    if (role == 'P') {
      DebugSerial.println("LoginTimerSelector PlayerSelected");
      ptrGameTimer = PlayerLockTimerFunc;
      ptrRfidFail  = LockFail;
      ptrRfidMode  = WaitRfid;
    } else if (role == 'G') {
      ptrGameTimer = GhostLockTimerFunc;
      ptrRfidFail  = GhostOpenFailUnlock;
      ptrRfidMode  = WaitRfid;
    } else if (role == 'T') {
      Mp3PlayLargeFolder(1, VD1);
      DebugSerial.println("Tagger Door Open");
      digitalWrite(RELAY_PIN, HIGH);
      has2wifi.Send((String)(const char *)my["device_name"], "device_state", "open");
      RoundNeoEffect(PURPLE);
      AllNeoOn(PURPLE);
      DoorOpen();
      AllNeoOn(YELLOW);
      ReturnNormalState();
    }
  }
}
void NewbieTaggerFail() {
  has2wifi.ReceiveMine();
  DataChanged();
  if (strCurState != "lock") {
    DebugSerial.println("debuff on");
    CancelTagProgress();
  } else {
    Mp3PlayLargeFolder(1, VD6);
    DebugSerial.println("Unlock Fail Door Shut");
    NeoBlink(ROUND, RED, 5, 500);
    AllNeoOn(GREEN);
    ReturnNormalState();
    ptrRfidMode = Login;
  }
}
void LockFail() {
  has2wifi.ReceiveMine();
  DataChanged();
  // DebugSerial.println("strCurState:" + String(strCurState));
  if (strCurState != "activate") {
    DebugSerial.println("debuff on");
    CancelTagProgress();
  } else {
    Mp3PlayLargeFolder(1, VD1);
    DebugSerial.println("Lock Fail Door Open");
    digitalWrite(RELAY_PIN, HIGH);
    has2wifi.Send((String)(const char *)my["device_name"], "device_state",
                  "open");
    RoundNeoEffect(YELLOW);
    AllNeoOn(YELLOW);
    DoorOpen();
    ReturnNormalState();
  }
}
void UnlockFail() {
  has2wifi.ReceiveMine();
  DataChanged();
  // DebugSerial.println("strCurState:" + String(strCurState));
  if (strCurState != "lock") {
    DebugSerial.println("debuff on");
    CancelTagProgress();
  } else {
    Mp3PlayLargeFolder(1, VD6);
    DebugSerial.println("Unlock Fail Door Shut");
    NeoBlink(ROUND, RED, 5, 500);
    AllNeoOn(GREEN);
    ReturnNormalState();
  }
}

void NewbieUnlockFail() {
  has2wifi.ReceiveMine();
  DataChanged();
  if (strCurState != "lock") {
    DebugSerial.println("debuff on");
    CancelTagProgress();
  } else {
    Mp3PlayLargeFolder(1, VD6);
    DebugSerial.println("Unlock Fail Door Shut (Newbie)");
    NeoBlink(ROUND, RED, 5, 500);
    AllNeoOn(GREEN);
    ReturnNormalState();
  }
}

void GhostOpenFailUnlock() {
  // has2wifi.ReceiveMine();
  // DataChanged();
  // // DebugSerial.println("strCurState:" + String(strCurState));
  // if(strCurState != "lock"){
  //     DebugSerial.println("debuff on");
  // }
  // else{
  Mp3PlayLargeFolder(1, VD6);
  DebugSerial.println("Ghost Door OpenFail");
  NeoBlink(ROUND, RED, 5, 500);
  AllNeoOn(YELLOW);
  ReturnNormalState();
  // // }
}
void GhostOpenFailLock() {
  // has2wifi.ReceiveMine();
  // DataChanged();
  // // DebugSerial.println("strCurState:" + String(strCurState));
  // if(strCurState != "activate"){
  //     DebugSerial.println("debuff on");
  // }
  // else{
  Mp3PlayLargeFolder(1, VD6);
  DebugSerial.println("Unlock Fail Door Shut");
  NeoBlink(ROUND, RED, 5, 500);
  AllNeoOn(GREEN);
  ReturnNormalState();
  // }
}
