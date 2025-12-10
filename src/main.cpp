#include <Arduino.h>
#include <SPI.h>

// --- YOUR SPECIFIC PIN MAPPING (From your photo) ---
const int PIN_DIN = 7;  // MOSI
const int PIN_CLK = 18; // SCK
const int PIN_CS = 5;   // Chip Select
const int PIN_DC = 17;  // Data/Command
const int PIN_RST = 16; // Reset
const int PIN_BUSY = 4; // Busy

// --- LOW LEVEL HARDWARE FUNCTIONS ---

void SPI_Write(unsigned char value)
{
    SPI.transfer(value);
}

void EPD_W21_WriteCMD(unsigned char command)
{
    digitalWrite(PIN_CS, LOW);
    digitalWrite(PIN_DC, LOW); // LOW = Command
    SPI_Write(command);
    digitalWrite(PIN_CS, HIGH);
}

void EPD_W21_WriteDATA(unsigned char data)
{
    digitalWrite(PIN_CS, LOW);
    digitalWrite(PIN_DC, HIGH); // HIGH = Data
    SPI_Write(data);
    digitalWrite(PIN_CS, HIGH);
}

// Wait for the screen to finish its internal operation
void EPD_CheckStatus(void)
{
    // On SSD1683: High = Busy, Low = Idle
    while (digitalRead(PIN_BUSY) == HIGH)
    {
        delay(1);
    }
}

// --- MANUFACTURER INITIALIZATION SEQUENCE (From your GitHub link) ---
void LU32_4D2_BWR_400X300_SSD1683_Init(void)
{

    EPD_CheckStatus(); // Wait just in case

    EPD_W21_WriteCMD(0x12); // Soft Reset
    EPD_CheckStatus();      // Wait for reset to complete

    EPD_W21_WriteCMD(0x21); // Display Update Control 1
    EPD_W21_WriteDATA(0x40);
    EPD_W21_WriteDATA(0x00);

    EPD_W21_WriteCMD(0x3C);  // Border Waveform
    EPD_W21_WriteDATA(0x05); // 0x05 usually sets border to white

    EPD_W21_WriteCMD(0x11);  // Data Entry Mode
    EPD_W21_WriteDATA(0x03); // X increment, Y increment

    EPD_W21_WriteCMD(0x44);  // Set X Ram Window
    EPD_W21_WriteDATA(0x00); // Start 0x00
    EPD_W21_WriteDATA(0x31); // End 0x31 (49 decimal -> 50 bytes * 8 = 400 pixels)

    EPD_W21_WriteCMD(0x45);  // Set Y Ram Window
    EPD_W21_WriteDATA(0x00); // Start 0
    EPD_W21_WriteDATA(0x00);
    EPD_W21_WriteDATA(0x2B); // End 0x12B (299 decimal -> 300 pixels)
    EPD_W21_WriteDATA(0x01);

    EPD_W21_WriteCMD(0x4E); // Set X Counter
    EPD_W21_WriteDATA(0x00);

    EPD_W21_WriteCMD(0x4F); // Set Y Counter
    EPD_W21_WriteDATA(0x00);
    EPD_W21_WriteDATA(0x00);
}

// --- MAIN SETUP ---
void setup()
{
    Serial.begin(115200);
    delay(1000);
    Serial.println("--- BARE METAL E-INK TEST ---");

    // 1. Setup Pins
    pinMode(PIN_CS, OUTPUT);
    pinMode(PIN_DC, OUTPUT);
    pinMode(PIN_RST, OUTPUT);
    pinMode(PIN_BUSY, INPUT);

    // 2. Setup SPI (Your Cluster Pins)
    SPI.end();
    SPI.begin(PIN_CLK, -1, PIN_DIN, PIN_CS);
    SPI.beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE0));

    // 3. Hardware Reset (Critical!)
    Serial.println("Resetting Display...");
    digitalWrite(PIN_RST, LOW);
    delay(20); // Hold reset
    digitalWrite(PIN_RST, HIGH);
    delay(200); // Wait for boot

    // 4. Run Manufacturer Init
    Serial.println("Initializing Controller...");
    LU32_4D2_BWR_400X300_SSD1683_Init();

    // 5. Send Image Data
    // Resolution: 400 x 300
    // Bytes needed: (400 * 300) / 8 = 15,000 bytes per color layer

    Serial.println("Sending Black Data...");
    EPD_W21_WriteCMD(0x24); // Write RAM (Black/White)
    for (int i = 0; i < 15000; i++)
    {
        // Send a checkerboard pattern (0xAA = 10101010)
        EPD_W21_WriteDATA(0xAA);
    }

    Serial.println("Sending Red Data...");
    EPD_W21_WriteCMD(0x26); // Write RAM (Red)
    for (int i = 0; i < 15000; i++)
    {
        // Send stripes (0xF0 = 11110000)
        // Note: On Red layer, 0 = Red, 1 = Transparent/White
        EPD_W21_WriteDATA(0xF0);
    }

    // 6. Trigger Refresh
    Serial.println("Triggering Refresh (This takes 15s)...");
    EPD_W21_WriteCMD(0x20); // Master Activation
    EPD_CheckStatus();      // Block until finished

    Serial.println("Done! Deep Sleep.");

    // 7. Sleep Display
    EPD_W21_WriteCMD(0x10); // Deep Sleep Mode
    EPD_W21_WriteDATA(0x01);
}

void loop()
{
    // Do nothing
}