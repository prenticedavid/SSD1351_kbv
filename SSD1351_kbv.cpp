/* I have stolen large sections from Adafruit_SSD1351
 * and just complied wih XXX_kbv methods
 * principally this means using the hardware for ALL rotations
 */

/***************************************************
  This is a library for the 1.5" & 1.27" 16-bit Color OLEDs
  with SSD1331 driver chip

  Pick one up today in the adafruit shop!
  ------> http://www.adafruit.com/products/1431
  ------> http://www.adafruit.com/products/1673

  These displays use SPI to communicate, 4 or 5 pins are required to
  interface
  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.
  BSD license, all text above must be included in any redistribution
 ****************************************************/


#include "Adafruit_GFX.h"
#include "SSD1351_kbv.h"
#ifdef __AVR
#include <avr/pgmspace.h>
#elif defined(ESP8266)
#include <pgmspace.h>
#endif
#include "pins_arduino.h"
#include "wiring_private.h"
#include <SPI.h>

#ifndef _BV
#define _BV(bit) (1<<(bit))
#endif

#define SID_HI    *sidport |= sidpinmask
#define SID_LOW   *sidport &= ~sidpinmask
#define SCLK_HI   *sclkport |= sclkpinmask
#define SCLK_LOW  *sclkport &= ~sclkpinmask
#define CD_DATA   *rsport |= rspinmask
#define CD_COMMAND *rsport &= ~rspinmask
#define CS_IDLE   *csport |= cspinmask
#define CS_ACTIVE *csport &= ~cspinmask

SPISettings oledSetting(8000000, MSBFIRST, SPI_MODE0);  //ESP8266 does not like MODE3

/********************************** low level pin interface */

inline void SSD1351_kbv::spiwrite(uint8_t c)
{
    if (!_sid) {
        SPI.transfer(c);
        // might be able to make this even faster but
        // a delay -is- required
//        delayMicroseconds(1);
        return;
    }

    SCLK_HI;
    for (int8_t i = 7; i >= 0; i--) {
        SCLK_LOW;
        if (c & _BV(i)) {
            SID_HI;
        } else {
            SID_LOW;
        }
        SCLK_HI;
    }
}

void SSD1351_kbv::writeCommand(uint8_t c)
{
    CD_COMMAND;
    CS_ACTIVE;
    spiwrite(c);
    CS_IDLE;
}

void SSD1351_kbv::writeData(uint8_t c) 
{
    CD_DATA;
    CS_ACTIVE;
    spiwrite(c);
    CS_IDLE;
}

/***********************************/
void SSD1351_kbv::setRotation(uint8_t r)
{
    uint8_t val;
    rotation = r & 3;           // just perform the operation ourselves on the protected variables
    _width = (rotation & 1) ? HEIGHT : WIDTH;
    _height = (rotation & 1) ? WIDTH : HEIGHT;
    switch (rotation) {
        case 0:                    //PORTRAIT:
            val = 0x74;            //COLR=1, SM=1, SS=1, ?ML=0, BGR=1, GS=0, MV=0
            break;
        case 1:                    //LANDSCAPE: 90 degrees
            val = 0x77;            //COLR=1, SM=1, SS=1, ?ML=0, BGR=1, GS=1, MV=1
            break;
        case 2:                    //PORTRAIT_REV: 180 degrees
            val = 0x66;            //COLR=1, SM=1, SS=0, ?ML=0, BGR=1, GS=1, MV=0
            break;
        case 3:                    //LANDSCAPE_REV: 270 degrees
            val = 0x65;            //COLR=1, SM=1, SS=0, ?ML=0, BGR=1, GS=0, MV=1
            break;
    }
    writeCommand(0xA0);
    writeData(val);
    _MC = rotation & 1 ? SSD1351_CMD_SETROW : SSD1351_CMD_SETCOLUMN;
    _MP = rotation & 1 ? SSD1351_CMD_SETCOLUMN : SSD1351_CMD_SETROW;
}

/**************************************************************************/
/*!
    @brief  Draws a filled rectangle using HW acceleration
*/
/**************************************************************************/

void SSD1351_kbv::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t fillcolor) 
{
    // Bounds check
    if ((x >= _width) || (y >= _height))
        return;

    // Y bounds check
    if (y + h > _height)
    {
        h = _height - y - 1;
    }

    // X bounds check
    if (x + w > _width)
    {
        w = _width - x - 1;
    }

    // set location
    setAddrWindow(x, y, x + w - 1, y + h - 1);

    // fill!
    writeCommand(SSD1351_CMD_WRITERAM);

    uint16_t i = w * h;
    uint8_t hi = fillcolor >> 8;
    uint8_t lo = fillcolor;
    CD_DATA;
	CS_ACTIVE;
	while (i--) {
        spiwrite(hi);
        spiwrite(lo);
    }
	CS_IDLE;
}

void SSD1351_kbv::drawPixel(int16_t x, int16_t y, uint16_t color)
{
    // Bounds check.
    if ((x >= _width) || (y >= _height)) return;
    if ((x < 0) || (y < 0)) return;
#if 0
    setAddrWindow(x, y, x, y);
    CD_COMMAND;
    CS_ACTIVE;
#else
    CD_COMMAND;
    CS_ACTIVE;
    spiwrite(_MC);
    CD_DATA;
    spiwrite(x);
    spiwrite(x);
    CD_COMMAND;
    spiwrite(_MP);
    CD_DATA;
    spiwrite(y);
    spiwrite(y);
    CD_COMMAND;
#endif
    spiwrite(SSD1351_CMD_WRITERAM);
    CD_DATA;
    spiwrite(color >> 8);
    spiwrite(color);

    CS_IDLE;;
}

void SSD1351_kbv::begin(uint16_t ID) 
{
    // set pin directions
    pinMode(_rs, OUTPUT);

    if (_sclk) {
        pinMode(_sclk, OUTPUT);

        pinMode(_sid, OUTPUT);
    } else {
        // using the hardware SPI
        SPI.begin();
        SPI.beginTransaction(oledSetting);
    }

    // Toggle RST low to reset; CS low so it'll listen to us
    pinMode(_cs, OUTPUT);
    digitalWrite(_cs, LOW);

    if (_rst || 1) {   //ESP8266 D8 pin is GPIO0
        pinMode(_rst, OUTPUT);
        digitalWrite(_rst, HIGH);
        delay(500);
        digitalWrite(_rst, LOW);
        delay(500);
        digitalWrite(_rst, HIGH);
        delay(500);
    }

    // Initialization Sequence
    writeCommand(SSD1351_CMD_COMMANDLOCK);  // set command lock
    writeData(0x12);
    writeCommand(SSD1351_CMD_COMMANDLOCK);  // set command lock
    writeData(0xB1);

    writeCommand(SSD1351_CMD_DISPLAYOFF);       // 0xAE

    writeCommand(SSD1351_CMD_CLOCKDIV);         // 0xB3
    writeCommand(0xF1);                         // 7:4 = Oscillator Frequency, 3:0 = CLK Div Ratio (A[3:0]+1 = 1..16)

    writeCommand(SSD1351_CMD_MUXRATIO);
    writeData(127);

    writeCommand(SSD1351_CMD_SETREMAP);         // 0xA0
    writeData(0x74);                            // COL=1, SM=1, SS=1, RGB=1, GS=0, MV=0

    writeCommand(SSD1351_CMD_SETCOLUMN);
    writeData(0x00);
    writeData(0x7F);
    writeCommand(SSD1351_CMD_SETROW);
    writeData(0x00);
    writeData(0x7F);

    writeCommand(SSD1351_CMD_STARTLINE);        // 0xA1
    if (SSD1351HEIGHT == 96) {
        writeData(96);
    } else {
        writeData(0);
    }


    writeCommand(SSD1351_CMD_DISPLAYOFFSET);    // 0xA2
    writeData(0x0);

    writeCommand(SSD1351_CMD_SETGPIO);
    writeData(0x00);

    writeCommand(SSD1351_CMD_FUNCTIONSELECT);
    writeData(0x01); // internal (diode drop)
    //writeData(0x01); // external bias

    //    writeCommand(SSSD1351_CMD_SETPHASELENGTH);
    //    writeData(0x32);

    writeCommand(SSD1351_CMD_PRECHARGE);        // 0xB1
    writeCommand(0x32);

    writeCommand(SSD1351_CMD_VCOMH);            // 0xBE
    writeCommand(0x05);

    writeCommand(SSD1351_CMD_NORMALDISPLAY);    // 0xA6

    writeCommand(SSD1351_CMD_CONTRASTABC);
    writeData(0xC8);
    writeData(0x80);
    writeData(0xC8);

    writeCommand(SSD1351_CMD_CONTRASTMASTER);
    writeData(0x0F);

    writeCommand(SSD1351_CMD_SETVSL );
    writeData(0xA0);
    writeData(0xB5);
    writeData(0x55);

    writeCommand(SSD1351_CMD_PRECHARGE2);
    writeData(0x01);

    writeCommand(SSD1351_CMD_DISPLAYON);        //--turn on oled panel

    setRotation(0);
}

void  SSD1351_kbv::invertDisplay(boolean v)
{
    if (v) {
        writeCommand(SSD1351_CMD_INVERTDISPLAY);
    } else {
        writeCommand(SSD1351_CMD_NORMALDISPLAY);
    }
}

void     SSD1351_kbv::setAddrWindow(int16_t x, int16_t y, int16_t x1, int16_t y1)
{
#if 0
    CD_COMMAND;
    CS_ACTIVE;
    spiwrite(_MC);
    CD_DATA;
    spiwrite(x);
    spiwrite(x1);
    CD_COMMAND;
    spiwrite(_MP);
    CD_DATA;
    spiwrite(y);
    spiwrite(y1);
    CS_IDLE;
#else
    writeCommand(_MC);
    writeData(x);
    writeData(x1);
    writeCommand(_MP);
    writeData(y);
    writeData(y1);
#endif
}

void SSD1351_kbv::pushColors_any(uint16_t cmd, uint8_t * block, int16_t n, bool first, uint8_t flags)
{
    uint8_t h, l;
    bool isconst = flags & 1;
    bool isbigend = (flags & 2) != 0;
    if (first) {
        writeCommand(cmd);
    }
    while (n-- > 0) {
        if (isconst) {
            h = pgm_read_byte(block++);
            l = pgm_read_byte(block++);
        } else {
            h = (*block++);
            l = (*block++);
        }
        if (isbigend) writeData(h);
        writeData(l);
        if (!isbigend) writeData(h);
    }
}

void SSD1351_kbv::pushColors(uint16_t * block, int16_t n, bool first)
{
    pushColors_any(SSD1351_CMD_WRITERAM, (uint8_t *)block, n, first, 0);
}
void SSD1351_kbv::pushColors(uint8_t * block, int16_t n, bool first)
{
    pushColors_any(SSD1351_CMD_WRITERAM, (uint8_t *)block, n, first, 2);   //regular bigend
}
void SSD1351_kbv::pushColors(const uint8_t * block, int16_t n, bool first, bool bigend)
{
    pushColors_any(SSD1351_CMD_WRITERAM, (uint8_t *)block, n, first, bigend ? 3 : 1);
}

void SSD1351_kbv::vertScroll(int16_t top, int16_t scrollines, int16_t offset)
{
    bool ML = 0; //getRotation() & 2;
    writeCommand(0xA1);
    writeData(ML ? 127 - offset : offset);
}
int16_t  SSD1351_kbv::readGRAM(int16_t x, int16_t y, uint16_t *block, int16_t w, int16_t h)
{
}

/********************************* low level pin initialization */

SSD1351_kbv::SSD1351_kbv(uint8_t cs, uint8_t rs, uint8_t sid, uint8_t sclk, uint8_t rst) : Adafruit_GFX(SSD1351WIDTH, SSD1351HEIGHT)
{
    _cs = cs;
    _rs = rs;
    _sid = sid;
    _sclk = sclk;
    _rst = rst;

    csport      = portOutputRegister(digitalPinToPort(cs));
    cspinmask   = digitalPinToBitMask(cs);

    rsport      = portOutputRegister(digitalPinToPort(rs));
    rspinmask   = digitalPinToBitMask(rs);

    sidport      = portOutputRegister(digitalPinToPort(sid));
    sidpinmask   = digitalPinToBitMask(sid);

    sclkport      = portOutputRegister(digitalPinToPort(sclk));
    sclkpinmask   = digitalPinToBitMask(sclk);

}

SSD1351_kbv::SSD1351_kbv(uint8_t cs, uint8_t rs,  uint8_t rst) : Adafruit_GFX(SSD1351WIDTH, SSD1351HEIGHT)
{
    _cs = cs;
    _rs = rs;
    _sid = 0;
    _sclk = 0;
    _rst = rst;

    csport      = portOutputRegister(digitalPinToPort(cs));
    cspinmask   = digitalPinToBitMask(cs);

    rsport      = portOutputRegister(digitalPinToPort(rs));
    rspinmask   = digitalPinToBitMask(rs);

}

















