#include <Arduino.h>

/*

   Connect the arduino as follows:
   
   Arduino        --- Flash
   -------            -----
   PA0  (D22)     --- A-1 (DQ15)
   PA1  (D23)     --- A0
   PA2  (D24)     --- A1
   PA3  (D25)     --- A2
   PA4  (D26)     --- A3
   PA5  (D27)     --- A4
   PA6  (D28)     --- A5
   PA7  (D29)     --- A6
   ---
   PC0  (D37)     --- A7
   PC1  (D36)     --- A8
   PC2  (D35)     --- A9
   PC3  (D34)     --- A10
   PC4  (D33)     --- A11
   PC5  (D32)     --- A12
   PC6  (D31)     --- A13
   PC7  (D30)     --- A14
   ---
   PL0  (D49)     --- A15
   PL1  (D48)     --- A16
   PL2  (D47)     --- A17
   PL3  (D46)     --- A18
   PL4  (D45)     --- A19
   PL5  (D44)     --- A20
   PL6  (D43)     --- A21
   PL7  (D42)     --- A22
   ---
   PG0  (D41)     --- A23
   
   PK0  (D62/A8)  --- DQ0
   PK1  (D63/A9)  --- DQ1
   PK2  (D64/A10) --- DQ2
   PK3  (D65/A11) --- DQ3
   PK4  (D66/A12) --- DQ4
   PK5  (D67/A13) --- DQ5
   PK6  (D68/A14) --- DQ6
   PK7  (D69/A15) --- DQ7
   
   PF0  (D54/A0)  --- CE
   PF1  (D55/A1)  --- OE
   PF2  (D56/A2)  --- WE
   PF3  (D57/A3)  --- RESET
   PF4  (D58/A4)  --- WP
   PF5  (D59/A5)  --- BYTE

*/

const int ADDR_PINS[] = {22, 23, 24, 25, 26, 27, 28, 37, 36, 35, 34, 33, 32, 31, 30, 49, 48, 47, 46, 45, 44, 43, 42};
const int DATA_PINS[] = {62, 63, 64, 65, 66, 67, 68, 69};

const int PINS_CE    = 54;
const int PINS_OE    = 55;
const int PINS_WE    = 56;
const int PINS_RESET = 57;
const int PINS_WP    = 58;
const int PINS_BYTE  = 59;

void init_pins();

void flash_set_addr_pins(uint32_t addr);
uint8_t flash_read_byte(uint32_t addr);
void flash_reset();

void setup() {
  init_pins();

  Serial.begin(115200);
  while(!Serial);
}

void loop() {
  Serial.println("Start of flash rom");

  int count = 0;
  for(uint32_t i = 0; i < 0x2000000; i++) {
    Serial.print(flash_read_byte(i), HEX);
    count++;

    if(count < 8) {
      Serial.print(" ");
    } else {
      Serial.print("\n");
      count = 0;
    }
  }
  
  Serial.print("\n");
  Serial.println("End of flash rom");
}

void flash_reset() {
  digitalWrite(PINS_RESET, LOW);
  delay(250);
  digitalWrite(PINS_RESET, HIGH);
  delay(250);
}

void init_pins() {
  // set pin mode for address lines to output
  for (int i = 0; i < 23; i++) {
    pinMode(ADDR_PINS[i], OUTPUT);
  }

  // set pin mode for data lines to input
  for (int i = 0; i < 8; i++) {
    pinMode(DATA_PINS[i], INPUT);
  }

  // set pin mode for control lines to output
  pinMode(PINS_CE, OUTPUT);
  pinMode(PINS_OE, OUTPUT);
  pinMode(PINS_WE, OUTPUT);
  pinMode(PINS_RESET, OUTPUT);
  pinMode(PINS_WP, OUTPUT);
  pinMode(PINS_BYTE, OUTPUT);

  // set initial pin states
  digitalWrite(PINS_CE, HIGH);
  digitalWrite(PINS_OE, HIGH);
  digitalWrite(PINS_WE, HIGH);
  digitalWrite(PINS_RESET, HIGH);
  digitalWrite(PINS_WP, LOW);
  digitalWrite(PINS_BYTE, LOW);

  flash_set_addr_pins(0x0000000);
  flash_reset();
}

void flash_set_addr_pins(uint32_t addr) {
  for (int i = 0; i < 23; i++) {
    digitalWrite(ADDR_PINS[i], (addr >> i) & 0x01);
  }
}

uint8_t flash_read_byte(uint32_t addr) {
  uint8_t data = 0;

  flash_set_addr_pins(addr);
  digitalWrite(PINS_CE, LOW);
  delayMicroseconds(1);
  digitalWrite(PINS_OE, LOW);
  delayMicroseconds(1);

  for (int i = 0; i < 8; i++) {
    data |= digitalRead(DATA_PINS[i]) << i;
  }

  delayMicroseconds(1);
  digitalWrite(PINS_CE, HIGH);
  delayMicroseconds(1);
  digitalWrite(PINS_OE, HIGH);

  return data;
}