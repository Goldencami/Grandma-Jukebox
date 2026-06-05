#include <SPI.h>
#include <TFT_eSPI.h>
#include <RTClib.h>
#include <FS.h>
#include <SD.h>
#include <Audio.h>

#define YELLOW_BTN 16
#define WHITE_BTN 17
#define RED_BTN 27
#define GREEN_BTN 32

// SD CARD VARIABLES
#define REASSIGN_PINS
#define SCK 18
#define MISO 19
#define MOSI 23
#define CS 5

// MAX98357 I2S VARIABLES
#define I2S_BCLK 14
#define I2S_LRC  13
#define I2S_DOUT 33

Audio audio;
String musicFiles[50];
String musicFolder = "";
int fileCount = 0;
int currentSong = 0;

// TFT VARIABLE
TFT_eSPI tft = TFT_eSPI(320, 240);

// RTC VARIABLE
RTC_DS3231 rtc;
String formattedTime = "";
String previousTime = "";
String dateArr[3]; // [month, day, year]
String hoursArr[3]; // [hour, minute, AM/PM]

// Convert month number to Spanish month name
const char* months[] = {"", "Enero", "Febrero", "Marzo", "Abril", "Mayo", "Junio", "Julio", "Agosto", "Septiembre", "Octubre", "Noviembre", "Diciembre"};

// STATE MACHINE VARIABLES
enum State {
  IDLE,
  SELECTION,
  PLAY,
  AUDIO,
  CONFIGS
};

State state = IDLE;
bool isIdleScreenDisplayed = false;
bool isMusicTypeDisplayed = false;
bool isConfigScreenDisplayed = false;
bool started = false;

// BMO COLOURS
uint16_t bmoGreen;

// RTC DELAY
unsigned int RTCdelay = 500;
unsigned long lastRTCdelay = millis();

// BUTTON STATES
unsigned int debounceDelayYellow = 50;
unsigned long lastYellowDebounce = millis();
byte yellowBtnState = LOW;

unsigned int debounceDelayWhite = 50;
unsigned long lastWhiteDebounce = millis();
byte whiteBtnState = LOW;

unsigned int debounceDelayRed = 50;
unsigned long lastRedDebounce = millis();
byte redBtnState = LOW;

unsigned int debounceDelayGreen = 50;
unsigned long lastGreenDebounce = millis();
byte greenBtnState = LOW;

// BMO FUNCTIONS
void drawSmile(int cx, int cy, int r) {
  for (int a = 20; a <= 160; a++) {
    float rad = a * DEG_TO_RAD;

    int x = cx + r * cos(rad);
    int y = cy + r * sin(rad);

    tft.fillCircle(x, y, 2, TFT_BLACK);
  }
}

void BMOidleFace() {
  int w = tft.width();
  int h = tft.height();

  // background
  tft.fillScreen(bmoGreen);

  // eye settings
  int eyeR = 7;
  int eyeY = h * 0.35;

  tft.fillCircle(w * 0.3, eyeY, eyeR, TFT_BLACK);
  tft.fillCircle(w * 0.7, eyeY, eyeR, TFT_BLACK);

  // smile :)
  drawSmile(w / 2, h * 0.43, 40);
}

void drawArrow(int width, int y) {
  int x = 20 + width + 5;
  // tft.fillTriangle(30, 40, 50, 30, 50, 50, TFT_BLACK);

  tft.fillTriangle(
    x, y - 8,
    x + 10, y - 15,
    x + 10, y + 2,
    TFT_BLACK
  );
}

void eraseArrow(int width, int y) {
  int x = 20 + width + 5; 

  tft.fillTriangle(
    x, y - 8,
    x + 10, y - 15,
    x + 10, y + 2,
    bmoGreen
  );
}

int musicIdx = 0;
void displayMusicTypes() {
  tft.fillScreen(bmoGreen);

  tft.setTextColor(TFT_BLACK);

  tft.setCursor(20, 40);
  tft.print("Reproductor de musica");

  tft.setCursor(20, 90);
  tft.print("Canciones");

  tft.setCursor(20, 120);
  tft.print("Himnos Biblicos");
}

void displayConfigs() {

}

void selectMusic() {
  if(handleWhiteBtn()) {
    musicIdx = (musicIdx + 1) % 2;
  }

  // Arrow pointing at index 0 (songs)
  if(musicIdx == 0) {
    eraseArrow(200, 120);
    drawArrow(135, 90);
  }
  else if(musicIdx == 1) { // Arrow pointing at index 1 (hymns)
    eraseArrow(135, 90);
    drawArrow(200, 120);
  }
}

// void playMusic(int index) {
//   if (index < 0 || index >= fileCount) {
//     return;
//   }

//   Serial.print("Playing: ");
//   Serial.println(musicFiles[index]);

//   audio.connecttoFS(SD, musicFiles[index].c_str());
// }

void playMusic(int index) {
  audio.stopSong();
  audio.connecttoFS(SD, "/himnos/Carlos Vives - La Gota Fria.wav");
}

void nextSong() {
  currentSong++;

  if(currentSong >= fileCount) {
    currentSong = 0;
  }

  playMusic(currentSong);
}

void previousSong() {
  currentSong--;

  if(currentSong < 0) {
    currentSong = fileCount - 1;
  }
  
  playMusic(currentSong);
}

// SD CARD FUNCTIONS
void readDirectory(File dir, int numTabs) {
  while (true) {
    File entry = dir.openNextFile();

    if (!entry) {
      break;
    }

    for (uint8_t i = 0; i < numTabs; i++) {
      Serial.print('\t');
    }
    Serial.print(entry.name());

    if (entry.isDirectory()) {
      Serial.println("/");
      readDirectory(entry, numTabs + 1);
    } else {
      Serial.print("\t\t");
      Serial.println(entry.size(), DEC);
    }
    entry.close();
  }
  dir.close();
}

void loadMusicList(File dir) {
  fileCount = 0;

  while (true) {
    File entry = dir.openNextFile();

    if (!entry) {
      break;
    }

    if (!entry.isDirectory()) {
      String name = entry.name();

      if (name.endsWith(".mp3") || name.endsWith(".MP3")) {
        musicFiles[fileCount++] = musicFolder + name;
      }
    }

    entry.close();
  }

  dir.close();
}

// RTC FUNCTIONS
void getDate(String *outputArray, DateTime now) {
  String yearStr = String(now.year());
  String dayStr = (now.day() < 10 ? "0" : "") + String(now.day());
  String monthStr = (now.month() < 10 ? "0" : "") + String(now.month());
  monthStr = months[now.month()];

  // [month, day, year]
  outputArray[0] = monthStr;
  outputArray[1] = dayStr;
  outputArray[2] = yearStr;
}

void getHour(String *outputArray, DateTime now) {
  // 12-hour clock
  int hour24 = now.hour();
  int hour12 = hour24 % 12;

  if (hour12 == 0) {
    hour12 = 12;
  }

  // [hour, minute, AM/PM]
  outputArray[0] = (hour12 < 10 ? "0" : "") + String(hour12);
  outputArray[1] = (now.minute() < 10 ? "0" : "") + String(now.minute());
  outputArray[2] = (hour24 < 12) ? "AM" : "PM";
}

void displayRTC() {
  // Get the current time from the RTC
  DateTime now = rtc.now();
    
  // Getting each time field in individual variables
  // And adding a leading zero when needed;
  getDate(dateArr, now);
  getHour(hoursArr, now);

  // Complete time string
  formattedTime = dateArr[0] + " " + dateArr[1] + ", " + dateArr[2] + "  —  " + hoursArr[0] + ":" + hoursArr[1] + " " + hoursArr[2];

  if (formattedTime != previousTime) {
    // Erase previous time
    tft.fillRect(0, 200, 320, 40, bmoGreen);
    previousTime = formattedTime;
  }

  tft.setTextColor(TFT_BLACK);
  tft.setCursor(12, 220);
  tft.print(formattedTime);
}

// BUTTON FUNCTIONS
bool handleYellowBtn() {
  unsigned long timeNow = millis();

  if(timeNow - lastYellowDebounce > debounceDelayYellow) {
    byte newBtnState = digitalRead(YELLOW_BTN);

    if(newBtnState != yellowBtnState) {
      yellowBtnState = newBtnState;
      lastYellowDebounce = timeNow;
      
      // because of pullups
      // pressed = LOW
      // released = HIGH
      if(yellowBtnState == LOW) {
        return true;
      }
    }
  }

  return false;
}

bool handleWhiteBtn() {
  unsigned long timeNow = millis();

  if(timeNow - lastWhiteDebounce > debounceDelayWhite) {
    byte newBtnState = digitalRead(WHITE_BTN);

    if(newBtnState != whiteBtnState) {
      whiteBtnState = newBtnState;
      lastWhiteDebounce = timeNow;

      // because of pullups
      // pressed = LOW
      // released = HIGH
      if(whiteBtnState == LOW) {
        return true;
      }
    }
  }

  return false;
}

bool handleRedBtn() {
  unsigned long timeNow = millis();

  if(timeNow - lastRedDebounce > debounceDelayRed) {
    byte newBtnState = digitalRead(RED_BTN);

    if(newBtnState != redBtnState) {
      redBtnState = newBtnState;
      lastRedDebounce = timeNow;

      // because of pullups
      // pressed = LOW
      // released = HIGH
      if(redBtnState == LOW) {
        return true;
      }
    }
  }

  return false;
}

bool handleGreenBtn() {
  unsigned long timeNow = millis();

  if(timeNow - lastGreenDebounce > debounceDelayGreen) {
    byte newBtnState = digitalRead(GREEN_BTN);

    if(newBtnState != greenBtnState) {
      greenBtnState = newBtnState;
      lastGreenDebounce = timeNow;

      // because of pullups
      // pressed = LOW
      // released = HIGH
      if(greenBtnState == LOW) {
        return true;
      }
    }
  }

  return false;
}

void setup() {
  Serial.begin(115200);

  pinMode(YELLOW_BTN, INPUT_PULLUP);
  pinMode(WHITE_BTN, INPUT_PULLUP);
  pinMode(RED_BTN, INPUT_PULLUP);
  pinMode(GREEN_BTN, INPUT_PULLUP);

  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  audio.setVolume(18);

  // FORCE CS pins HIGH BEFORE SPI START
  pinMode(CS, OUTPUT);
  digitalWrite(CS, HIGH);

  SPI.begin(SCK, MISO, MOSI);

  tft.init();
  pinMode(26, OUTPUT);
  digitalWrite(26, HIGH);

  tft.setRotation(0);
  tft.setFreeFont(&FreeSansBold12pt7b);
  bmoGreen = tft.color565(150, 240, 186);

  if (! rtc.begin()) {
    // Serial.println("Couldn't find RTC");
    while (1);
  }

  if (rtc.lostPower()) {
    // When time needs to be set on a new device, or after a power loss, the
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    //rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }

  // When time needs to be re-set on a previously configured device, the
  // following line sets the RTC to the date & time this sketch was compiled
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  // This line sets the RTC with an explicit date & time, for example to set
  // January 21, 2014 at 3am you would call:
  //rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));

  // Setup SD Card module
  if (!SD.begin(5, SPI, 4000000)) {
    // Serial.println("SD mount failed");
    while (1);
  }
  // else {
  //   Serial.println("SD OK");
  // }

  BMOidleFace();
  displayRTC();
}

void loop() {
  audio.loop();

  if(handleYellowBtn()) {
    state = SELECTION;
  }

  if(digitalRead(RED_BTN) == LOW && digitalRead(WHITE_BTN) == LOW) {
    state = CONFIGS;
  }

  unsigned long timeNow = millis();
  if(timeNow - lastRTCdelay > RTCdelay) {
    lastRTCdelay = timeNow;
    displayRTC();
  }

  if(state == IDLE) {
    if(!isIdleScreenDisplayed) {
      BMOidleFace();
      isIdleScreenDisplayed = true;
    }
    
  }
  else {
    isIdleScreenDisplayed = false;

    if(state == SELECTION) {
      if(!isMusicTypeDisplayed) {
        displayMusicTypes();
        isMusicTypeDisplayed = true;
      }

      selectMusic();

      if(handleRedBtn()) {
        musicIdx = 0;
        isMusicTypeDisplayed = false;
        state = IDLE;
        started = false;
      }

      if(handleGreenBtn()) {
        if(musicIdx == 0) {
          musicFolder = "/canciones/";
        }
        else if(musicIdx == 1) {
          musicFolder = "/himnos/";
        }
        
        state = AUDIO;
      }
    }
    if (state == AUDIO && !started) {

      File root = SD.open(musicFolder);

      if (root) {
        loadMusicList(root);
      }

      musicFolder = "";
      currentSong = 0;
      playMusic(currentSong);

      started = true;
      state = PLAY;
    }
    else if(state == PLAY) {
      if(handleGreenBtn()) {
        audio.pauseResume();   // PLAY/PAUSE
      }

      // if(handleWhiteBtn()) {
      //   nextSong();
      // }

      // if(handleYellowBtn()) {
      //   previousSong();
      // }

      if(handleRedBtn()) {
        musicIdx = 0;
        isMusicTypeDisplayed = false;
        state = IDLE;
        started = false;
      }
    }
    else if(state == CONFIGS) {
      if(!isConfigScreenDisplayed) {
        
      }
    }
  }
}