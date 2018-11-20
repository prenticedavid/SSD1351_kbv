/* I have stolen large sections from Adafruit_SSD1351
 * and just complied wih XXX_kbv methods
 * principally this means using the hardware for ALL rotations
 */

/*
 * SSD1351_kbv class inherits from Adafruit_GFX class and the Arduino Print class.
 * Any use of SSD1351_kbv class and examples is dependent on Adafruit and Arduino licenses
 * The license texts are in the accompanying license.txt file
 */

#ifndef SSD1351_KBV_H_
#define SSD1351_KBV_H_   101

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

#define SSD1351WIDTH 128
#define SSD1351HEIGHT 128  // SET THIS TO 96 FOR 1.27"!

#define swap(a, b) { uint16_t t = a; a = b; b = t; }

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#include "Adafruit_GFX.h"

#ifdef __SAM3X8E__
typedef volatile RwReg PortReg;
typedef uint32_t PortMask;
#elif defined(__arm__)
typedef volatile uint32_t PortReg;
typedef uint32_t PortMask;
#elif defined(ESP32)
typedef volatile uint32_t PortReg;
typedef uint32_t PortMask;
#elif defined(ESP8266)
typedef volatile uint32_t PortReg;
typedef uint32_t PortMask;
#else
typedef volatile uint8_t PortReg;
typedef uint8_t PortMask;
#endif

// Select one of these defines to set the pixel color order
#define SSD1351_COLORORDER_RGB
// #define SSD1351_COLORORDER_BGR

#if defined SSD1351_COLORORDER_RGB && defined SSD1351_COLORORDER_BGR
#error "RGB and BGR can not both be defined for SSD1351_COLORODER."
#endif

// Timing Delays
#define SSD1351_DELAYS_HWFILL        (3)
#define SSD1351_DELAYS_HWLINE       (1)

// SSD1351 Commands
#define SSD1351_CMD_SETCOLUMN       0x15
#define SSD1351_CMD_SETROW          0x75
#define SSD1351_CMD_WRITERAM        0x5C
#define SSD1351_CMD_READRAM         0x5D
#define SSD1351_CMD_SETREMAP        0xA0
#define SSD1351_CMD_STARTLINE       0xA1
#define SSD1351_CMD_DISPLAYOFFSET   0xA2
#define SSD1351_CMD_DISPLAYALLOFF   0xA4
#define SSD1351_CMD_DISPLAYALLON    0xA5
#define SSD1351_CMD_NORMALDISPLAY   0xA6
#define SSD1351_CMD_INVERTDISPLAY   0xA7
#define SSD1351_CMD_FUNCTIONSELECT  0xAB
#define SSD1351_CMD_DISPLAYOFF      0xAE
#define SSD1351_CMD_DISPLAYON       0xAF
#define SSD1351_CMD_PRECHARGE       0xB1
#define SSD1351_CMD_DISPLAYENHANCE  0xB2
#define SSD1351_CMD_CLOCKDIV        0xB3
#define SSD1351_CMD_SETVSL      0xB4
#define SSD1351_CMD_SETGPIO         0xB5
#define SSD1351_CMD_PRECHARGE2      0xB6
#define SSD1351_CMD_SETGRAY         0xB8
#define SSD1351_CMD_USELUT      0xB9
#define SSD1351_CMD_PRECHARGELEVEL  0xBB
#define SSD1351_CMD_VCOMH       0xBE
#define SSD1351_CMD_CONTRASTABC     0xC1
#define SSD1351_CMD_CONTRASTMASTER  0xC7
#define SSD1351_CMD_MUXRATIO            0xCA
#define SSD1351_CMD_COMMANDLOCK         0xFD
#define SSD1351_CMD_HORIZSCROLL         0x96
#define SSD1351_CMD_STOPSCROLL          0x9E
#define SSD1351_CMD_STARTSCROLL         0x9F

#define color565 Color565

class SSD1351_kbv  : public /*virtual*/ Adafruit_GFX {
    public:
        SSD1351_kbv(uint8_t CS, uint8_t RS, uint8_t SID, uint8_t SCLK, uint8_t RST);
        SSD1351_kbv(uint8_t CS, uint8_t RS, uint8_t RST);

        void     reset(void);                                       // you only need the constructor
        void     begin(uint16_t ID = 0x1351);                       // you only need the constructor
        uint16_t readID(void) {
            return 0x1351;
        }
        uint16_t color565(uint8_t r, uint8_t g, uint8_t b) {
            return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | ((b & 0xF8) >> 3);
        }

        virtual void     drawPixel(int16_t x, int16_t y, uint16_t color);  // and these three
        virtual void     fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
        virtual void     drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color) {
            fillRect(x, y, 1, h, color);
        }
        virtual void     drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color) {
            fillRect(x, y, w, 1, color);
        }
        virtual void     fillScreen(uint16_t color)                                     {
            fillRect(0, 0, _width, _height, color);
        }
        virtual void     setRotation(uint8_t r);
        virtual void     invertDisplay(boolean i);

        int16_t  readGRAM(int16_t x, int16_t y, uint16_t *block, int16_t w, int16_t h);
        uint16_t readPixel(int16_t x, int16_t y) {
            uint16_t color;
            readGRAM(x, y, &color, 1, 1);
            return color;
        }
        void     setAddrWindow(int16_t x, int16_t y, int16_t x1, int16_t y1);
        void     pushColors(uint16_t *block, int16_t n, bool first);
        void     pushColors(uint8_t *block, int16_t n, bool first);
        void     pushColors(const uint8_t *block, int16_t n, bool first, bool bigend = false);
        void     vertScroll(int16_t top, int16_t scrollines, int16_t offset);

        /* low level */

        void writeData(uint8_t d);
        void writeCommand(uint8_t c);

    private:
        void spiwrite(uint8_t);
        void pushColors_any(uint16_t cmd, uint8_t * block, int16_t n, bool first, uint8_t flags);
        uint8_t _cs, _rs, _rst, _sid, _sclk, _MC, _MP;
        PortReg *csport, *rsport, *sidport, *sclkport;
        PortMask cspinmask, rspinmask, sidpinmask, sclkpinmask;
};

// New color definitions.  thanks to Bodmer
#define TFT_BLACK       0x0000      /*   0,   0,   0 */
#define TFT_NAVY        0x000F      /*   0,   0, 128 */
#define TFT_DARKGREEN   0x03E0      /*   0, 128,   0 */
#define TFT_DARKCYAN    0x03EF      /*   0, 128, 128 */
#define TFT_MAROON      0x7800      /* 128,   0,   0 */
#define TFT_PURPLE      0x780F      /* 128,   0, 128 */
#define TFT_OLIVE       0x7BE0      /* 128, 128,   0 */
#define TFT_LIGHTGREY   0xC618      /* 192, 192, 192 */
#define TFT_DARKGREY    0x7BEF      /* 128, 128, 128 */
#define TFT_BLUE        0x001F      /*   0,   0, 255 */
#define TFT_GREEN       0x07E0      /*   0, 255,   0 */
#define TFT_CYAN        0x07FF      /*   0, 255, 255 */
#define TFT_RED         0xF800      /* 255,   0,   0 */
#define TFT_MAGENTA     0xF81F      /* 255,   0, 255 */
#define TFT_YELLOW      0xFFE0      /* 255, 255,   0 */
#define TFT_WHITE       0xFFFF      /* 255, 255, 255 */
#define TFT_ORANGE      0xFDA0      /* 255, 180,   0 */
#define TFT_GREENYELLOW 0xB7E0      /* 180, 255,   0 */
#define TFT_PINK        0xFC9F

#endif
