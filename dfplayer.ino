//****************************************mp3_setup()****************************************************************
bool dfPlayerReady = false;

void Mp3_Setup(){
  pinMode(DFPLAYER_RX_PIN, INPUT); // GPIO 39은 input-only, INPUT_PULLUP 미지원 → SoftwareSerial.begin() 전에 선점
  MP3Serial.begin(9600);
  DebugSerial.println("DFRobot DFPlayer Mini Demo");
  DebugSerial.println("Initializing DFPlayer ... (May take 3~5 seconds)");
  myDFPlayer.setTimeOut(1000);

  // begin()은 SoftwareSerial 환경에서 모듈이 정상이어도 false를 반환하는 경우가
  // 많다(타이밍 이슈). 과거 정상 동작하던 펌웨어처럼 begin() 실패를 치명적으로
  // 보지 않고 몇 번 재시도한 뒤, 결과와 무관하게 재생을 시도한다.
  // 예전에 begin() 실패 시 return 해버려 dfPlayerReady가 false로 굳으면서
  // 모든 효과음이 안 나오던 문제를 수정.
  bool ok = false;
  for (int i = 0; i < 3 && !ok; i++) {
    ok = myDFPlayer.begin(MP3Serial, false);
    if (!ok) {
      DebugSerial.println("DFPlayer begin() failed, retrying...");
      delay(200);
    }
  }
  if (ok) DebugSerial.println(F("DFPlayer Mini online."));
  else    DebugSerial.println(F("DFPlayer begin() reported failure - continuing anyway."));

  dfPlayerReady = true;  // begin() 결과와 무관하게 재생 시도(과거 동작 복원)
  myDFPlayer.setTimeOut(500);
  myDFPlayer.volume(30);
  myDFPlayer.EQ(DFPLAYER_EQ_NORMAL);
  myDFPlayer.outputDevice(DFPLAYER_DEVICE_SD);
}//void MP3_SETUP


void Mp3PlayLargeFolder(uint8_t folder_number, uint16_t file_number)
{
  if(!dfPlayerReady) return;  // DFPlayer 미초기화 시 스킵
  const char* lang = (const char *)shift_machine["selected_language"];
  if (lang != nullptr && String(lang) == "EN")
  {
    folder_number = 2 + folder_number;
  }
  // 이전에는 myDFPlayer.available()(=DFPlayer가 알림 프레임을 보냈는지)로
  // 재생 여부를 게이트했는데, available()은 "재생 준비"가 아니라 수신 프레임
  // 존재 여부라 대부분 false였다. static 카운터가 3에 도달하면 재생이 영구히
  // 막혀, 트랙 재생 중 연속 호출되는 알람음 등이 안 나오는 문제가 있었다.
  // 표준 사용법대로 항상 재생 명령을 전송한다.
  myDFPlayer.playLargeFolder(folder_number, file_number);
}