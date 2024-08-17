#include <Arduino.h>
#include <Servo.h>

#include <IRremote.hpp>  // include the library

#define MY_PLAYER_NUM 1  // my player number
#define FX_DURATION 100  // the duration of my shot's FX
#define INVINCIBLE_WINDOW \
  2000  // how long I am invincible for after being shot or putting up my shield
#define SHOOT_COOLDOWN 1000  // how long it takes before I can fire another shot
#define DEFAULT_SHIELD_POS \
  0  // the postion of the shield when it is all the way down
#define SHIELD_SPEED \
  0.005  // the speed at which my shield falls back down to default position
#define SHIELD_COOLDOWN \
  3000  // how long it takes before I can lift my shield back up

// declare function prototypes
int decodeSignal();
void iShotFX(bool startFX);
void deductLife();
void displayLives();
void playLossTune();
void moveShield();

// declare global variables here
Servo servo;

bool FXIsOn = false;
unsigned long previousMillis = 0;  // start time: record when we start FX
unsigned long currentMillis;  // current time: current - start = time elapsed
unsigned long previousMillisInvincible;  // start time: invincibility frames
unsigned long previousMillisShot;        // start time: last shot fired
unsigned long previousMillisShield;      // start time: last shield raised

const int ledYellowPin = 2;
const int ledRedPin1 = 9;
const int ledRedPin2 = 10;
const int ledRedPin3 = 11;
const int buzzerPin = 5;
const int receiverPin = 12;
const int buttonPin = 13;
const int servoPin = 6;
int lives = 3;
double shieldPos = 0;

void setup() {
  Serial.begin(115200);
  while (!Serial);  // Wait for Serial to become available. Is optimized away
                    // for some cores.

  // Start the receiver and if not 3. parameter specified, take LED_BUILTIN pin
  // from the internal boards definition as default feedback LED
  IrReceiver.begin(receiverPin, ENABLE_LED_FEEDBACK);
  servo.attach(servoPin);
  pinMode(ledYellowPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  pinMode(ledRedPin1, OUTPUT);
  pinMode(ledRedPin2, OUTPUT);
  pinMode(ledRedPin3, OUTPUT);
  pinMode(buttonPin, INPUT);
}

void loop() {
  // take the current time
  currentMillis = millis();
  // decode incoming IR signals
  int signal = decodeSignal();
  // if the incoming signal is from me, try to shoot
  bool shotFX = false;
  // if my shield is down and my gun is not on cooldown, fire
  if (signal == MY_PLAYER_NUM &&
      currentMillis - previousMillisShot > SHOOT_COOLDOWN &&
      shieldPos <= DEFAULT_SHIELD_POS) {
    previousMillisShot = currentMillis;
    shotFX = true;
  }
  iShotFX(shotFX);  // either fire a new shot or update the FX duration on the
                    // current shot
  // if the incoming signal is from an enemy player, try to deduct a life point
  if (signal == 2 || signal == 3) {
    deductLife();
  }
  // display the amount of lives left
  displayLives();
  // update the shield; either lift it up or slowly fall back down
  moveShield();
}

// place function definitions

// decode the incoming IR signal.
int decodeSignal() {
  if (IrReceiver.decode()) {
    /*
     * Print a summary of received data
     */
    if (IrReceiver.decodedIRData.protocol == UNKNOWN) {
      // Serial.println(F("Received noise or an unknown (or not yet enabled)
      // protocol")); We have an unknown protocol here, print extended info
      // IrReceiver.printIRResultRawFormatted(&Serial, true);
      IrReceiver.resume();  // Do it here, to preserve raw data for printing
                            // with printIRResultRawFormatted()
    } else {
      IrReceiver.resume();  // Early enable receiving of the next IR frame
      // IrReceiver.printIRResultShort(&Serial);
      // IrReceiver.printIRSendUsage(&Serial);
    }
    Serial.println();
    /*
     * Finally, check the received data and perform actions according to the
     * received command
     */
    switch (IrReceiver.decodedIRData.command) {
      case 0xC:
        Serial.println("signal received: 1");
        return 1;
        break;
      case 0x18:
        Serial.println("signal received: 2");
        return 2;
        break;
      case 0x5E:
        Serial.println("signal received: 3");
        return 3;
        break;
      default:
        Serial.println("signal received: Don't Care");
        return 0;
    }
  } else {
    return 0;
  }
}

// restart the current FX (play sound and light LED) or update its remaining
// duration and stop when done
void iShotFX(bool startFX) {
  // the shoot button was just pressed and no sound is playing yet
  if (startFX && !FXIsOn) {
    digitalWrite(buzzerPin, HIGH);
    digitalWrite(ledYellowPin, HIGH);
    previousMillis = currentMillis;
    FXIsOn = true;
  }
  // sound has played long enough
  if (FXIsOn && currentMillis - previousMillis > FX_DURATION) {
    digitalWrite(buzzerPin, LOW);
    digitalWrite(ledYellowPin, LOW);
    FXIsOn = false;
  }
}

// if I am not invincible, deduct a hitpoint.
void deductLife() {
  if (currentMillis - previousMillisInvincible > INVINCIBLE_WINDOW) {
    lives--;
    previousMillisInvincible = currentMillis;
  }
}

// display the current amount of lives I have left
void displayLives() {
  switch (lives) {
    case 3:
      digitalWrite(ledRedPin1, HIGH);
      digitalWrite(ledRedPin2, HIGH);
      digitalWrite(ledRedPin3, HIGH);
      break;
    case 2:
      digitalWrite(ledRedPin1, HIGH);
      digitalWrite(ledRedPin2, HIGH);
      digitalWrite(ledRedPin3, LOW);
      break;
    case 1:
      digitalWrite(ledRedPin1, HIGH);
      digitalWrite(ledRedPin2, LOW);
      digitalWrite(ledRedPin3, LOW);
      break;
    default:
      digitalWrite(ledRedPin1, LOW);
      digitalWrite(ledRedPin2, LOW);
      digitalWrite(ledRedPin3, LOW);
      playLossTune();
  }
}

// if I have 0 lives left, play this tune
void playLossTune() {
  digitalWrite(buzzerPin, HIGH);
  delay(100);
  digitalWrite(buzzerPin, LOW);
  delay(50);
  digitalWrite(buzzerPin, HIGH);
  delay(100);
  digitalWrite(buzzerPin, LOW);
  delay(50);
  digitalWrite(buzzerPin, HIGH);
  delay(300);
  digitalWrite(buzzerPin, LOW);
  delay(5000);
  lives = 3;
}

// if the button has been pressed and my shield is off cooldown, lift it up.
// otherwise, let shield fall down to its default position
void moveShield() {
  if (digitalRead(buttonPin) == HIGH &&
      currentMillis - previousMillisShield > SHIELD_COOLDOWN) {
    shieldPos = 100;
    previousMillisShield = currentMillis;
    previousMillisInvincible = currentMillis;
  } else {
    if (shieldPos > DEFAULT_SHIELD_POS) {
      shieldPos -= SHIELD_SPEED;
    }
  }
  servo.write((int)shieldPos);
}
