void DataChanged()
{
  static StaticJsonDocument<1000> cur;
  DebugSerial.println("[DC] device_state=" + (String)(const char*)my["device_state"] +
                      " game_state=" + (String)(const char*)my["game_state"] +
                      " cur_device=" + (String)(const char*)cur["device_state"] +
                      " loginDone=" + String(loginDone));
  if (my["brightness"].as<int>() != cur["brightness"].as<int>()) {
    UpdateBrightness();
  }
  if((String)(const char*)my["game_state"] != (String)(const char*)cur["game_state"]){
    if((String)(const char*)my["game_state"] == "setting"){
        strCurState = "setting";
        SettingFunc();
    }
    else if((String)(const char*)my["game_state"] == "ready"){
        strCurState = "ready";
        ReadyFunc();
    }
    else if((String)(const char*)my["game_state"] == "activate"){
        strCurState = "activate";
        ActivateFunc();
    }
  }
  if((String)(const char*)my["device_state"] != (String)(const char*)cur["device_state"]){
    String deviceState = (String)(const char*)my["device_state"];
    DebugSerial.println("[DC] device_state changed: " + (String)(const char*)cur["device_state"] + " -> " + deviceState);
    if(deviceState == "lock"){
      strCurState = deviceState;
      if(loginDone) QueuePendingDeviceState(deviceState);
      else ApplyDeviceState(deviceState);
    }
    else if(deviceState == "mo"){
        digitalWrite(RELAY_PIN, HIGH);
        delay(1000);
        digitalWrite(RELAY_PIN, LOW);
        has2wifi.Send((String)(const char*)my["device_name"], "device_state", (String)(const char*)cur["device_state"]);
    }
    else if(deviceState == "activate"){
        strCurState = deviceState;
        if(loginDone) QueuePendingDeviceState(deviceState);
        else ApplyDeviceState(deviceState);
    }
    else if(deviceState == "tagger"){
        ApplyDeviceState("tagger");
    }
    else if(deviceState == "github"){
        ota.check();
    }
  }
  if((String)(const char*)my["device_state"] == "debuff"){ 
    ApplyDeviceState("debuff");
  }
  GameSetting();
  NewbieModeSetting();
  {
    String _ds = (String)(const char*)my["device_state"];
    if (_ds == "lock" || _ds == "activate" || _ds == "debuff") strCurState = _ds;
  }
  DebugSerial.println("[DC] end -> strCurState=" + strCurState);
  cur = my;
}

void QueuePendingDeviceState(String deviceState) {
    pendingDeviceState = deviceState;
    pendingDeviceStateApply = true;
    DebugSerial.println("Queue Device State: " + deviceState);
}

void ApplyPendingDeviceState() {
    if (!pendingDeviceStateApply || loginDone) return;

    String deviceState = pendingDeviceState;
    pendingDeviceState = "";
    pendingDeviceStateApply = false;
    ApplyDeviceState(deviceState);
}

void ApplyDeviceState(String deviceState) {
    if(deviceState == "lock"){
      strCurState = "lock";
      digitalWrite(RELAY_PIN, LOW); // 봉쇄(tagger) 등 문이 열린 상태에서 lock으로 복귀 시 문 닫힘 보장
      AllNeoOn(GREEN);
    }
    else if(deviceState == "activate"){
        strCurState = "activate";
        ActivateFunc();
    }
    else if(deviceState == "debuff"){
        pendingDeviceState = "";
        pendingDeviceStateApply = false;
        strCurState = "debuff";
        ptrRfidFail = WaitFunc;
        AllNeoOn(PURPLE);
        DebugSerial.println("Debuff start");
        DebuffTimer.deleteTimer(debuffTimerId);
        debuffTimerId = DebuffTimer.setInterval(60000,DebuffTimerFunc);
        ReturnNormalState();
    }
    else if(deviceState == "tagger"){
        pendingDeviceState = "";
        pendingDeviceStateApply = false;
        strCurState = "tagger";
        AllNeoOn(PURPLE);
        digitalWrite(RELAY_PIN, HIGH);
        WifiTimer.deleteTimer(wifiTimerId);
        wifiTimerId = WifiTimer.setInterval(2000, WifiIntervalFunc);
        ptrCurrentMode = WhichTagged;
        ptrRfidMain = CommnunicationMainBeetle;
        ptrRfidSub = CommnunicationBeetle;
        ptrRfidMode = Login;
        ptrRfidFail = WaitFunc;
    }
}
void WaitFunc(){

}
void SettingFunc(void){
    DebugSerial.println("SETTING");
    DebuffTimer.deleteTimer(debuffTimerId);
    SubSerialTimer.deleteTimer(subSerialTimerId);
    GameTimer.deleteTimer(gameTimerId);
    WifiTimer.deleteTimer(wifiTimerId);                                          //게임 타이머 종료
    wifiTimerId = WifiTimer.setInterval(2000,WifiIntervalFunc);

    AllNeoOn(WHITE);
    digitalWrite(RELAY_PIN, HIGH);
    ptrCurrentMode = WaitFunc;
    ptrRfidMode = WaitRfid;
    GameSetting();
}
void ActivateFunc(void){
    DebugSerial.println("ACTIVATE");
    DebuffTimer.deleteTimer(debuffTimerId);
    SubSerialTimer.deleteTimer(subSerialTimerId);
    GameTimer.deleteTimer(gameTimerId);
    WifiTimer.deleteTimer(wifiTimerId);                                          //게임 타이머 종료
    wifiTimerId = WifiTimer.setInterval(2000,WifiIntervalFunc);

    AllNeoOn(YELLOW);
    digitalWrite(RELAY_PIN, LOW);
    ptrCurrentMode = WhichTagged;
    ptrRfidMode = Login;
    ptrRfidFail = WaitFunc;
    ptrRfidMain = CommnunicationMainBeetle;
    ptrRfidSub = CommnunicationBeetle;
    GameSetting();
    NewbieModeSetting();
    if ((String)(const char*)my["mode"] == "easy") {
        has2wifi.Send((String)(const char*)my["device_name"], "device_state", "lock");
        my["device_state"] = "lock";
        strCurState = "lock";
        AllNeoOn(GREEN);
    }
}
void ReadyFunc(void){
    DebugSerial.println("READY");
    DebuffTimer.deleteTimer(debuffTimerId);
    SubSerialTimer.deleteTimer(subSerialTimerId);
    GameTimer.deleteTimer(gameTimerId);
    WifiTimer.deleteTimer(wifiTimerId);                                          //게임 타이머 종료
    wifiTimerId = WifiTimer.setInterval(2000,WifiIntervalFunc);

    AllNeoOn(RED);
    digitalWrite(RELAY_PIN, HIGH);
    ptrCurrentMode = WaitFunc;
    ptrRfidMode = WaitRfid;
    GameSetting();
}
void GameSetting(){
    playerLockTime = (int)my["player_lock_time"];
    playerUnlockTime = (int)my["player_unlock_time"];
    taggerUnlockTime = (int)my["tagger_unlock_time"];
    ghostOpenTime = (int)my["ghost_open_time"];
}
void NewbieModeSetting() {
    if ((String)(const char*)my["mode"] == "easy" &&
        (String)(const char*)my["game_state"] == "activate" &&
        (String)(const char*)my["device_state"] != "tagger") {  // tagger일 때 NewbieLogin으로 덮어쓰기 방지
        ptrRfidMode = Login;
    }
}
