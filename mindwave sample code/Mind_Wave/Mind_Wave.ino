////////////////////////////////////////////////////////////////////////
// Arduino Interface with Mindwave
// 
// This is example code provided by NeuroSky, Inc. and is provided
// license free.
////////////////////////////////////////////////////////////////////////

#define LED 13
#define BAUDRATE 57600
#define DEBUGOUTPUT 0

#define GREENLED1  3
#define GREENLED2  4
#define GREENLED3  5
#define YELLOWLED1 6
#define YELLOWLED2 7
#define YELLOWLED3 8
#define YELLOWLED4 9
#define REDLED1    10
#define REDLED2    11
#define REDLED3    12

// checksum variables
byte generatedChecksum = 0;
byte checksum = 0; 
int payloadLength = 0;
byte payloadData[64] = {0};
byte poorQuality = 0;
byte attention = 0;
byte meditation = 0;

// system variables
long lastReceivedPacket = 0;
boolean bigPacket = false;

//////////////////////////
// Microprocessor Setup //
//////////////////////////
void setup() {
  pinMode(GREENLED1, OUTPUT);
  pinMode(GREENLED2, OUTPUT);
  pinMode(GREENLED3, OUTPUT);
  pinMode(YELLOWLED1, OUTPUT);
  pinMode(YELLOWLED2, OUTPUT);
  pinMode(YELLOWLED3, OUTPUT);
  pinMode(YELLOWLED4, OUTPUT);
  pinMode(REDLED1, OUTPUT);
  pinMode(REDLED2, OUTPUT);
  pinMode(REDLED3, OUTPUT);


  pinMode(LED, OUTPUT);
  Serial.begin(BAUDRATE);           // USB

  delay(3000);
  Serial.write(194);


  // Replaced Serial.print(194,BYTE) with Serial.write(194)
}

////////////////////////////////
// Read data from Serial UART //
////////////////////////////////
byte ReadOneByte() {
  while(!Serial.available());
  return Serial.read();
}

/////////////
//MAIN LOOP//
/////////////
void loop() {
  // Look for sync bytes
  if(ReadOneByte() == 0xAA) {
    if(ReadOneByte() == 0xAA) {
      payloadLength = ReadOneByte();
      if(payloadLength > 169) {     // Payload length can not be greater than 169
        return;
      }

      generatedChecksum = 0;        
      for(int i = 0; i < payloadLength; i++) {  
        payloadData[i] = ReadOneByte();    // Read payload into memory
        generatedChecksum += payloadData[i];
      }   

      checksum = ReadOneByte();            // Read checksum byte from stream      
      generatedChecksum = 255 - generatedChecksum; // Take one's compliment of generated checksum

      if(checksum == generatedChecksum) {    
        poorQuality = 200;
        attention = 0;
        meditation = 0;

        for(int i = 0; i < payloadLength; i++) {    // Parse the payload
          switch (payloadData[i]) {
            case 2:
              i++;            
              poorQuality = payloadData[i];
              bigPacket = true;            
              break;
            case 4:
              i++;
              attention = payloadData[i];                        
              break;
            case 5:
              i++;
              meditation = payloadData[i];
              break;
            case 0x80:
              i += 3;
              break;
            case 0x83:
              i += 25;      
              break;
            default:
              break;
          }
        }

        if(bigPacket) {
          if(poorQuality == 0)
            digitalWrite(LED, HIGH);
          else
            digitalWrite(LED, LOW);
            
          Serial.print("PoorQuality: ");
          Serial.print(poorQuality);
          Serial.print(" Attention: ");
          Serial.print(attention);
          
          if(attention > 40)
            digitalWrite(GREENLED1, HIGH);
          else
            digitalWrite(GREENLED1, LOW);

          Serial.print(" Time since last packet: ");
          Serial.print(millis() - lastReceivedPacket);
          lastReceivedPacket = millis();
          Serial.println();

          // Update LED display based on attention level
          updateLEDs(attention);
        }
        bigPacket = false;        
      }
    }
  }
}

// Helper function to update LEDs based on attention value
void updateLEDs(byte attentionValue) {
  // Turn all LEDs off first
  for (int i = GREENLED1; i <= REDLED3; i++) {
    digitalWrite(i, LOW);
  }

  // Determine which LEDs to turn on based on attention value
  int level = attentionValue / 10;
  
  if (level >= 1) digitalWrite(GREENLED1, HIGH);
  if (level >= 2) digitalWrite(GREENLED2, HIGH);
  if (level >= 3) digitalWrite(GREENLED3, HIGH);
  if (level >= 4) digitalWrite(YELLOWLED1, HIGH);
  if (level >= 5) digitalWrite(YELLOWLED2, HIGH);
  if (level >= 6) digitalWrite(YELLOWLED3, HIGH);
  if (level >= 7) digitalWrite(YELLOWLED4, HIGH);
  if (level >= 8) digitalWrite(REDLED1, HIGH);
  if (level >= 9) digitalWrite(REDLED2, HIGH);
  if (level >= 10) digitalWrite(REDLED3, HIGH);
}
