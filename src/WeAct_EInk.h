#ifndef WEACT_EINK_H
#define WEACT_EINK_H

#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>

#define EINK_WIDTH 400
#define EINK_HEIGHT 300

// Standard Colors
#define EINK_BLACK 0x0000
#define EINK_WHITE 0xFFFF
#define EINK_RED 0xF800

class WeAct42_Driver : public Adafruit_GFX
{
public:
    uint8_t *blackBuffer;
    uint8_t *redBuffer;
    int8_t _cs, _dc, _rst, _busy, _clk, _din;

    WeAct42_Driver(int8_t cs, int8_t dc, int8_t rst, int8_t busy, int8_t clk, int8_t din)
        : Adafruit_GFX(EINK_WIDTH, EINK_HEIGHT), _cs(cs), _dc(dc), _rst(rst), _busy(busy), _clk(clk), _din(din)
    {
        blackBuffer = (uint8_t *)malloc(EINK_WIDTH * EINK_HEIGHT / 8);
        redBuffer = (uint8_t *)malloc(EINK_WIDTH * EINK_HEIGHT / 8);
    }

    void begin()
    {
        pinMode(_cs, OUTPUT);
        pinMode(_dc, OUTPUT);
        pinMode(_rst, OUTPUT);
        pinMode(_busy, INPUT);
        SPI.end();
        SPI.begin(_clk, -1, _din, _cs);
        SPI.beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE0));
        clearBuffer();
    }

    void spiWrite(uint8_t v) { SPI.transfer(v); }
    void writeCMD(uint8_t c)
    {
        digitalWrite(_cs, LOW);
        digitalWrite(_dc, LOW);
        spiWrite(c);
        digitalWrite(_cs, HIGH);
    }
    void writeDATA(uint8_t d)
    {
        digitalWrite(_cs, LOW);
        digitalWrite(_dc, HIGH);
        spiWrite(d);
        digitalWrite(_cs, HIGH);
    }

    void waitBusy()
    {
        unsigned long start = millis();
        while (digitalRead(_busy) == HIGH)
        {
            if (millis() - start > 10000)
                break;
            delay(1);
        }
    }

    void hardwareInit()
    {
        digitalWrite(_rst, LOW);
        delay(20);
        digitalWrite(_rst, HIGH);
        delay(200);
        waitBusy();
        writeCMD(0x12);
        waitBusy();

        // CRITICAL FIX: Enable Red RAM (0x00 instead of 0x40)
        writeCMD(0x21);
        writeDATA(0x00);
        writeDATA(0x00);

        writeCMD(0x3C);
        writeDATA(0x05);
        writeCMD(0x11);
        writeDATA(0x03);
        writeCMD(0x44);
        writeDATA(0x00);
        writeDATA(0x31);
        writeCMD(0x45);
        writeDATA(0x00);
        writeDATA(0x00);
        writeDATA(0x2B);
        writeDATA(0x01);
        writeCMD(0x4E);
        writeDATA(0x00);
        writeCMD(0x4F);
        writeDATA(0x00);
        writeDATA(0x00);
    }

    // --- FINAL PIXEL LOGIC ---
    // Black Buffer: 0 = Ink (Black), 1 = Paper (White)
    // Red Buffer:   1 = Ink (Red),   0 = Paper (Transparent)
    void drawPixel(int16_t x, int16_t y, uint16_t color)
    {
        if ((x < 0) || (x >= width()) || (y < 0) || (y >= height()))
            return;
        uint16_t byteIdx = (x + y * EINK_WIDTH) / 8;
        uint8_t bitMask = 0x80 >> (x % 8);

        if (color == EINK_BLACK)
        {
            // Black: Set Black to 0, Red to 0
            blackBuffer[byteIdx] &= ~bitMask;
            redBuffer[byteIdx] &= ~bitMask;
        }
        else if (color == EINK_RED)
        {
            // Red: Set Black to 1 (White), Red to 1 (Red)
            // Note: Black must be "White" (1) for Red to show
            blackBuffer[byteIdx] |= bitMask;
            redBuffer[byteIdx] |= bitMask;
        }
        else
        {
            // White: Set Black to 1, Red to 0
            blackBuffer[byteIdx] |= bitMask;
            redBuffer[byteIdx] &= ~bitMask;
        }
    }

    void display()
    {
        hardwareInit();
        writeCMD(0x24);
        for (int i = 0; i < 15000; i++)
            writeDATA(blackBuffer[i]);
        writeCMD(0x26);
        for (int i = 0; i < 15000; i++)
            writeDATA(redBuffer[i]);
        writeCMD(0x20);
        waitBusy();
        writeCMD(0x10);
        writeDATA(0x01);
    }

    // Clear to White: Black=1, Red=0
    void clearBuffer()
    {
        memset(blackBuffer, 0xFF, 15000);
        memset(redBuffer, 0x00, 15000);
    }
};
#endif