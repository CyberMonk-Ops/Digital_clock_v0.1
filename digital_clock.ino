#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>



// ---  ROUTER CREDENTIALS ---
const char* ssid = "your wifi name";
const char* password = "your wifi password";




// ---  PIN MAPPING ---
#define DHTPIN D4
#define DHTTYPE DHT11
#define BUZZER_PIN D5
#define TOUCH_LEFT D6
#define TOUCH_RIGHT D7

Adafruit_SSD1306 display(128, 64, &Wire, -1);
DHT dht(DHTPIN, DHTTYPE);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 19800, 60000); 
ESP8266WebServer server(80);

// --- TELEMETRY & SENSORS ---
unsigned long lastDHTRead = 0;
float t = 0.0; 
float h = 0.0;
float batV = 0.0;
int batPct = 0;

  // --- STATE MACHINE ---
enum Mode { CLOCK_MODE, TIMER_MODE, GAME_MODE }; // <--- ADDED THE GAME MODE!
Mode currentMode = CLOCK_MODE;
bool isScreaming = false;
bool isDimmed = false;

// --- ARCADE LOGIC (CYBER-REX) ---
int dinoY = 40;
int dinoVY = 0;
int obsX = 128;
int score = 0;
bool inAir = false;
bool gameOver = false;


// --- ALARM LOGIC ---
int alarmHour = 7; 
int alarmMinute = 0;
bool alarmEnabled = false;
bool alarmTriggeredToday = false;

// --- TIMER LOGIC ---
int timerDefaultSeconds = 300; 
int timerRemaining = 300;
bool timerRunning = false;
unsigned long lastTimerTick = 0;

// ==========================================
// WEB SERVER HTML DASHBOARD
// ==========================================
void handleRoot() {
  String html = "<html><body style='background-color:black; color:cyan; font-family:monospace; text-align:center;'>";
  html += "<h2>CLOCK MATRIX COMMAND</h2>";
  html += "<p>Alarm (" + String(alarmHour) + ":" + (alarmMinute < 10 ? "0" : "") + String(alarmMinute) + ")</p>";
  html += "<form action='/set_alarm'><input type='number' name='h' placeholder='Hour (0-23)' min='0' max='23'> <input type='number' name='m' placeholder='Min (0-59)' min='0' max='59'> <input type='submit' value='SET ALARM'></form>";
  html += "<br><p>Timer Duration (Seconds)</p>";
  html += "<form action='/set_timer'><input type='number' name='s' placeholder='Seconds'> <input type='submit' value='SET TIMER'></form>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void handleSetAlarm() {
  if (server.hasArg("h") && server.hasArg("m")) {
    alarmHour = server.arg("h").toInt();
    alarmMinute = server.arg("m").toInt();
    alarmEnabled = true;
    alarmTriggeredToday = false;
  }
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleSetTimer() {
  if (server.hasArg("s")) {
    timerDefaultSeconds = server.arg("s").toInt();
    timerRemaining = timerDefaultSeconds;
    timerRunning = false;
  }
  server.sendHeader("Location", "/");
  server.send(303);
}

// ==========================================
//  ANIMATION
// ==========================================
void playAnimation() {
  for(int i = 0; i < 3; i++) {
    digitalWrite(BUZZER_PIN, HIGH); delay(20);
    digitalWrite(BUZZER_PIN, LOW); delay(20);
  }
  for(int radius = 1; radius < 32; radius += 3) {
    display.clearDisplay();
    display.drawCircle(64, 32, radius, WHITE);
    display.drawCircle(64, 32, radius - 1, WHITE);
    if (radius > 15) {
       display.fillRect(16, 26, 96, 12, BLACK);
       display.setTextSize(1);
       display.setCursor(18, 28);
       display.print("DISPLAY OVERRIDE");
    }
    display.display();
    delay(30); 
  }
  delay(1000); 
  
  // Trap until the user lets go
  while(digitalRead(TOUCH_LEFT) == HIGH || digitalRead(TOUCH_RIGHT) == HIGH) {
    delay(10); 
  }
}

// ==========================================
// THE NOISE FILTER 
// ==========================================
bool isRealTouch(int pin) {
  if (digitalRead(pin) == HIGH) {
    delay(50); // Wait 50ms to let the Wi-Fi RF radiation spike pass
    if (digitalRead(pin) == HIGH) {
      return true; //  finger confirmed
    }
  }
  return false;
}

// ==========================================
// SYSTEM BOOT
// ==========================================
void setup() {
  Serial.begin(115200);
  
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW); // BOOT UP SOUND
  
  pinMode(TOUCH_LEFT, INPUT);
  pinMode(TOUCH_RIGHT, INPUT);
  dht.begin();

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("OLED FAILED."));
    for(;;);
  }
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(10, 25);
  display.println("CLOCK MATRIX");
  display.display();

  WiFi.begin(ssid, password);
  while ( WiFi.status() != WL_CONNECTED ) { delay(500); }

  timeClient.begin();
  
  server.on("/", handleRoot);
  server.on("/set_alarm", handleSetAlarm);
  server.on("/set_timer", handleSetTimer);
  server.begin();

  Serial.println("");
  Serial.print("WEB COMMAND IP: ");
  Serial.println(WiFi.localIP());

  lastDHTRead = millis() + 5000; 
  digitalWrite(BUZZER_PIN, HIGH); delay(100); digitalWrite(BUZZER_PIN, LOW);
}

// ==========================================
// MAIN  LOOP
// ==========================================
void loop() {
  // 1. THE NETWORK ARMOR
  if (WiFi.status() == WL_CONNECTED) {
    timeClient.update();
    server.handleClient(); 
  }

  // 2. THE TOUCH SENSOR FILTER
  bool leftTouched = isRealTouch(TOUCH_LEFT);
  bool rightTouched = isRealTouch(TOUCH_RIGHT);



  
  // 3. THE NERVOUS SYSTEM
  if (leftTouched && rightTouched) {
    playAnimation(); 
    
    isDimmed = !isDimmed;
    if (isDimmed) {
      display.ssd1306_command(SSD1306_SETCONTRAST);
      display.ssd1306_command(10); // VERY LITTLE BRIGHTNESS
    } else {
      display.ssd1306_command(SSD1306_SETCONTRAST);
      display.ssd1306_command(255); // MAX BRIGHTNESS
    }
    
    while(digitalRead(TOUCH_LEFT) == HIGH || digitalRead(TOUCH_RIGHT) == HIGH) { delay(10); }
  } 
  else if (leftTouched || rightTouched) {
    if (isScreaming) {
      isScreaming = false;
      digitalWrite(BUZZER_PIN, LOW);
      if (timerRemaining <= 0) { timerRemaining = timerDefaultSeconds; timerRunning = false; }
      while(digitalRead(TOUCH_LEFT) == HIGH || digitalRead(TOUCH_RIGHT) == HIGH) { delay(10); }
    } else if (leftTouched) {
      // --- CYCLE THE SCREENS ---
      if (currentMode == CLOCK_MODE) currentMode = TIMER_MODE;
      else if (currentMode == TIMER_MODE) currentMode = GAME_MODE;
      else currentMode = CLOCK_MODE;
      
      digitalWrite(BUZZER_PIN, HIGH); delay(20); digitalWrite(BUZZER_PIN, LOW);
      while(digitalRead(TOUCH_LEFT) == HIGH) { delay(10); }
    } else if (rightTouched) {
      // --- CONTEXTUAL ACTION ---
      if (currentMode == CLOCK_MODE) {
        alarmEnabled = !alarmEnabled; 
        alarmTriggeredToday = false;
      } else if (currentMode == TIMER_MODE) {
        timerRunning = !timerRunning; 
      } else if (currentMode == GAME_MODE) {
        // CYBER-REX JUMP LOGIC
        if (gameOver) {
          gameOver = false; obsX = 128; score = 0; dinoY = 40; dinoVY = 0;
        } else if (!inAir) {
          dinoVY = -6; // Propel the Rex upward
          inAir = true;
        }
      }
      digitalWrite(BUZZER_PIN, HIGH); delay(20); digitalWrite(BUZZER_PIN, LOW);
      while(digitalRead(TOUCH_RIGHT) == HIGH) { delay(10); }
    }
  }


  // --- TIMER LOGIC ---
  if (timerRunning && timerRemaining > 0) {
    if (millis() - lastTimerTick >= 1000) {
      timerRemaining--;
      lastTimerTick = millis();
      if (timerRemaining <= 0) {
        isScreaming = true;
        currentMode = TIMER_MODE;
      }
    }
  }

  // --- ALARM LOGIC ---
  int ch = timeClient.getHours();
  int cm = timeClient.getMinutes();
  if (alarmEnabled && ch == alarmHour && cm == alarmMinute && !alarmTriggeredToday) {
    isScreaming = true;
    alarmTriggeredToday = true;
    currentMode = CLOCK_MODE; 
  }
  if (ch == 0 && cm == 0) alarmTriggeredToday = false;

  // --- THE SCREAM ---
  if (isScreaming) {
    if (millis() % 500 < 250) digitalWrite(BUZZER_PIN, HIGH);
    else digitalWrite(BUZZER_PIN, LOW);
  }



  // --- TELEMETRY & LITHIUM MATH ---
  if (millis() - lastDHTRead > 5000) {
    float newT = dht.readTemperature(); 
    float newH = dht.readHumidity();
    if (!isnan(newT) && !isnan(newH)) { t = newT; h = newH; }
    
    //  ReadS the battery 10 times to smooth out the Wi-Fi noise
    float rawAverage = 0;
    for(int i = 0; i < 10; i++) {
      rawAverage += analogRead(A0);
      delay(2); // Wait 2 milliseconds between readings
    }
    rawAverage = rawAverage / 10.0; // Calculate the true average
    
    batV = (rawAverage / 1023.0) * 36.3; 
    batPct = (batV - 3.2) * 100;
    if (batPct > 100) batPct = 100;
    if (batPct < 0) batPct = 0;

    lastDHTRead = millis();
  }



    // --- RENDER HUD ---
  display.clearDisplay();

  if (currentMode == CLOCK_MODE) {
    display.setTextSize(1); display.setCursor(0, 0);
    if (alarmEnabled) display.print("ALR:ON "); else display.print("ALR:OFF");
    display.setCursor(75, 0); display.print("BAT:"); display.print(batPct); display.print("%");

    display.setTextSize(3); display.setCursor(0, 20);
    int dispHour = ch; String ampm = "AM";
    if (dispHour >= 12) { ampm = "PM"; if (dispHour > 12) dispHour -= 12; }
    if (dispHour == 0) dispHour = 12;

    if (dispHour < 10) display.print("0");
    display.print(dispHour); display.print(":");
    if (cm < 10) display.print("0"); display.print(cm);
    display.setTextSize(1); display.setCursor(95, 20); display.print(ampm);
  } 
  else if (currentMode == TIMER_MODE) {
    display.setTextSize(1); display.setCursor(0, 0); display.print("TACTICAL TIMER");
    display.setCursor(75, 0); display.print("BAT:"); display.print(batPct); display.print("%");

    display.setTextSize(3); display.setCursor(15, 20);
    int tMin = timerRemaining / 60; int tSec = timerRemaining % 60;
    if (tMin < 10) display.print("0"); display.print(tMin); display.print(":");
    if (tSec < 10) display.print("0"); display.print(tSec);
  }
  else if (currentMode == GAME_MODE) {
    // --- CYBER-REX PHYSICS ENGINE ---
    if (!gameOver) {
      dinoVY += 1; // Gravity pulls down
      dinoY += dinoVY;
      if (dinoY >= 40) { dinoY = 40; inAir = false; dinoVY = 0; } // Hit the ground

      obsX -= 4; // Obstacle flying at you!
      if (obsX < 0) { obsX = 128; score++; }

      // Collision Math!
      if (obsX < 20 && obsX + 5 > 10 && dinoY + 10 > 40) gameOver = true;
    }

    // Draw the Cybernetic World
    display.drawLine(0, 50, 128, 50, WHITE); // The Floor
    display.fillRect(10, dinoY, 10, 10, WHITE); // Cyber-Rex
    display.fillRect(obsX, 40, 5, 10, WHITE); // The Obstacle

    display.setTextSize(1); display.setCursor(0, 0);
    display.print("SCORE: "); display.print(score);

    if (gameOver) {
      display.setCursor(35, 15); display.print("CRASHED");
      display.setCursor(10, 25); display.print("TAP RIGHT TO RETRY");
    }
  }

  // Only shows on Clock/Timer to keep the Game clean
  if (currentMode != GAME_MODE) {
    display.setTextSize(1); display.setCursor(0, 55);
    display.print("T:"); display.print(t, 1); display.print("C H:"); display.print(h, 1); display.print("%");
  }

  display.display();
  
  // Game Speed controller
  if (currentMode == GAME_MODE && !gameOver) {
    delay(30); // Prevent the game from running at 1000 FPS 
  }
}
