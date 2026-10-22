// Pin assignment
const uint8_t PIN_LED      = 2;
const uint8_t PIN_MOTOR_A  = 27;
const uint8_t PIN_MOTOR_B  = 26;
const uint8_t PIN_MOTOR_EN = 12;
const uint8_t PIN_TACHO    = 13;

// Parameter PWM ESP32
const uint32_t PWM_FREQ = 30000;
const uint8_t  PWM_CHAN = 0;
const uint8_t  PWM_RES  = 8;
const uint8_t  SPEED_PWM = 150; // Kecepatan motor (0-255)

volatile uint32_t pulseCount = 0;

void setMotorSpeed(uint8_t speed, bool directionForward) {
  digitalWrite(PIN_MOTOR_A, directionForward ? HIGH : LOW);
  digitalWrite(PIN_MOTOR_B, directionForward ? LOW : HIGH);
  ledcWrite(PWM_CHAN, speed);
}

void setup() {
  Serial.begin(115200);

  // Inisialisasi Pin
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_MOTOR_A, OUTPUT);
  pinMode(PIN_MOTOR_B, OUTPUT);
  pinMode(PIN_MOTOR_EN, OUTPUT);
  pinMode(PIN_TACHO, INPUT_PULLUP);

  // Konfigurasi PWM
  ledcSetup(PWM_CHAN, PWM_FREQ, PWM_RES);
  ledcAttachPin(PIN_MOTOR_EN, PWM_CHAN);

  // Matikan sistem awal
  digitalWrite(PIN_LED, LOW);
  setMotorSpeed(0, true);

  Serial.println("[System Ready] Controller active.");
}

void loop() {
  if (Serial.available() > 0) {
    char command = Serial.read();

    switch (command) {
      case '1':
        digitalWrite(PIN_LED, HIGH);
        setMotorSpeed(SPEED_PWM, true);
        Serial.println("State: ACTIVE (LED & Motor ON)");
        break;

      case '0':
        digitalWrite(PIN_LED, LOW);
        setMotorSpeed(0, true);
        Serial.println("State: INACTIVE (LED & Motor OFF)");
        break;
        
      default:
        // Mengabaikan karakter newline/carriage return
        break;
    }
  }
}
