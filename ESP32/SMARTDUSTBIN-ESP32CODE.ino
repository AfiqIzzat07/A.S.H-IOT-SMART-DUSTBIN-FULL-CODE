#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <ESP32Servo.h>
#include <NewPing.h>
#include <Firebase_ESP_Client.h>
#include <HTTPClient.h>
#include <time.h>

/* ================= WIFI ================= */
const char* WIFI_SSID = "MakmalDT_KMK";
const char* WIFI_PASS = "MakmalDT_KMK";

/* ================= SHEET ================= */
const char* SHEET_URL = "https://script.google.com/macros/s/AKfycbzq-6QsFR0E9ens02t16Uqyrv9z6iM_XoSJZ7aJftNz9MVfyxZf3MP_uRbX7jKvwlZw/exec";

/* ================= FIREBASE ================= */
#define API_KEY "AIzaSyCn1ocjTr9yE297jt8im1URPkOzzbJti4E"
#define DATABASE_URL "https://ash-smart-dustbin-default-rtdb.asia-southeast1.firebasedatabase.app"

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

/* ================= TELEGRAM ================= */
const char* BOT_TOKEN = "8456561690:AAGdyXXTWpM_9LKriH6sjsaAz5CSYpiuXF4";
const char* CHAT_ID  = "-5249451947";

WiFiClientSecure client;

/* ================= PINS ================= */
#define TRIG_PIN 5
#define ECHO_PIN 18
#define PIR_PIN 19
#define SERVO1 23
#define SERVO2 22
#define LED_FULL 26
#define LED_OK 27

#define BIN_HEIGHT_CM 62
#define FULL_THRESHOLD 80
#define CLEAN_THRESHOLD 20
#define PIR_DURATION 10000

Servo servo1, servo2;
NewPing sonar(TRIG_PIN, ECHO_PIN, BIN_HEIGHT_CM);

/* ================= STATES ================= */
bool pirOpen = false;
unsigned long pirStart = 0;
int beforeValue = 0;

bool prevFull = false;
bool prevClean = false;

/* ================= SENSOR ================= */
int lastStable = 0;
int lastValid = 0;
int badReadCount = 0;

/* ================= BOOT ================= */
bool bootDone = false;
unsigned long bootStart = 0;

/* ================= SENSOR LOCK ================= */
bool sensorLock = false;
unsigned long lockStart = 0;
#define LOCK_TIME 3000

/* ================= CONTROL ================= */
bool forceOpen = false;
bool lidOpen = false;

/* ================= SHEET ================= */
int lastLoggedFullness = -1;
#define MAX_QUEUE 10
String logQueue[MAX_QUEUE];
int logCount = 0;

/* ================= TIME ================= */
String getTime() {
  struct tm t;
  if (!getLocalTime(&t)) return "0000-00-00 00:00:00";
  char buf[25];
  strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &t);
  return String(buf);
}

/* ================= TELEGRAM ================= */
void sendTelegram(String msg) {
  client.setInsecure();
  client.connect("api.telegram.org", 443);

  String url = "/bot" + String(BOT_TOKEN) +
               "/sendMessage?chat_id=" + CHAT_ID +
               "&text=" + msg;

  client.print(String("GET ") + url +
               " HTTP/1.1\r\nHost: api.telegram.org\r\nConnection: close\r\n\r\n");
}

/* ================= SENSOR ================= */
int getFullness() {

  int d = sonar.ping_cm();

  if (d <= 0 || d > BIN_HEIGHT_CM) {

    badReadCount++;

    if (badReadCount >= 3) {
      lastStable = 0;
      lastValid = 0;
      return 0;
    }

    return lastValid;
  }

  badReadCount = 0;

  int raw = map(d, BIN_HEIGHT_CM, 5, 0, 100);
  raw = constrain(raw, 0, 100);

  int diff = raw - lastStable;

  if (abs(diff) <= 15) lastStable += diff * 0.35;
  else lastStable = raw;

  int result = (lastStable / 5) * 5;

  lastValid = result;
  return result;
}

/* ================= QUEUE ================= */
void enqueue(String type, int fullness, int before, int after) {
  if (logCount >= MAX_QUEUE) return;

  String json =
    "{\"timestamp\":\"" + getTime() +
    "\",\"event_type\":\"" + type +
    "\",\"fullness\":" + String(fullness) +
    ",\"before\":" + String(before) +
    ",\"after\":" + String(after) + "}";

  logQueue[logCount++] = json;
}

/* ================= SHEET ================= */
void processQueue() {
  if (logCount == 0 || WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;
  http.begin(SHEET_URL);
  http.addHeader("Content-Type", "application/json");

  if (http.POST(logQueue[0]) > 0) {
    for (int i = 1; i < logCount; i++)
      logQueue[i - 1] = logQueue[i];
    logCount--;
  }

  http.end();
}

/* ================= SETUP ================= */
void setup() {
  Serial.begin(115200);

  pinMode(PIR_PIN, INPUT);
  pinMode(LED_FULL, OUTPUT);
  pinMode(LED_OK, OUTPUT);

  servo1.attach(SERVO1);
  servo2.attach(SERVO2);

  servo1.write(25);
  servo2.write(150);

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) delay(300);

  configTime(8 * 3600, 0, "pool.ntp.org");

  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  auth.user.email = "afiqizzat1105@gmail.com";
  auth.user.password = "J1T12007";

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  bootStart = millis();

  sendTelegram("🟢 Smart Dustbin Online");
}

/* ================= LOOP ================= */
void loop() {

  unsigned long now = millis();

  if (!bootDone && now - bootStart > 8000) {
    bootDone = true;
    lastLoggedFullness = getFullness();
  }

  /* ================= MANUAL ================= */
  String cmd;
  if (Firebase.RTDB.getString(&fbdo, "/dustbin/servo")) {
    cmd = fbdo.stringData();
    forceOpen = (cmd == "OPEN");
  }

  /* ================= SENSOR LOCK ================= */
  if (lidOpen) {
    sensorLock = true;
    lockStart = now;
  }

  if (sensorLock && now - lockStart > LOCK_TIME) {
    sensorLock = false;
  }

  int fullness = sensorLock ? lastValid : getFullness();

  Firebase.RTDB.setInt(&fbdo, "/dustbin/fullness", fullness);

  /* ================= EVENT (FIXED TELEGRAM LOGIC) ================= */
  bool eventThisLoop = false;

  /* FULL EVENT */
  if (fullness >= FULL_THRESHOLD) {
    if (!prevFull) {
      sendTelegram("🚨 Bin FULL (" + String(fullness) + "%)");
      enqueue("FULL", fullness, lastLoggedFullness, fullness);
      prevFull = true;
      eventThisLoop = true;
      lastLoggedFullness = fullness;
    }
  } else {
    prevFull = false;
  }

  /* CLEAN EVENT (NO TELEGRAM) */
  if (fullness <= CLEAN_THRESHOLD) {
    if (!prevClean) {
      enqueue("CLEANED", fullness, lastLoggedFullness, fullness);
      prevClean = true;
      lastLoggedFullness = fullness;
    }
  } else {
    prevClean = false;
  }

  /* ================= UPDATE ================= */
  static int stableCount = 0;

  if (!eventThisLoop) {

    if (fullness == lastLoggedFullness) {
      stableCount = 0;
    } else {
      stableCount++;

      if (stableCount >= 2) {

        String json =
          "{\"timestamp\":\"" + getTime() +
          "\",\"event_type\":\"UPDATE" +
          "\",\"fullness\":" + String(fullness) +
          ",\"before\":" + String(lastLoggedFullness) +
          ",\"after\":" + String(fullness) + "}";

        if (logCount < MAX_QUEUE) {
          logQueue[logCount++] = json;
          lastLoggedFullness = fullness;
        }

        stableCount = 0;
      }
    }
  } else {
    stableCount = 0;
  }

  /* ================= PIR ================= */
  bool pirAllowed = (fullness < FULL_THRESHOLD);
  bool pirTrigger = pirAllowed && digitalRead(PIR_PIN);

  if (forceOpen) {
    lidOpen = true;
    pirOpen = false;
  }
  else if (pirTrigger) {
    lidOpen = true;

    if (!pirOpen) {
      pirOpen = true;
      pirStart = now;
      beforeValue = fullness;
    }
  }
  else if (!pirOpen) {
    lidOpen = false;
  }

  servo1.write(lidOpen ? 95 : 25);
  servo2.write(lidOpen ? 80 : 150);

  if (pirOpen && now - pirStart >= PIR_DURATION) {
    pirOpen = false;

    if (fullness != beforeValue) {
      enqueue("PIR", 0, beforeValue, fullness);
    }
  }

  digitalWrite(LED_FULL, fullness >= FULL_THRESHOLD);
  digitalWrite(LED_OK, fullness < FULL_THRESHOLD);

  processQueue();
  delay(1000);
}