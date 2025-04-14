#include <Servo.h>

#define LED_PIN 13
#define RECONNECT_INTERVAL 5000
#define SERVO_COUNT 5

Servo servos[SERVO_COUNT];
const int servoPins[SERVO_COUNT] = {2, 3, 4, 5, 6};

unsigned long lastPacketTime = 0;
int badSignalCount = 0;
bool servoAt180 = false;
bool actionTriggered = false;

void setup() {
  Serial.begin(115200);
  for (int i = 0; i < SERVO_COUNT; i++) {
    servos[i].attach(servoPins[i]);
  }

  resetServos();
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  delay(3000);
  connectHeadset();
  Serial.println("MindWave stabilized. Awaiting EEG data...");
}

void loop() {
  if (readPacket()) {
    lastPacketTime = millis();
  }

  if (millis() - lastPacketTime > RECONNECT_INTERVAL || badSignalCount >= 10) {
    Serial.println("Connection timeout or poor signal. Reconnecting...");
    connectHeadset();
    badSignalCount = 0;
    lastPacketTime = millis();
    resetServos();
    servoAt180 = false;
    actionTriggered = false;
  }
}

bool readPacket() {
  if (Serial.available() >= 2) {
    if (Serial.read() == 0xAA && Serial.read() == 0xAA) {
      while (!Serial.available());
      byte payloadLength = Serial.read();
      if (payloadLength > 169) return false;

      byte payload[256];
      int bytesRead = 0;
      unsigned long startTime = millis();

      while (bytesRead < payloadLength) {
        if (Serial.available()) {
          payload[bytesRead++] = Serial.read();
        }
        if (millis() - startTime > 100) return false;
      }

      while (!Serial.available());
      byte checksum = Serial.read();

      byte computedChecksum = 0;
      for (int i = 0; i < payloadLength; i++) {
        computedChecksum += payload[i];
      }
      computedChecksum = 255 - computedChecksum;

      if (checksum != computedChecksum) {
        return false;
      }

      parsePayload(payload, payloadLength);
      return true;
    }
  }
  return false;
}

void connectHeadset() {
  Serial.write(0xC2); // Auto-connect
  delay(3000);
  byte enableStream[] = { 0xAA, 0x02, 0x01, 0x02 };
  Serial.write(enableStream, sizeof(enableStream));
}

void parsePayload(byte* data, int length) {
  int i = 0;
  byte attention = 0;
  byte meditation = 0;
  bool bigPacket = false;

  while (i < length) {
    byte code = data[i++];
    switch (code) {
      case 0x02: {
        byte poor = data[i++];
        Serial.print("Poor signal: ");
        Serial.println(poor);
        if (poor > 200) {
          badSignalCount++;
        } else {
          badSignalCount = 0;
        }
        bigPacket = true;
        break;
      }
      case 0x04: {
        attention = data[i++];
        Serial.print("Attention: ");
        Serial.println(attention);
        break;
      }
      case 0x05: {
        meditation = data[i++];
        Serial.print("Meditation: ");
        Serial.println(meditation);
        break;
      }
      case 0x80:
        i++; i += 2;
        break;
      case 0x83:
        i++; i += 24;
        break;
      default: {
        byte vlen = data[i++];
        i += vlen;
        break;
      }
    }
  }

  if (bigPacket) {
    digitalWrite(LED_PIN, attention != 0 ? HIGH : LOW);
    controlServos(attention);
  }
}

void controlServos(byte attention) {
  if (attention > 60 && !actionTriggered) {
    if (!servoAt180) {
      servos[0].write(180);
      servos[1].write(0);
      servos[2].write(0);
      servos[3].write(0);
      servos[4].write(180);
      servoAt180 = true;
    } else {
      resetServos();
      servoAt180 = false;
    }
    actionTriggered = true;
    delay(2000);
  }

  if (attention <= 60) {
    actionTriggered = false;
  }
}

void resetServos() {
  servos[0].write(0);
  servos[1].write(180);
  servos[2].write(180);
  servos[3].write(180);
  servos[4].write(0);
}
