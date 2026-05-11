/*
 * FM Radio Controller for M5Cardputer 1.1 & Advanced
 * Author: PatsBotBench, revised with Codex
 * License: MIT
 *
 * Controls a TEA5767 FM receiver over I2C from the Grove port.
 * Copy RadioSta.txt to the SD card root for saved-station mode.
 *
 * RadioSta.txt format:
 *   89.50,Station Name
 *   101.10,Another Station
 */

#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <M5Cardputer.h>
#include "TEA5767Local.h"

TEA5767 radio = TEA5767();

enum RadioMode { SCANNED, SAVED };

// ------------------------- User preferences -------------------------
const RadioMode startupMode = SAVED;

const int i2cSdaPin = 2;       // Cardputer Grove SDA: G2
const int i2cSclPin = 1;       // Cardputer Grove SCL: G1

const int sdSpiSckPin = 40;
const int sdSpiMisoPin = 39;
const int sdSpiMosiPin = 14;
const int sdSpiCsPin = 12;
const uint32_t sdSpiFrequency = 25000000;

const double bottomFrequency = 87.50;
const double topFrequency = 108.00;
const double scanStep = 0.10;
const double fineTuneStep = 0.10;

const short minSignalLevel = 9;          // TEA5767 reports 0-15
const unsigned long scanDelay = 100;     // ms between scan checks
const unsigned long debounceDelay = 200; // ms between accepted key events
const unsigned long statusUpdateInterval = 120000;
const unsigned long messageDuration = 5000;

const bool displaySignalStrength = true;
const bool displayStereoStatus = true;

const char* stationsFile = "/RadioSta.txt";
const char* stationsTempFile = "/RadioSta.tmp";
const int maxStations = 100;
const int maxStationNameLength = 20;
const int infoLineX = 0;
const int infoLineY = 120;
const int infoLineClearY = 112;
const int infoLineHeight = 23;
// --------------------------------------------------------------------

RadioMode currentMode = startupMode;

double frequency = bottomFrequency;
int stationIndex = 0;
int stationCount = 0;
double stationFrequencies[maxStations];
String stationNames[maxStations];

int savedStationCount = 0;
double savedStationFrequencies[maxStations];
String savedStationNames[maxStations];

bool sdAvailable = false;
bool showMessage = false;
bool muted = false;
bool forcedMono = false;
bool enteringFrequency = false;
String frequencyEntry = "";

unsigned long lastScanTime = 0;
unsigned long lastKeyPressTime = 0;
unsigned long lastStatusUpdate = 0;
unsigned long messageStartTime = 0;

double clampFrequency(double value) {
  if (value < bottomFrequency) return bottomFrequency;
  if (value > topFrequency) return topFrequency;
  return value;
}

double roundedFrequency(double value) {
  return round(value * 100.0) / 100.0;
}

bool sameFrequency(double left, double right) {
  return fabs(roundedFrequency(left) - roundedFrequency(right)) < 0.01;
}

int findSavedStationIndex(double targetFrequency) {
  for (int i = 0; i < savedStationCount; i++) {
    if (sameFrequency(savedStationFrequencies[i], targetFrequency)) {
      return i;
    }
  }
  return -1;
}

void drawSavedStationLabel() {
  if (showMessage || enteringFrequency) return;

  M5Cardputer.Display.fillRect(infoLineX, infoLineClearY, 240, infoLineHeight, BLACK);
  int savedIndex = findSavedStationIndex(frequency);
  if (savedIndex >= 0) {
    M5Cardputer.Display.setCursor(infoLineX, infoLineY);
    M5Cardputer.Display.setTextColor(WHITE);
    M5Cardputer.Display.setTextSize(2);
    M5Cardputer.Display.print(savedStationNames[savedIndex]);
  }
}

void showStatusMessage(const char* message) {
  M5Cardputer.Display.fillRect(infoLineX, infoLineClearY, 240, infoLineHeight, BLACK);
  M5Cardputer.Display.setCursor(infoLineX, infoLineY);
  M5Cardputer.Display.setTextColor(WHITE);
  M5Cardputer.Display.setTextSize(2);
  M5Cardputer.Display.print(message);
  showMessage = true;
  messageStartTime = millis();
}

void clearMessage() {
  M5Cardputer.Display.fillRect(infoLineX, infoLineClearY, 240, infoLineHeight, BLACK);
  showMessage = false;
  drawSavedStationLabel();
}

void drawFrequencyEntry() {
  M5Cardputer.Display.fillRect(infoLineX, infoLineClearY, 240, infoLineHeight, BLACK);
  M5Cardputer.Display.setCursor(infoLineX, infoLineY);
  M5Cardputer.Display.setTextColor(CYAN);
  M5Cardputer.Display.setTextSize(2);
  M5Cardputer.Display.print("Tune: ");
  M5Cardputer.Display.print(frequencyEntry);
}

void beginFrequencyEntry(char key) {
  clearMessage();
  enteringFrequency = true;
  frequencyEntry = "";
  appendFrequencyEntryKey(key);
}

void appendFrequencyEntryKey(char key) {
  if (key == '-') key = '.';

  if (key == '.') {
    if (frequencyEntry.indexOf('.') >= 0) return;
    if (frequencyEntry.length() == 0) frequencyEntry = "0";
  }

  if (frequencyEntry.length() >= 6) return;
  frequencyEntry += key;
  drawFrequencyEntry();
}

void cancelFrequencyEntry() {
  enteringFrequency = false;
  frequencyEntry = "";
  drawSavedStationLabel();
}

void confirmFrequencyEntry() {
  if (frequencyEntry.length() == 0) {
    cancelFrequencyEntry();
    return;
  }

  double enteredFrequency = roundedFrequency(frequencyEntry.toFloat());
  if (enteredFrequency < bottomFrequency || enteredFrequency > topFrequency) {
    enteringFrequency = false;
    frequencyEntry = "";
    showStatusMessage("Out of band");
    return;
  }

  enteringFrequency = false;
  frequencyEntry = "";
  frequency = enteredFrequency;
  radio.setFrequency(frequency);
  displayStationInfo();
}

bool handleFrequencyEntryKey() {
  if (!enteringFrequency) return false;

  if (M5Cardputer.Keyboard.isKeyPressed(0x28)) {
    confirmFrequencyEntry();
  } else if (M5Cardputer.Keyboard.isKeyPressed(0x08)) {
    if (frequencyEntry.length() > 0) {
      frequencyEntry.remove(frequencyEntry.length() - 1);
      drawFrequencyEntry();
    }
  } else if (M5Cardputer.Keyboard.isKeyPressed(0x1B)) {
    cancelFrequencyEntry();
  } else if (M5Cardputer.Keyboard.isKeyPressed('-')) {
    appendFrequencyEntryKey('-');
  } else {
    for (char key = '0'; key <= '9'; key++) {
      if (M5Cardputer.Keyboard.isKeyPressed(key)) {
        appendFrequencyEntryKey(key);
        break;
      }
    }
  }

  return true;
}

bool maybeStartFrequencyEntry() {
  if (M5Cardputer.Keyboard.isKeyPressed('-')) {
    beginFrequencyEntry('-');
    return true;
  }

  for (char key = '0'; key <= '9'; key++) {
    if (M5Cardputer.Keyboard.isKeyPressed(key)) {
      beginFrequencyEntry(key);
      return true;
    }
  }

  return false;
}

void setup() {
  Serial.begin(115200);
  Wire.begin(i2cSdaPin, i2cSclPin);

  auto cfg = M5.config();
  M5Cardputer.begin(cfg, true);
  M5Cardputer.Display.setRotation(1);
  M5Cardputer.Display.clear();
  M5Cardputer.Display.setTextColor(WHITE);
  M5Cardputer.Display.setTextSize(2);

  SPI.begin(sdSpiSckPin, sdSpiMisoPin, sdSpiMosiPin, sdSpiCsPin);
  sdAvailable = SD.begin(sdSpiCsPin, SPI, sdSpiFrequency);
  if (!sdAvailable && currentMode == SAVED) {
    currentMode = SCANNED;
  }

  displayMode();

  if (!sdAvailable) {
    showStatusMessage("SD not found; scan only");
  } else {
    loadSavedStationList(true);
  }

  if (currentMode == SCANNED) {
    prescanStations();
  } else {
    copySavedStationsToNavigation();
  }

  if (stationCount > 0) {
    stationIndex = 0;
    frequency = stationFrequencies[stationIndex];
    setStation();
  } else {
    frequency = bottomFrequency;
    radio.setFrequency(frequency);
    displayStationInfo();
    showStatusMessage("No stations found");
  }
}

void loop() {
  M5Cardputer.update();

  unsigned long currentMillis = millis();
  if (currentMillis - lastKeyPressTime >= debounceDelay) {
    if (M5Cardputer.Keyboard.isChange()) {
      lastKeyPressTime = currentMillis;

      if (handleFrequencyEntryKey()) {
        return;
      } else if (maybeStartFrequencyEntry()) {
        return;
      } else if (M5Cardputer.Keyboard.isKeyPressed('c')) {
        cycleMode();
      } else if (M5Cardputer.Keyboard.isKeyPressed('m')) {
        toggleMute();
      } else if (M5Cardputer.Keyboard.isKeyPressed('f')) {
        toggleForcedMono();
      } else if (M5Cardputer.Keyboard.isKeyPressed('d')) {
        saveCurrentStation();
      } else if (M5Cardputer.Keyboard.isKeyPressed('x')) {
        deleteCurrentStation();
      } else if (M5Cardputer.Keyboard.isKeyPressed('/')) {
        handleKeyPress('/');
      } else if (M5Cardputer.Keyboard.isKeyPressed(',')) {
        handleKeyPress(',');
      } else if (M5Cardputer.Keyboard.isKeyPressed(0x28)) {
        if (showMessage) {
          clearMessage();
        } else {
          setStation();
        }
      } else if (M5Cardputer.Keyboard.isKeyPressed('s')) {
        if (currentMode == SCANNED) {
          prescanStations();
          if (stationCount > 0) {
            stationIndex = 0;
            frequency = stationFrequencies[stationIndex];
            setStation();
          } else {
            displayStationInfo();
            showStatusMessage("No stations found");
          }
        }
      } else if (M5Cardputer.Keyboard.isKeyPressed('.')) {
        adjustFrequency(-fineTuneStep);
      } else if (M5Cardputer.Keyboard.isKeyPressed(';')) {
        adjustFrequency(fineTuneStep);
      }
    }
  }

  if ((currentMillis - lastStatusUpdate >= statusUpdateInterval) && (displaySignalStrength || displayStereoStatus)) {
    lastStatusUpdate = currentMillis;
    displayStationInfo();
  }

  if (showMessage && currentMillis - messageStartTime >= messageDuration) {
    clearMessage();
  }

  delay(50);
}

void adjustFrequency(double increment) {
  frequency = roundedFrequency(frequency + increment);
  if (frequency > topFrequency) frequency = bottomFrequency;
  if (frequency < bottomFrequency) frequency = topFrequency;

  Serial.print("Fine tuned frequency: ");
  Serial.println(frequency);

  radio.setFrequency(frequency);
  displayStationInfo();
}

void toggleMute() {
  muted = !muted;
  radio.setMuted(muted);
  displayStationInfo();
  showStatusMessage(muted ? "Muted" : "Audio on");
}

void toggleForcedMono() {
  forcedMono = !forcedMono;
  radio.setForcedMono(forcedMono);
  displayStationInfo();
  showStatusMessage(forcedMono ? "Forced mono" : "Stereo allowed");
}

void selectSavedFrequency(double targetFrequency) {
  for (int i = 0; i < stationCount; i++) {
    if (sameFrequency(stationFrequencies[i], targetFrequency)) {
      stationIndex = i;
      frequency = stationFrequencies[stationIndex];
      return;
    }
  }
}

void copySavedStationsToNavigation() {
  stationCount = savedStationCount;
  for (int i = 0; i < savedStationCount; i++) {
    stationFrequencies[i] = savedStationFrequencies[i];
    stationNames[i] = savedStationNames[i];
  }

  if (stationCount > 0) {
    stationIndex = 0;
    frequency = stationFrequencies[stationIndex];
  }
}

bool loadSavedStationList(bool showErrors) {
  savedStationCount = 0;

  File file = SD.open(stationsFile, FILE_READ);
  if (!file) {
    if (showErrors) showStatusMessage("RadioSta.txt missing");
    return false;
  }

  while (file.available() && savedStationCount < maxStations) {
    String line = file.readStringUntil('\n');
    line.trim();
    if (line.length() == 0 || line.startsWith("#")) continue;

    int commaIndex = line.indexOf(',');
    if (commaIndex <= 0) continue;

    double savedFrequency = roundedFrequency(line.substring(0, commaIndex).toFloat());
    String name = line.substring(commaIndex + 1);
    name.trim();

    if (savedFrequency >= bottomFrequency && savedFrequency <= topFrequency && findSavedStationIndex(savedFrequency) < 0) {
      savedStationFrequencies[savedStationCount] = savedFrequency;
      savedStationNames[savedStationCount] = name.length() > maxStationNameLength ? name.substring(0, maxStationNameLength) : name;
      savedStationCount++;
    }
  }

  file.close();

  if (savedStationCount == 0 && showErrors) {
    showStatusMessage("No valid saved stations");
  }

  return savedStationCount > 0;
}

bool writeSavedStationListToSd() {
  if (!sdAvailable) return false;

  SD.remove(stationsTempFile);
  File file = SD.open(stationsTempFile, FILE_WRITE);
  if (!file) return false;

  for (int i = 0; i < savedStationCount; i++) {
    file.printf("%.2f,%s\n", savedStationFrequencies[i], savedStationNames[i].c_str());
  }
  file.close();

  SD.remove(stationsFile);
  return SD.rename(stationsTempFile, stationsFile);
}

void saveCurrentStation() {
  if (!sdAvailable) {
    showStatusMessage("SD not found");
    return;
  }

  double savedFrequency = roundedFrequency(clampFrequency(frequency));
  if (findSavedStationIndex(savedFrequency) >= 0) {
    showStatusMessage("Already saved");
    return;
  }

  if (savedStationCount >= maxStations) {
    showStatusMessage("Save list full");
    return;
  }

  char stationLabel[12];
  snprintf(stationLabel, sizeof(stationLabel), "%.2f", savedFrequency);
  savedStationFrequencies[savedStationCount] = savedFrequency;
  savedStationNames[savedStationCount] = stationLabel;
  savedStationCount++;

  if (!writeSavedStationListToSd()) {
    savedStationCount--;
    showStatusMessage("Save failed");
    return;
  }

  if (currentMode == SAVED) {
    copySavedStationsToNavigation();
    selectSavedFrequency(savedFrequency);
    displayStationInfo();
  }

  char message[32];
  snprintf(message, sizeof(message), "Saved %.2f", savedFrequency);
  showStatusMessage(message);
}

void deleteCurrentStation() {
  if (!sdAvailable) {
    showStatusMessage("SD not found");
    return;
  }

  double deletedFrequency = roundedFrequency(clampFrequency(frequency));
  int previousIndex = stationIndex;
  int savedIndex = findSavedStationIndex(deletedFrequency);

  if (savedIndex < 0) {
    showStatusMessage("Not saved");
    return;
  }

  double removedFrequency = savedStationFrequencies[savedIndex];
  String removedName = savedStationNames[savedIndex];
  for (int i = savedIndex; i < savedStationCount - 1; i++) {
    savedStationFrequencies[i] = savedStationFrequencies[i + 1];
    savedStationNames[i] = savedStationNames[i + 1];
  }
  savedStationCount--;

  if (!writeSavedStationListToSd()) {
    for (int i = savedStationCount; i > savedIndex; i--) {
      savedStationFrequencies[i] = savedStationFrequencies[i - 1];
      savedStationNames[i] = savedStationNames[i - 1];
    }
    savedStationFrequencies[savedIndex] = removedFrequency;
    savedStationNames[savedIndex] = removedName;
    savedStationCount++;
    showStatusMessage("Delete failed");
    return;
  }

  if (currentMode == SAVED) {
    copySavedStationsToNavigation();
    if (stationCount > 0) {
      stationIndex = previousIndex;
      if (stationIndex >= stationCount) stationIndex = stationCount - 1;
      frequency = stationFrequencies[stationIndex];
      radio.setFrequency(frequency);
    }
    displayStationInfo();
  } else {
    displayStationInfo();
  }

  char message[32];
  snprintf(message, sizeof(message), "Deleted %.2f", deletedFrequency);
  showStatusMessage(message);
}

void displayMode() {
  M5Cardputer.Display.fillRect(0, 0, 320, 20, BLACK);
  M5Cardputer.Display.setCursor(0, 0);
  M5Cardputer.Display.setTextSize(2);
  M5Cardputer.Display.setTextColor(WHITE);

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
  clearMessage();

  if (currentMode == SCANNED) {
    prescanStations();
  } else if (sdAvailable) {
    loadSavedStationList(true);
    copySavedStationsToNavigation();
  } else {
    stationCount = 0;
    showStatusMessage("SD not found");
  }

  if (stationCount > 0) {
    stationIndex = 0;
    frequency = stationFrequencies[stationIndex];
    setStation();
  } else {
    displayStationInfo();
    if (currentMode == SAVED) {
      showStatusMessage("No saved stations");
    }
  }
}

void prescanStations() {
  frequency = bottomFrequency;
  stationCount = 0;
  lastScanTime = 0;
  showStatusMessage("Scanning...");

  while (frequency <= topFrequency && stationCount < maxStations) {
    M5Cardputer.update();

    unsigned long currentMillis = millis();
    if (currentMillis - lastScanTime >= scanDelay) {
      lastScanTime = currentMillis;
      radio.setFrequency(frequency);

      M5Cardputer.Display.fillRect(0, 80, 240, 30, BLACK);
      M5Cardputer.Display.setCursor(0, 80);
      M5Cardputer.Display.setTextColor(WHITE);
      M5Cardputer.Display.setTextSize(3);
      M5Cardputer.Display.printf("%.2f MHz", frequency);

      if (radio.getSignalLevel() >= minSignalLevel) {
        stationFrequencies[stationCount] = roundedFrequency(frequency);
        stationNames[stationCount] = "";
        stationCount++;
      }

      frequency = roundedFrequency(frequency + scanStep);
    }
  }

  displayStationInfo();
  char message[32];
  snprintf(message, sizeof(message), "Found %d stations", stationCount);
  showStatusMessage(message);
  delay(1000);
}

void handleKeyPress(char key) {
  if (stationCount <= 0) {
    showStatusMessage("No station list");
    return;
  }

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

void setStation() {
  frequency = roundedFrequency(clampFrequency(frequency));
  radio.setFrequency(frequency);
  showMessage = false;
  displayStationInfo();
}

void displayStationInfo() {
  M5Cardputer.Display.fillRect(0, 20, 240, 92, BLACK);
  M5Cardputer.Display.setCursor(0, 20);
  M5Cardputer.Display.setTextSize(2);
  M5Cardputer.Display.setTextColor(WHITE);

  if (stationCount > 0) {
    M5Cardputer.Display.printf("%d/%d  ", stationIndex + 1, stationCount);
  } else {
    M5Cardputer.Display.print("--/--  ");
  }

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

  M5Cardputer.Display.setTextColor(WHITE);
  M5Cardputer.Display.setCursor(0, 50);
  M5Cardputer.Display.setTextSize(3);
  M5Cardputer.Display.print("Freq:");
  if (muted) {
    M5Cardputer.Display.setCursor(105, 50);
    M5Cardputer.Display.setTextColor(RED);
    M5Cardputer.Display.print("MUTE");
    M5Cardputer.Display.setTextColor(WHITE);
  } else if (displayStereoStatus) {
    M5Cardputer.Display.setCursor(105, 50);
    if (forcedMono) {
      M5Cardputer.Display.setTextColor(YELLOW);
      M5Cardputer.Display.print("FMon");
    } else if (radio.isStereo()) {
      M5Cardputer.Display.setTextColor(GREEN);
      M5Cardputer.Display.print("Ster");
    } else {
      M5Cardputer.Display.setTextColor(WHITE);
      M5Cardputer.Display.print("Mono");
    }
    M5Cardputer.Display.setTextColor(WHITE);
  }

  M5Cardputer.Display.setCursor(0, 80);
  M5Cardputer.Display.printf("%.2f MHz", frequency);

  M5Cardputer.Display.setTextColor(WHITE);
  drawSavedStationLabel();
}
