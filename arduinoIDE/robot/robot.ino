#include <string>
#include "BluetoothSerial.h"

const String BL_PAIRING_NAME = "StokesTheorem";

const int R_MOTOR_1 = 26;
const int R_MOTOR_2 = 25;

const int L_MOTOR_1 = 33;
const int L_MOTOR_2 = 32;

const int F_MOTOR_1 = 18;
const int F_MOTOR_2 = 19;
const int F_MOTOR_PWM = 21;

const bool R_MOTOR_INVERTED = true;
const bool L_MOTOR_INVERTED = false;
const bool F_MOTOR_INVERTED = false;

const bool TURNING_INVERTED = true;

const float DRIVE_DEADBAND = 0.01;
const float FLYWHEEL_DEADBAND = 0.005;

const int MAX_INPUT_COUNT = 64;


BluetoothSerial SerialBT;

void setup() {
  Serial.begin(115200);
  delay(2000); // wait for it to start up

  SerialBT.begin(BL_PAIRING_NAME);
  Serial.println("Bluetooth open, paring name : " + BL_PAIRING_NAME);


  pinMode(R_MOTOR_1, OUTPUT);
  pinMode(R_MOTOR_2, OUTPUT);

  pinMode(L_MOTOR_1, OUTPUT);
  pinMode(L_MOTOR_2, OUTPUT);

  pinMode(F_MOTOR_1, OUTPUT);
  pinMode(F_MOTOR_2, OUTPUT);
  pinMode(F_MOTOR_PWM, OUTPUT);
}

void parse_command(String data) {
  String valStr = data.substring(2);

  if (data.charAt(0) == 'd'){
    data = data.substring(2);
    int commaIndex = data.indexOf(',');
    String dStr = data.substring(0, commaIndex);
    String tStr = data.substring(commaIndex + 1);

    // Serial.println("d:"+dStr + "  t:"+tStr);
    float drive = dStr.toFloat();
    float turning = tStr.toFloat();

    if (TURNING_INVERTED){
      turning *= -1;
    }

    // Serial.println("Driving, drive:" + String(drive) + ", turning:" + String(turning));

    driveMotor(R_MOTOR_1, R_MOTOR_2, drive + turning, R_MOTOR_INVERTED);
    driveMotor(L_MOTOR_1, L_MOTOR_2, drive - turning, L_MOTOR_INVERTED);
  } 
  else if (data.charAt(0) == 'f') {
    int commaIndex = data.indexOf(',');
    String sStr = data.substring(commaIndex + 1);
    float speed = sStr.toFloat();

    flywheelMotor(
      F_MOTOR_1,
      F_MOTOR_2,
      F_MOTOR_PWM,
      speed,
      F_MOTOR_INVERTED
    );
  }
}

int driveHelper(float& x, bool inverted) {
  if (inverted) {
    x *= -1;
  }
  x = constrain(x, -1.0, 1.0);

  return abs(x) * 255;
}

void flywheelMotor(int in1, int in2, int pwm, float x, bool inverted) {
  int speed = driveHelper(x, inverted);

  if (x > FLYWHEEL_DEADBAND) {
    analogWrite(pwm, speed);
    digitalWrite(in1, HIGH);
    digitalWrite(in2, LOW);
  } else if (x < -FLYWHEEL_DEADBAND) {
    analogWrite(pwm, speed);
    digitalWrite(in1, LOW);
    digitalWrite(in2, HIGH);
  } else { // coast
    analogWrite(pwm, 0);
    digitalWrite(in1, LOW);
    digitalWrite(in2, LOW);
  }
}

void driveMotor(int in1, int in2, float x, bool inverted) {
  int speed = driveHelper(x, inverted);

  if (x > DRIVE_DEADBAND) {
    analogWrite(in1, speed);
    digitalWrite(in2, LOW);
  } else if (x < -DRIVE_DEADBAND) {
    digitalWrite(in1, LOW);
    analogWrite(in2, speed);
  } else { // coast
    // this took a while to debug, you need to reset before getting out of pwn mode
    analogWrite(in1, 0);
    analogWrite(in2, 0);
    analogWrite(in1, LOW);
    digitalWrite(in2, LOW);
  }
}

void loop() {
  char inputBuffer[MAX_INPUT_COUNT];
  int inputIndex = 0;
  char c;

  // if (false) {
  while (SerialBT.available()) {
    char c = SerialBT.read();

    if (c == '\n') {
      inputBuffer[inputIndex] = '\0'; // this is the null terminator, determines the end of a string
      inputIndex = 0;
      String in = String(inputBuffer);
      in.trim();
      // Serial.println("Over Bluetooth : [" + in + "]");
      // SerialBT.println("Received : " + in);

      Serial.println(in + " <- in");
      // connection testing
      if (in.equals("connect from python")) {
        Serial.println("Successfully Connected to Bluetooth Transmitter Python Program");
        SerialBT.println("connected from esp");
      }

      // we got a command
      if (in.charAt(1) == ',') {
        // Serial.println("Recived Command : " + in.charAt(0));
        parse_command(in);
      }
    } else if (inputIndex < MAX_INPUT - 1) {
      inputBuffer[inputIndex++] = c;
    } else {
      inputIndex = 0;
    }
  }
  delay(5);
}