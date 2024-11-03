/*
 * FM Radio Controller for M5Cardputer
 * Author: [Your Name or GitHub Username]
 * License: MIT
 * 
 * This software is provided "as is," without any warranty, express or implied.
 * Users assume all responsibility for using and modifying this code.
 */

#include <Wire.h>
#include <SD.h>
#include <TEA5767.h>
#include <M5Cardputer.h>

TEA5767 radio = TEA5767();

enum RadioMode { SCANNED, SAVED };
const RadioMode startupMode = SCANNED;
RadioMode currentMode = startupMode;

const int maxStationNameLength = 20;
double frequency = 87.50;
double topFrequency = 108.00;
double intervalFreq = 0.1;
int stationIndex = 0;
int stationCount = 0;
double stationFrequencies[100];
String stationNames[100];

short minSignalLevel = 9;
unsigned long scanDelay = 100;
unsigned long lastScanTime = 0;
unsigned long lastKeyPressTime = 0;
const unsigned long debounceDelay = 200;
const char* stationsFile = "/RadioSta.txt";

bool displaySignalStrength = true;
bool displayStereoStatus = true;
unsigned long statusUpdateInterval = 120000;
unsigned long lastStatusUpdate = 0;

bool showMessage = false;

void setup() {
  Wire.begin(2, 1);

  auto cfg = M5.config();
  M5Cardputer.begin(cfg, true);
  M5Cardputer.Display.setRotation(1);
  M5Cardputer.Display.clear();
  M5Cardputer.Display.setTextSize(2);

  if (!SD.begin()) {
    M5Cardputer.Display.println("SD Card initialization failed!");
    return;
  }

  displayMode();
  if (currentMode == SCANNED) {
    prescanStations();
  } else {
    loadSavedStations();
  }

  if (stationCount > 0) {
    stationIndex = 0;
    frequency = stationFrequencies[stationIndex];
    setStation();
  } else {
    M5Cardputer.Display.setCursor(0, 20);
    M5Cardputer.Display.print("No stations found");
  }
}

void loop() {
  M5Cardputer.update();

  unsigned long currentMillis = millis();
  if (currentMillis - lastKeyPressTime >= debounceDelay) {
    if (M5Cardputer.Keyboard.isChange()) {
      lastKeyPressTime = currentMillis;

      if (M5Cardputer.Keyboard.isKeyPressed('m')) {
        cycleMode();
      } else if (M5Cardputer.Keyboard.isKeyPressed('/')) {
        handleKeyPress('/');
      } else if (M5Cardputer.Keyboard.isKeyPressed(',')) {
        handleKeyPress(',');
      } else if (M5Cardputer.Keyboard.isKeyPressed(0x28)) {  // OK/Enter key
        if (showMessage) {
          clearMessage();
        } else {
          setStation();
        }
      } else if (M5Cardputer.Keyboard.isKeyPressed('s') && currentMode == SCANNED) {
        prescanStations();
      } else if (M5Cardputer.Keyboard.isKeyPressed('.')) {  // Fine-tune down
        Serial.println("Fine-tune down");
        adjustFrequency(-0.1);
      } else if (M5Cardputer.Keyboard.isKeyPressed(';')) {  // Fine-tune up
        Serial.println("Fine-tune up");
        adjustFrequency(0.1);
      }
    }
  }

  if ((currentMillis - lastStatusUpdate >= statusUpdateInterval) && (displaySignalStrength || displayStereoStatus)) {
    lastStatusUpdate = currentMillis;
    displayStationInfo();
  }

  delay(200);
}

void adjustFrequency(double increment) {
  frequency += increment;

  // Ensure the frequency is within the FM band
  if (frequency < 87.5) frequency = 87.5;
  if (frequency > 108.0) frequency = 108.0;

  Serial.print("Adjusted frequency: ");
  Serial.println(frequency);

  // Set the adjusted frequency on the TEA5767
  radio.setFrequency(frequency);
  displayStationInfo();
}

void clearMessage() {
  M5Cardputer.Display.fillRect(0, 120, 320, 20, BLACK);
  showMessage = false;
}

void displayMode() {
  M5Cardputer.Display.setCursor(0, 0);
  M5Cardputer.Display.fillRect(0, 0, 320, 20, BLACK);
  M5Cardputer.Display.setTextSize(2);

  switch (currentMode) {
    case SCANNED:
      M5Cardputer.Display.print("Scanned Stations");
      break;
    case SAVED:
      M5Cardputer.Display.print("Saved Stations");
      break;
  }
}

void cycleMode() {
  currentMode = (currentMode == SCANNED) ? SAVED : SCANNED;
  displayMode();

  if (currentMode == SCANNED) {
    prescanStations();
  } else {
    loadSavedStations();
  }
  displayStationInfo();
}

void loadSavedStations() {
  stationCount = 0;
  File file = SD.open(stationsFile);
  if (!file) return;

  while (file.available() && stationCount < 100) {
    String line = file.readStringUntil('\n');
    int commaIndex = line.indexOf(',');
    if (commaIndex > 0) {
      frequency = line.substring(0, commaIndex).toFloat();
      String name = line.substring(commaIndex + 1);
      if (frequency >= 87.5 && frequency <= 108.0) {
        stationFrequencies[stationCount] = frequency;
        stationNames[stationCount] = name.length() > maxStationNameLength ? name.substring(0, maxStationNameLength) : name;
        stationCount++;
      }
    }
  }
  file.close();
  stationIndex = 0;
  frequency = stationFrequencies[stationIndex];
}

void prescanStations() {
  M5Cardputer.Display.fillRect(0, 20, 320, 200, BLACK);
  M5Cardputer.Display.setCursor(0, 20);
  M5Cardputer.Display.print("Scanning...");

  frequency = 87.50;
  stationCount = 0;

  while (frequency <= topFrequency) {
    unsigned long currentMillis = millis();
    if (currentMillis - lastScanTime >= scanDelay) {
      lastScanTime = currentMillis;
      radio.setFrequency(frequency);

      if (radio.getSignalLevel() >= minSignalLevel) {
        stationFrequencies[stationCount] = frequency;
        stationCount++;
        if (stationCount >= 100) break;
      }
      frequency += intervalFreq;
    }
  }

  M5Cardputer.Display.fillRect(0, 20, 320, 200, BLACK);
  M5Cardputer.Display.setCursor(0, 20);
  M5Cardputer.Display.printf("Found %d stations\n", stationCount);
  delay(1000);
}

void handleKeyPress(char key) {
  if (stationCount > 0) {
    if (key == '/') {
      stationIndex = (stationIndex + 1) % stationCount;
      frequency = stationFrequencies[stationIndex];
      Serial.print("Next station: ");
      Serial.println(frequency);
    } else if (key == ',') {
      stationIndex = (stationIndex - 1 + stationCount) % stationCount;
      frequency = stationFrequencies[stationIndex];
      Serial.print("Previous station: ");
      Serial.println(frequency);
    }
    displayStationInfo();
  }
}

void setStation() {
  if (stationCount > 0) {
    radio.setFrequency(stationFrequencies[stationIndex]);
    displayStationInfo();
    M5Cardputer.Display.setCursor(0, 120);
    M5Cardputer.Display.fillRect(0, 120, 320, 20, BLACK);

    if (currentMode == SAVED) {
      M5Cardputer.Display.print(stationNames[stationIndex]);
    }
  }
}

void displayStationInfo() {
  M5Cardputer.Display.fillRect(0, 20, 320, 200, BLACK);
  M5Cardputer.Display.setCursor(0, 20);
  M5Cardputer.Display.printf("%d/%d  ", stationIndex + 1, stationCount);

  if (displaySignalStrength) {
    short signalLevel = radio.getSignalLevel();
    int signalPercentage = (signalLevel * 100) / 15;

    if (signalPercentage < 60) {
      M5Cardputer.Display.setTextColor(YELLOW);
    } else {
      M5Cardputer.Display.setTextColor(GREEN);
    }
    M5Cardputer.Display.printf("Sig:%d%%  ", signalPercentage);
  }

  if (displayStereoStatus) {
    if (radio.isStereo()) {
      M5Cardputer.Display.setTextColor(GREEN);
      M5Cardputer.Display.print("Ster");
    } else {
      M5Cardputer.Display.setTextColor(WHITE);
      M5Cardputer.Display.print("Mono");
    }
  }

  M5Cardputer.Display.setTextColor(WHITE);
  M5Cardputer.Display.setCursor(0, 50);
  M5Cardputer.Display.setTextSize(3);
  M5Cardputer.Display.print("Freq:");

  M5Cardputer.Display.setCursor(0, 80);
  M5Cardputer.Display.printf("%.2f MHz", frequency);

  M5Cardputer.Display.setTextSize(2);
  M5Cardputer.Display.setCursor(0, 120);
  M5Cardputer.Display.fillRect(0, 120, 320, 20, BLACK);
  if (currentMode == SAVED) {
    M5Cardputer.Display.print(stationNames[stationIndex]);
  }
  M5Cardputer.Display.setTextColor(WHITE);
}
