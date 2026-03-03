#include <Arduino.h>
#include <Servo.h>

const byte BTN_PIN = 2;
const byte SERVO_PINS[3] = {9, 10, 11};


int usNow[3] = {1500,1500,1500};
int usMin = 1000;
int usMax = 2000;

int usStep = 5;
int usDelay = 5;

Servo sv[3];
int active = 0;           // 0..2

// антидребезг
bool lastRead = HIGH;
unsigned long lastChangeMs = 0;
const unsigned long DEBOUNCE_MS = 35;

// простий буфер для Serial
char lineBuf[40];
byte lineLen = 0;

void selectServo(int idx) {
  // відчепити всі, щоб “активна” була тільки одна
  for (int i = 0; i < 3; i++) {
    if (sv[i].attached()) sv[i].detach();
    pinMode(SERVO_PINS[i], INPUT);
  }

  active = idx;

  pinMode(SERVO_PINS[active], OUTPUT);
  sv[active].attach(SERVO_PINS[active]);
  sv[active].writeMicroseconds(usNow[active]);

  Serial.print(F("Active servo: "));
  Serial.println(active + 1);
  delay(100);
}
void moveServoUS(int idx, int targetUS)
{
  targetUS = constrain(targetUS, usMin, usMax);
  if (!sv[idx].attached()) return;

  int current = usNow[idx];

  if (current < targetUS) {
    for (int u = current; u <= targetUS; u += usStep) {
      sv[idx].writeMicroseconds(u);
      usNow[idx] = u;
      delay(usDelay);
    }
  } else if (current > targetUS) {
    for (int u = current; u >= targetUS; u -= usStep) {
      sv[idx].writeMicroseconds(u);
      usNow[idx] = u;
      delay(usDelay);
    }
  }
}


void setPulseFor(int idx, int us) 
{
  us = constrain(us, usMin, usMax);

  if(sv[idx].attached() && idx == active)
  {
    moveServoUS(idx, us);
  }

  Serial.print(F("Servo "));// F - Use flash (32kB) not SRAM (2kB)
  Serial.print(idx + 1);
  Serial.print(F(" pulse = "));
  Serial.println(us);
}

void printHelp() {
  Serial.println(F("Commands:"));
  Serial.println(F("  u <1000..2000> - set pulse width (us)"));
  Serial.println(F("  a <0..180>     - set angle (mapped to us)"));
  Serial.println(F("  step <n>       - pulse step (us)"));
  Serial.println(F("  delay <ms>     - delay per step"));
  Serial.println(F("  s <1..3>      - select servo"));
  Serial.println(F("  help          - show this help"));
  Serial.println(F("Button on D2 cycles 1->2->3"));
}

void handleLine(char* s) {
  // пропустити пробіли
  while (*s == ' ' || *s == '\t' || *s == '\r') s++;
  if (*s == 0) return;

  if (!strncmp(s, "help", 4)) {
    printHelp();
    return;
  }

  // команда "s N"
  if (s[0] == 's' || s[0] == 'S') {
    int n = -1;
    if (sscanf(s + 1, "%d", &n) == 1 && n >= 1 && n <= 3) {
      selectServo(n - 1);
    } else {
      Serial.println(F("Bad 's' command. Use: s 1..3"));
    }
    return;
  }

  // команда "a X"
if (s[0] == 'a' || s[0] == 'A') {
  int a;
  if (sscanf(s + 1, "%d", &a) == 1) {
    a = constrain(a, 0, 180);
    int us = map(a, 0, 180, usMin, usMax);
    setPulseFor(active, us);
  }
  return;
}




  

  if (s[0] == 'u') {
  int us;
  if (sscanf(s + 1, "%d", &us) == 1) {
    setPulseFor(active, us);
  }
  return;
}

  if (!strncmp(s, "step", 4)) {
    int v;
    if (sscanf(s + 4, "%d", &v) == 1 && v > 0) {
      usStep = v;
      Serial.print(F("usStep = "));
      Serial.println(usStep);

    }
    return;
  }


if (!strncmp(s, "delay", 5)) {
  int v;
  if (sscanf(s + 5, "%d", &v) == 1 && v > 0) {
    usDelay = v;
    Serial.print(F("usDelay = "));
    Serial.println(usDelay);
  }
  return;
}


    // якщо це просто число — теж кут
  int v = -1;
  if (sscanf(s, "%d", &v) == 1) {
    setPulseFor(active, v);
    return;
  }

  Serial.println(F("Unknown command. Type: help"));
}

void setup() {
  Serial.begin(115200);
  pinMode(BTN_PIN, INPUT_PULLUP);

  // старт: серва 1 активна
  selectServo(0);
  printHelp();
}

void loop() {
  // ---- кнопка: циклічний вибір серви ----
  bool r = digitalRead(BTN_PIN);

  if (r != lastRead) {
    lastRead = r;
    lastChangeMs = millis();
  }

  if (millis() - lastChangeMs > DEBOUNCE_MS) {
    static bool armed = true;

    if (armed && r == LOW) {
      armed = false;
      int next = (active + 1) % 3;
      selectServo(next);
    }
    if (r == HIGH) armed = true;
  }

  // ---- Serial: читання рядка ----
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\n') {
      lineBuf[lineLen] = 0;
      handleLine(lineBuf);
      lineLen = 0;
    } else if (c != '\r') {
      if (lineLen < sizeof(lineBuf) - 1) {
        lineBuf[lineLen++] = c;
      } else {
        // переповнення — скидаємо
        lineLen = 0;
      }
    }
  }
}

