void WhichTagged()
{
    if (ptrRfidMain != nullptr) ptrRfidMain();
    if (ptrRfidSub  != nullptr) ptrRfidSub();
}

void DoorOpen(){
    digitalWrite(RELAY_PIN, LOW);
    if(strCurState == "debuff"){  //debuff 인경우위해서?
        DebugSerial.println("DEBUFF OPEN");
    }
    else{
        has2wifi.Send((String)(const char*)my["device_name"], "device_state", "activate");
        RoundNeoEffectDown(BLACK);
        has2wifi.Loop(DataChanged); //LOCK -> ACTIVATE 바뀐것을 업데이트 받기 위함
        AllNeoOn(YELLOW);
    }
    digitalWrite(RELAY_PIN, LOW);
}

void GhostDoorOpen(){
    digitalWrite(RELAY_PIN, LOW);
    RoundNeoEffectDown(BLACK);
    // delay(3000);
}

// ======================== NEWBIE MODE ========================

void NewbieOpenBody(int neoColor) {
    ReturnNormalState();
    ptrRfidMode = Login;
    Mp3PlayLargeFolder(1, VD1);
    digitalWrite(RELAY_PIN, HIGH);
    has2wifi.Send((String)(const char*)my["device_name"], "device_state", "open");
    RoundNeoEffect(neoColor);
    GhostDoorOpen();
    has2wifi.Send((String)(const char*)my["device_name"], "device_state", "lock");
    AllNeoOn(GREEN);
    SubSerialFlush();
    MainSerialFlush();
    delay(1000);
    has2wifi.Loop(DataChanged);
}

void NewbiePlayerOpen() { NewbieOpenBody(GREEN); }
void NewbieGhostOpen()  { NewbieOpenBody(BLUE);  }


