#ifndef TFT_CONV_h
#define TFT_CONV_h

//Standard support
#include <Arduino.h>
#include <Print.h>
#include <SPI.h>

#define VGA_BLACK		0x0000
#define VGA_WHITE		0xFFFF
#define VGA_RED			0xF800
#define VGA_GREEN		0x0400
#define VGA_BLUE		0x001F
#define VGA_SILVER		0xC618
#define VGA_GRAY		0x8410
#define VGA_MAROON		0x8000
#define VGA_YELLOW		0xFFE0
#define VGA_OLIVE		0x8400
//#define VGA_LIME		0xB7E0
#define VGA_LIME		0x07E0
#define VGA_AQUA		0x07FF
#define VGA_TEAL		0x0410
#define VGA_NAVY		0x0010
#define VGA_FUCHSIA		0xF81F
#define VGA_PURPLE		0x8010

#define LEFT 0
#define RIGHT 9999
#define CENTER 9998

#define SmallFont 1
#define BigFont 2
#define BFontRu 2
#define SevenSegNumFont 7

#define swap2(type, i, j) {type t = i; i = j; j = t;}

class TFT_CONV
{
	public:
    TFT_CONV();
	
    void InitLCD();
    void clrScr();
    void setFont(int font);
    void setFontAlt(int font);
    void setColor(uint16_t color);
	void setBackColor(uint16_t color);
	void setColor(byte r, byte g, byte b);
    void drawLine(int x1, int y1, int x2, int y2);
    void printNumI(long num, int x, int y, int length=0, char filler=' ');
    void printNumF(double num, byte dec, int x, int y, char divider='.', int length=0, char filler=' ');
    void print(char *st, int x, int y);
    void print(String st, int x, int y);
    void drawRoundRect(int x1, int y1, int x2, int y2);
    void fillRoundRect(int x1, int y1, int x2, int y2);
    void drawBitmap(int32_t x, int32_t y, int32_t sx, int32_t sy, uint16_t *bitmap, int32_t scale=1);
	void drawCircle(int32_t x, int32_t y, int32_t r);
	void drawPixel(int32_t x, int32_t y);
	void fillCircle(int32_t x0, int32_t y0, int32_t r);
    int getDisplayXSize();
    int getDisplayYSize();
	#ifdef Set_Touch
	uint8_t getTouchRaw(uint16_t *x, uint16_t *y);
	uint8_t getTouch(uint16_t *x, uint16_t *y, uint16_t threshold = 600);
	void calibrateTouch(uint16_t *parameters, uint32_t color_fg, uint32_t color_bg, uint8_t size);
	void setTouch(uint16_t *data);
	#endif
}; // End of class TFT_CONV

#endif // ends #ifndef TFT_CONV_h
