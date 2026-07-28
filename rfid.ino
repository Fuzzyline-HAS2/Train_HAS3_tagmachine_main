char FixedRoleFromTag(String tagUser)
{
  if (tagUser == "G2P1") return 'T';
  if (tagUser == "G2P2") return 'P';
  if (tagUser == "G9P1") return 'T';
  if (tagUser == "G9P2") return 'G';
  if (tagUser == "G9P3" || tagUser == "G9P4" ||
      tagUser == "G9P5" || tagUser == "G9P6" ||
      tagUser == "G9P7" || tagUser == "G9P8") return 'P';
  return '\0';
}

void CheckingPlayers(String tagUser)
{
  // 태그 인식 2단계 흐름:
  //   1단계 (loginDone=false): 처음 태그 → 고정 태그 목록에서 역할 확인 후 Login() 호출
  //   2단계 (loginDone=true) : 같은 카드 재태그 → 타이머 진행 중임을 확인
  //                            다른 카드가 태그되면 → ptrRfidFail() 호출 (실패 처리)
  DebugSerial.println("tag_user_data : " + tagUser);
  if (tagUser == "MMMM") { // 스태프카드: 릴레이 펄스로 수동 열림
    digitalWrite(RELAY_PIN, HIGH);
    delay(500);
    digitalWrite(RELAY_PIN, LOW);
  } else {
    if (loginDone == false)
    {
      char fixedRole = FixedRoleFromTag(tagUser);
      if (fixedRole == '\0') {
        DebugSerial.println("Unknown fixed tag ignored");
        return;
      }

      loginRole = fixedRole;
      if (fixedRole == 'P') DebugSerial.println("Fixed Player Tagged");
      else if (fixedRole == 'T') DebugSerial.println("Fixed Tagger Tagged");
      else if (fixedRole == 'G') DebugSerial.println("Fixed Ghost Tagged");
      if (ptrRfidMode != nullptr) ptrRfidMode(fixedRole);
    } else {
      if (strLastTagUser == tagUser) { // 같은 카드 재태그 → 타이머 계속 진행
        DebugSerial.println("LoginRole: " + String(loginRole));
        tagMismatchCount = 0;  // 정상 읽기 → 불일치 카운터 리셋
        if (ptrRfidMode != nullptr) ptrRfidMode(loginRole);
      } else {                         // 다른 카드 감지 → 실패 처리
        // 단발성 불일치는 시리얼 손상(예: G1P4→1P4) 오판일 가능성이 높으므로 무시.
        // 3회 연속 불일치일 때만 실제 다른 카드로 판단해 fail 처리.
        tagMismatchCount++;
        if (tagMismatchCount >= 3) {
            Serial.println("Different TAG deteced (x3)");
            tagMismatchCount = 0;
            if (ptrRfidFail != nullptr) ptrRfidFail();
        } else {
            Serial.println("Mismatch ignored (transient)");
        }
      }
    }
    strLastTagUser = tagUser;
  }
}

void Login(char role) {
  // 첫 번째 태그 성공 시 호출. loginDone=true로 이후 태그를 2단계로 전환.
  // 1초 간격 GameTimer를 시작하고 LoginTimerSelector()로 역할별 타이머 함수를 지정.
  // WifiTimer를 끄는 이유: 게임 진행 중 WiFi 수신 인터럽트로 상태가 흔들리는 것 방지.
  tagMismatchCount = 0;  // 새 로그인 시작 시 불일치 카운터 초기화
  DebugSerial.println("LOGIN");
  loginDone = true;
  lightColor(pixels[ROUND], color[BLACK]);
  lightColor(pixels[ROUND_SUB], color[BLACK]);

  GameTimer.deleteTimer(gameTimerId);
  gameTimerId = GameTimer.setInterval(1000, GameTimerFunc);
  WifiTimer.deleteTimer(wifiTimerId);
  gameTimerCnt = 0;

  // 어느 리더에서 태그됐는지에 따라 반대쪽 리더를 대기(WaitFunc)로 전환.
  // 두 리더가 동시에 다른 카드를 처리하지 않도록 하기 위함.
  if (mainRfidTagged) {
    ptrRfidMain = CommnunicationMainBeetle;
    ptrRfidSub = WaitFunc;
    LoginTimerSelector(role);
  } else {
    ptrRfidMain = WaitFunc;
    ptrRfidSub = CommnunicationBeetle;
    LoginTimerSelector(role);
  }
  if (loginDone && ptrGameTimer != nullptr) ptrGameTimer(); // 첫 틱 즉시 실행
}

void WaitRfid(char role) { DebugSerial.println("WAIT RFID"); }

void LoginTimerSelector(char role) {
  // device_state와 뉴비 여부에 따라 역할별 타이머 함수 / 실패 함수를 결정.
  //
  // isNewbie: mode=="easy" && device_state=="lock" 일 때만 true.
  //   → 뉴비 타이머는 성공 후 lock으로 복귀하는 사이클을 처리함.
  //
  // device_state 분기:
  //   "lock"   → 생존자/유령은 잠금해제 시도, 술래는 침입 시도
  //   "debuff" → 생존자/유령은 진입 불가(블링크 후 복귀), 술래는 즉시 통과
  //   그 외(activate) → 생존자/유령은 잠금 시도, 술래는 즉시 통과
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
      ptrRfidFail  = GhostOpenFailLock; // 일반/뉴비 동일
      ptrRfidMode  = WaitRfid;
    } else if (role == 'T') {
      ptrGameTimer = isNewbie ? NewbieTaggerUnlockTimerFunc : TaggerUnlockTimerFunc;
      ptrRfidFail  = isNewbie ? NewbieTaggerFail : UnlockFail;
      ptrRfidMode  = WaitRfid;
    }
  } else if ((String)(const char *)my["device_state"] == "tagger") {
    if (role == 'P' || role == 'G') {
      NeoBlink(ROUND, RED, 3, 400);
    }
    AllNeoOn(PURPLE);
    ReturnNormalState();
    // RELAY_PIN HIGH 유지 — tagger 상태 동안 도어는 계속 열려 있음
  } else if ((String)(const char *)my["device_state"] == "debuff") {
    AllNeoOn(PURPLE);
    if (role == 'P') {
      NeoBlink(ROUND, RED, 2, 400); // 진입 거부 연출
      AllNeoOn(PURPLE);
      ReturnNormalState();
    } else if (role == 'G') {
      NeoBlink(ROUND, RED, 2, 400);
      AllNeoOn(PURPLE);
      ReturnNormalState();
    } else if (role == 'T') {
      // 술래는 debuff 상태에서도 즉시 통과 (타이머 없음)
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
  } else { // activate 상태
    if (role == 'P') {
      DebugSerial.println("LoginTimerSelector PlayerSelected");
      ptrGameTimer = PlayerLockTimerFunc; // 문 잠그기 시도
      ptrRfidFail  = LockFail;
      ptrRfidMode  = WaitRfid;
    } else if (role == 'G') {
      ptrGameTimer = GhostLockTimerFunc;
      ptrRfidFail  = GhostOpenFailUnlock;
      ptrRfidMode  = WaitRfid;
    } else if (role == 'T') {
      // 술래는 activate 상태에서도 즉시 통과 (타이머 없음)
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
  // 뉴비 술래가 두 번째 태그에서 다른 카드를 찍거나 타임아웃됐을 때 호출.
  // debuff로 상태가 바뀌어 있으면 태그 진행 전체를 취소하고, 아니면 실패 연출.
  has2wifi.ReceiveMine();
  DataChanged();
  if (strCurState != "lock") {
    DebugSerial.println("debuff on");
    CancelTagProgress();
  } else {
    Mp3PlayLargeFolder(1, VD6);
    DebugSerial.println("Unlock Fail Door Shut");
    NeoBlink(ROUND, RED, 2, 150);
    AllNeoOn(GREEN);
    ReturnNormalState();
    ptrRfidMode = Login;
  }
}

void LockFail() {
  // 생존자가 잠금 시도 중(activate 상태) 다른 카드가 태그됨.
  // 술래가 통과한 것으로 간주하고 문을 열어줌.
  has2wifi.ReceiveMine();
  DataChanged();
  if (strCurState != "activate") {
    DebugSerial.println("debuff on");
    CancelTagProgress();
  } else {
    Mp3PlayLargeFolder(1, VD1);
    DebugSerial.println("Lock Fail Door Open");
    digitalWrite(RELAY_PIN, HIGH);
    has2wifi.Send((String)(const char *)my["device_name"], "device_state", "open");
    RoundNeoEffect(YELLOW);
    AllNeoOn(YELLOW);
    DoorOpen();
    ReturnNormalState();
  }
}

void UnlockFail() {
  // 생존자/술래가 잠금해제 시도 중 다른 카드 태그 또는 타임아웃 → 실패 연출 후 복귀.
  has2wifi.ReceiveMine();
  DataChanged();
  if (strCurState != "lock") {
    DebugSerial.println("debuff on");
    CancelTagProgress();
  } else {
    Mp3PlayLargeFolder(1, VD6);
    DebugSerial.println("Unlock Fail Door Shut");
    NeoBlink(ROUND, RED, 2, 150);
    AllNeoOn(GREEN);
    ReturnNormalState();
  }
}

void NewbieUnlockFail() {
  // 뉴비 모드 생존자 전용 실패 함수.
  // 동작은 UnlockFail과 동일하지만 isNewbie 분기에서 명시적으로 구분하기 위해 분리.
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
  // 유령이 activate 상태에서 잠금 시도 실패. YELLOW(activate 색상) 유지하며 복귀.
  Mp3PlayLargeFolder(1, VD6);
  DebugSerial.println("Ghost Door OpenFail");
  NeoBlink(ROUND, RED, 2, 150);
  AllNeoOn(YELLOW);
  ReturnNormalState();
}

void GhostOpenFailLock() {
  // 유령이 lock 상태에서 잠금해제 시도 실패. GREEN(lock 색상) 유지하며 복귀.
  Mp3PlayLargeFolder(1, VD6);
  DebugSerial.println("Unlock Fail Door Shut");
  NeoBlink(ROUND, RED, 2, 150);
  AllNeoOn(GREEN);
  ReturnNormalState();
}
