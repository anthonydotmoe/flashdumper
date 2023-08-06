#include <Arduino.h>
#include <SPI.h>
#include <Ethernet.h>

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

byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
IPAddress ip(192, 168, 6, 177);
EthernetServer server(80);

const uint32_t FLASH_SIZE = 0x200000;

const int ADDR_PINS[] = {22, 23, 24, 25, 26, 27, 28, 29, 37, 36, 35, 34, 33, 32, 31, 30, 49, 48, 47, 46, 45, 44, 43, 42, 41};
const int DATA_PINS[] = {62, 63, 64, 65, 66, 67, 68, 69};
const int NUM_ADDR_PINS = sizeof(ADDR_PINS) / sizeof(ADDR_PINS[0]);

const int PINS_CE    = 54;
const int PINS_OE    = 55;
const int PINS_WE    = 56;
const int PINS_RESET = 57;
const int PINS_WP    = 58;
const int PINS_BYTE  = 59;

void init_pins();

void flash_set_addr_pins(uint32_t addr);
void flash_read_page(uint32_t address, uint8_t *buffer, size_t length);
uint8_t flash_read_byte(uint32_t addr);
void flash_reset();

void setup() {
  init_pins();

  Serial.begin(115200);
  while(!Serial);
  
  Ethernet.begin(mac, ip);
}

void send_flash_data(EthernetClient &client, const uint32_t start_address, const uint32_t size) {
  const size_t page_size = 16; // 16 bytes for the S29GL256P
  uint8_t buffer[page_size];

  // Send flash content
  for (uint32_t address = start_address; address < size; address += page_size) {
    flash_read_page(address, buffer, page_size);
    client.write(buffer, page_size);
  }
}

void loop() {
  EthernetClient client = server.available(); // Check if a client has connected
  if (client) {
    bool request_line_saved = false;
    Serial.println("Client connected");

    String currentLine = "";
    String request = "";
    bool isHeaderEnd = false;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        if (c == '\n') {
          if(!request_line_saved) {
            request = currentLine;
            request_line_saved = true;
            Serial.println(request);
          }
          if (currentLine.length() == 0) { // End of HTTP headers
            isHeaderEnd = true;
          }
          currentLine = "";
        } else if (c != '\r') {
          currentLine += c;
        }
      }

      if (isHeaderEnd) {
        if (request.startsWith("GET /flash.bin")) {
          Serial.println("Sending flash contents");
          // Send the flash data with the appropriate headers
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: application/octet-stream");
          client.println("Connection: close");
          client.print("Content-Length: ");
          client.println(FLASH_SIZE); // Set this to the actual flash size
          client.println();
          send_flash_data(client, 0, FLASH_SIZE); // Set this to the actual flash size
        } else {
          // Send the HTML page with the download link
          Serial.println("Sending homepage");
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Connection: close");
          client.println();

          client.println("<!DOCTYPE HTML>");
          client.println("<html>");
          client.println("<head>");
          client.println("<title>Flash Download</title>");
          client.println("</head>");
          client.println("<body>");
          client.println("<h1>Download Flash Contents</h1>");
          client.println("<p><a href=\"/flash.bin\" download>Download Flash Data</a></p>");
          client.println("</body>");
          client.println("</html>");
        }
        break;
      }
    }

    delay(1); // Give the client time to receive the data
    client.stop(); // Close the connection
    Serial.println("Client disconnected");
  }
}

void flash_reset() {
  digitalWrite(PINS_RESET, LOW);
  delay(250);
  digitalWrite(PINS_RESET, HIGH);
  delay(250);
}

void init_pins() {
  // set pin mode for address lines to output
  for (int i = 0; i < NUM_ADDR_PINS; i++) {
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
  addr ^= 1; // Correct endianness since we're using byte mode
  PORTA = (PORTA & 0x03) | ((addr & 0xFF) << 2);
  PORTC = (addr >> 8) & 0xFF;
  PORTL = (addr >> 16) & 0xFF;
  PORTG = (addr >> 24) & 0x01;
}

uint8_t flash_read_byte(uint32_t addr) {
  uint8_t data = 0;

  flash_set_addr_pins(addr);
  digitalWrite(PINS_CE, LOW);
//  delayMicroseconds(1);
  digitalWrite(PINS_OE, LOW);
//  delayMicroseconds(7);

  for (int i = 0; i < 8; i++) {
    data |= digitalRead(DATA_PINS[i]) << i;
  }

//  delayMicroseconds(1);
  digitalWrite(PINS_CE, HIGH);
//  delayMicroseconds(1);
  digitalWrite(PINS_OE, HIGH);

  return data;
}

void flash_read_page(uint32_t address, uint8_t *buffer, size_t length) {
  PORTF &= ~_BV(PF0); // CE low
  PORTF &= ~_BV(PF1); // OE low

  // Set the higher address bits (A23 to A3)
  uint32_t pageAddress = address & 0xFFFFF8;
  flash_set_addr_pins(pageAddress);

  // Read the specified number of bytes into the buffer
  for (size_t i = 0; i < length; i++) {
    // Set the lower address bits (A2 to A-1)
    uint32_t intraPageAddress = (address + i);
    flash_set_addr_pins(intraPageAddress);

    delayMicroseconds(1);
    // Read the byte
    buffer[i] = PINK;
  }

  PORTF |= _BV(PF0); // CE high
  PORTF |= _BV(PF1); // OE high
}