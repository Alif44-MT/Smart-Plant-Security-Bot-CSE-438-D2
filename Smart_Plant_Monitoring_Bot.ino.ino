#include <Servo.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>

// Motor Driver Pins
#define enA 5
#define in1 7
#define in2 8
#define in3 9
#define in4 10
#define enB 6

// IR Sensors
#define L_S A3
#define R_S A0

// Ultrasonic Sensor
#define TRIG_PIN A1
#define ECHO_PIN A2

// Servo + Buzzer + PIR
#define SERVO_PIN   11
#define BUZZER_PIN  4
#define PIR_PIN     12

// GSM Module
SoftwareSerial gsm(2, 3);

// LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Speed Settings
int MOTOR_SPEED = 150;
int TURN_SPEED  = 130;

// IR Sensor Logic: Black line = HIGH (1), White = LOW (0)
#define BLACK 1

// Distance Settings
#define OBSTACLE_DIST 20

// Servo Angles
#define CENTER_ANGLE 90
#define LEFT_ANGLE  150
#define RIGHT_ANGLE  30

// Phone Number
String PHONE = "+8801XXXXXXXXX";

// Variables
int  lastTurn = 0;
bool smsSent  = false;
String lcdLine0 = "";
String lcdLine1 = "";

Servo scanServo;

// ==================================================
// SETUP
// ==================================================
void setup() {
  Serial.begin(9600);
  gsm.begin(9600);

  pinMode(enA, OUTPUT);
  pinMode(in1, OUTPUT);
  pinMode(in2, OUTPUT);
  pinMode(in3, OUTPUT);
  pinMode(in4, OUTPUT);
  pinMode(enB, OUTPUT);

  pinMode(L_S,        INPUT);
  pinMode(R_S,        INPUT);
  pinMode(TRIG_PIN,   OUTPUT);
  pinMode(ECHO_PIN,   INPUT);
  pinMode(PIR_PIN,    INPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  scanServo.attach(SERVO_PIN);
  scanServo.write(CENTER_ANGLE);
  delay(500);

  lcd.init();
  lcd.backlight();
  motorStop();

  lcdPrint("Smart Plant Bot", "Starting...");
  delay(2000);

  lcdPrint("GSM Checking...", "Please wait");
  initGSM();

  lcdPrint("System Ready!", "Line Follow");
  delay(1500);
}

// ==================================================
// MAIN LOOP
// ==================================================
void loop() {
  int pirState  = digitalRead(PIR_PIN);
  int frontDist = getDistance();

  Serial.print("Dist:"); Serial.print(frontDist);
  Serial.print(" PIR:"); Serial.print(pirState);
  Serial.print(" L:");   Serial.print(digitalRead(L_S));
  Serial.print(" R:");   Serial.println(digitalRead(R_S));

  // Priority 1: PIR threat
  if (pirState == HIGH) {
    dangerAlert();
    return;
  }

  // Priority 2: Obstacle
  if (frontDist > 0 && frontDist <= OBSTACLE_DIST) {
    obstacleAvoid();
    return;
  }

  // Priority 3: Line follow
  lineFollow();
}

// ==================================================
// GSM
// ==================================================
void initGSM() {

  lcdPrint("Checking GSM", "");

  gsm.println("AT");
  delay(1000);

  gsm.println("AT+CMGF=1");
  delay(1000);

  gsm.println("AT+CNMI=1,2,0,0,0");
  delay(1000);

  lcdPrint("GSM Ready", "");
  delay(1000);
}
void sendSMS(String message) {
  gsm.println("AT+CMGF=1");
  delay(500);
  gsm.print("AT+CMGS=\"");
  gsm.print(PHONE);
  gsm.println("\"");
  delay(500);
  gsm.print(message);
  delay(200);
  gsm.write(26);
  delay(3000);
}

// ==================================================
// PIR DANGER ALERT
// ==================================================
void dangerAlert() {
  motorStop();
  int dist = getDistance();

  if (!smsSent) {
    String smsMsg = "DANGER! Motion near plants. Distance: ";
    smsMsg += String(dist);
    smsMsg += " cm. Check immediately!";
    sendSMS(smsMsg);
    smsSent = true;
  }

  while (digitalRead(PIR_PIN) == HIGH) {
    motorStop();
    dist = getDistance();
    lcdPrint("!! DANGER !!", "Dist:" + String(dist) + "cm");
    digitalWrite(BUZZER_PIN, HIGH); delay(200);
    digitalWrite(BUZZER_PIN, LOW);  delay(150);
  }

  digitalWrite(BUZZER_PIN, LOW);
  smsSent = false;
  lcdPrint("Danger Cleared", "Resume Patrol");
  delay(1200);
}

// ==================================================
// LINE FOLLOWING
// ==================================================
void lineFollow() {
  int L = digitalRead(L_S);
  int R = digitalRead(R_S);

  if (L == BLACK && R == BLACK) {
    lcdUpdate("Mode: Patrol", "Forward");
    motorForward();
    lastTurn = 0;
  }
  else if (L == BLACK && R != BLACK) {
    lcdUpdate("Mode: Patrol", "Turning Left");
    motorTurnLeft();
    lastTurn = 1;
  }
  else if (L != BLACK && R == BLACK) {
    lcdUpdate("Mode: Patrol", "Turning Right");
    motorTurnRight();
    lastTurn = 2;
  }
  else {
    lcdUpdate("Mode: Patrol", "Searching...");
    searchLine();
  }

  delay(8);
}

// ==================================================
// OBSTACLE AVOIDANCE
// ==================================================
void obstacleAvoid() {
  motorStop();
  lcdPrint("Obstacle Found!", "Scanning...");
  beepTwice();
  delay(300);

  scanServo.write(LEFT_ANGLE);
  delay(700);
  int leftDist = getDistance();

  scanServo.write(RIGHT_ANGLE);
  delay(700);
  int rightDist = getDistance();

  scanServo.write(CENTER_ANGLE);
  delay(300);

  lcdPrint("L:" + String(leftDist) + " R:" + String(rightDist), "Avoiding...");

  if (leftDist < 15 && rightDist < 15) {

  lcdPrint("Path Blocked", "Reversing");

  motorBackward();
  delay(600);

  motorStop();
  delay(200);

  return;
}

if (leftDist >= rightDist) {
  avoidLeft();
}
else {
  avoidRight();
}

findLineAfterAvoid();

}



// ==================================================
// ULTRASONIC DISTANCE
// ==================================================

int getDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long dur = pulseIn(ECHO_PIN, HIGH, 25000);
  if (dur == 0) return 100;

  int dist = dur * 0.034 / 2;
  return (dist == 0) ? 100 : dist;
}

// ==================================================
// BUZZER
// ==================================================
void beepTwice() {
  for (int i = 0; i < 2; i++) {
    digitalWrite(BUZZER_PIN, HIGH); delay(200);
    digitalWrite(BUZZER_PIN, LOW);  delay(200);
  }
}

// ==================================================
// LOST LINE SEARCH
// ==================================================
void searchLine() {

  if (lastTurn == 1) {

    motorTurnLeft();
    delay(80);

    if (digitalRead(L_S) == BLACK ||
        digitalRead(R_S) == BLACK)
      return;
  }

  else if (lastTurn == 2) {

    motorTurnRight();
    delay(80);

    if (digitalRead(L_S) == BLACK ||
        digitalRead(R_S) == BLACK)
      return;
  }

  else {

    motorTurnLeft();
    delay(120);

    if (digitalRead(L_S) == BLACK ||
        digitalRead(R_S) == BLACK)
      return;

    motorTurnRight();
    delay(180);

    if (digitalRead(L_S) == BLACK ||
        digitalRead(R_S) == BLACK)
      return;
  }
}
// ==================================================
// FIND LINE AFTER OBSTACLE AVOID
// ==================================================
void findLineAfterAvoid() {
  lcdPrint("Finding Line...", "");
  unsigned long start = millis();

  while (millis() - start < 5000) {
    int L = digitalRead(L_S);
    int R = digitalRead(R_S);

    if (L == BLACK || R == BLACK) {
      motorStop();
      delay(150);
      lcdPrint("Line Found!", "Resume Patrol");
      delay(800);
      return;
    }

    motorTurnLeft();  delay(150);

    L = digitalRead(L_S);
    R = digitalRead(R_S);
    if (L == BLACK || R == BLACK) {
      motorStop(); delay(150); return;
    }

    motorTurnRight(); delay(250);
  }

  motorStop();
  lcdPrint("Line Not Found", "Bot Stopped");
  delay(1000);
}

// ==================================================
// AVOID MANEUVERS
// ==================================================
void avoidLeft() {
  motorBackward(); delay(250);
  motorTurnLeft(); delay(500);
  motorForward();  delay(750);
  motorTurnRight();delay(450);
  motorStop();     delay(100);
}

void avoidRight() {
  motorBackward(); delay(250);
  motorTurnRight();delay(500);
  motorForward();  delay(750);
  motorTurnLeft(); delay(450);
  motorStop();     delay(100);
}

// ==================================================
// LCD HELPER
// ==================================================
void lcdPrint(String line0, String line1) {
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print(line0);
  lcd.setCursor(0, 1); lcd.print(line1);
  lcdLine0 = line0;
  lcdLine1 = line1;
}

void lcdUpdate(String line0, String line1) {
  if (line0 != lcdLine0) {
    lcd.setCursor(0, 0);
    lcd.print(line0 + "                ");
    lcd.setCursor(0, 0);
    lcd.print(line0);
    lcdLine0 = line0;
  }
  if (line1 != lcdLine1) {
    lcd.setCursor(0, 1);
    lcd.print(line1 + "                ");
    lcd.setCursor(0, 1);
    lcd.print(line1);
    lcdLine1 = line1;
  }
}

// ==================================================
// MOTOR FUNCTIONS
// ==================================================
void motorForward() {
  analogWrite(enA, MOTOR_SPEED);
  analogWrite(enB, MOTOR_SPEED);
  digitalWrite(in1, LOW);  digitalWrite(in2, HIGH);
  digitalWrite(in3, HIGH); digitalWrite(in4, LOW);
}

void motorBackward() {
  analogWrite(enA, MOTOR_SPEED);
  analogWrite(enB, MOTOR_SPEED);
  digitalWrite(in1, HIGH); digitalWrite(in2, LOW);
  digitalWrite(in3, LOW);  digitalWrite(in4, HIGH);
}

void motorTurnLeft() {
  analogWrite(enA, TURN_SPEED);
  analogWrite(enB, TURN_SPEED);
  digitalWrite(in1, HIGH); digitalWrite(in2, LOW);
  digitalWrite(in3, HIGH); digitalWrite(in4, LOW);
}

void motorTurnRight() {
  analogWrite(enA, TURN_SPEED);
  analogWrite(enB, TURN_SPEED);
  digitalWrite(in1, LOW);  digitalWrite(in2, HIGH);
  digitalWrite(in3, LOW);  digitalWrite(in4, HIGH);
}

void motorStop() {
  analogWrite(enA, 0);
  analogWrite(enB, 0);
  digitalWrite(in1, LOW); digitalWrite(in2, LOW);
  digitalWrite(in3, LOW); digitalWrite(in4, LOW);
}