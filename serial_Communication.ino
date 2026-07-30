void CommnunicationBeetle(){
  // Serial.println("READ");
  if(toSubSerial.available() > 0){
    String command = toSubSerial.readStringUntil('\n'); //추가 시리얼의 값을 수신하여 String으로 저장
    DebugSerial.println(command);
    // Serial.println("received:" + String(command[0])); //기본 시리얼에 추가 시리얼 내용을 출력

    if(command[0] == 'W'){
      DebugSerial.println("Beetle Init Success");
      toSubSerial.println("W");
    }
    else if(command[0] == 'R'){
      DebugSerial.println("Beetle Reset Success");
    }
    else if(command[0] == 'M'){
      digitalWrite(RELAY_PIN, HIGH);
      delay(500);
      digitalWrite(RELAY_PIN, LOW);
    }
    else if(command.length() >= 4){   // NFC 태그 데이터 (4자 이상이면 태그로 처리)
      mainRfidTagged = false;
      DebugSerial.println("Sub Beetle Tag: " + command.substring(0,4));
      SendBeetleTag(BEETLE_SUB, "sub_beetle_player", command.substring(0,4));
      CheckingPlayers(command.substring(0,4));
      if(SubSerialTimerStart == true){
        SubSerialTimer.deleteTimer(subSerialTimerId);
        SubSerialTimerStart = false;
      }
      subSerialTimerId = SubSerialTimer.setInterval(1000,SubSerialTimerFunc);
      SubSerialTimerStart = true;
    }
    while(toSubSerial.available())
      toSubSerial.read();
  }
}

void SendBeetleTag(int idx, const char *field, const String &tagUser){
  if(tagUser == "MMMM") return;   // 스태프 카드는 서버 전송 제외
  unsigned long now = millis();
  if(beetleSendLastMs[idx] != 0 && now - beetleSendLastMs[idx] < 1000) return;
  beetleSendLastMs[idx] = now;
  has2wifi.Send((String)(const char *)my["device_name"], field, tagUser);
}

void SubSerialFlush(){
  while(toSubSerial.available())      //시리얼 통신 버퍼 flush
      toSubSerial.read();
}

void MainSerialFlush(){
  while(toMainSerial.available())     //Main Beetle 시리얼 통신 버퍼 flush
      toMainSerial.read();
}

/**
 * @brief Main Beetle 시리얼 통신 처리 (Sub Beetle의 CommnunicationBeetle과 동일 구조)
 * WifiIntervalFunc에서 호출되어 모든 게임 상태에서 W 핸드셰이크 처리
 * activate 상태에서는 WhichTagged → ptrRfidMain에서도 호출되어 태그 데이터 처리
 */
void CommnunicationMainBeetle(){
  if(toMainSerial.available() > 0){
    String command = toMainSerial.readStringUntil('\n');
    DebugSerial.println("Main Beetle: " + command);

    if(command[0] == 'W'){
      DebugSerial.println("Main Beetle Init Success");
      toMainSerial.println("W");
    }
    else if(command[0] == 'R'){
      DebugSerial.println("Main Beetle Reset Success");
    }
    else if(command[0] == 'M'){
      digitalWrite(RELAY_PIN, HIGH);
      delay(500);
      digitalWrite(RELAY_PIN, LOW);
    }
    else if(command.length() >= 4){   // NFC 태그 데이터 (4자 이상)
      mainRfidTagged = true;
      DebugSerial.println("TAGGGED (Main Beetle)");
      SendBeetleTag(BEETLE_MAIN, "main_beetle_player", command.substring(0, 4));
      CheckingPlayers(command.substring(0, 4));
      if(SubSerialTimerStart == true){
        SubSerialTimer.deleteTimer(subSerialTimerId);
        SubSerialTimerStart = false;
      }
      subSerialTimerId = SubSerialTimer.setInterval(1000, SubSerialTimerFunc);
      SubSerialTimerStart = true;
    }
    while(toMainSerial.available())
      toMainSerial.read();
  }
}
