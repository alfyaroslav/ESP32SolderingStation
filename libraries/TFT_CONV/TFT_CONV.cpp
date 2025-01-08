#include "TFT_CONV.h"
#include <TFT_eSPI.h>
#include <Arduino.h>

//TFT_eSPI tft;

TFT_eSPI tft = TFT_eSPI();


void _convert_float(char *buf, double num, int width, byte prec)
{
	char format[10];
	
	sprintf(format, "%%%i.%if", width, prec);
	sprintf(buf, format, num);
}
  
TFT_CONV::TFT_CONV()
{
}

void TFT_CONV::InitLCD()
{

	tft.init();
	tft.setRotation(1);
}

void TFT_CONV::clrScr()
{
	tft.fillScreen(TFT_BLACK); 
}

void TFT_CONV::setFont(int font)
{
	tft.setTextFont(font);
	//tft.setTextFont(2);
}

void TFT_CONV::setFontAlt(int font) 
{
	tft.setTextFont(font);
	//tft.setTextFont(2);
}

void TFT_CONV::setColor(uint16_t color)
{
  
    tft.setTextColor(color, TFT_BLACK);
}

void TFT_CONV::drawLine(int x1, int y1, int x2, int y2)
{
	tft.drawLine(x1, y1, x2, y2, tft.textcolor);
}

void TFT_CONV::printNumI(long num, int x, int y, int length, char filler)
{
	char buf[25];
	char st[27];
	boolean neg=false;
	int c=0, f=0;
	
	if (num==0)
	{
		if (length!=0)
		{
			for (c=0; c<(length-1); c++)
				st[c]=filler;
			st[c]=48;
			st[c+1]=0;
		}
		else
		{
			st[0]=48;
			st[1]=0;
		}
	}
	else
	{
		if (num<0)
		{
			neg=true;
			num=-num;
		}
	  
		while (num>0)
		{
			buf[c]=48+(num % 10);
			c++;
			num=(num-(num % 10))/10;
		}
		buf[c]=0;
	  
		if (neg)
		{
			st[0]=45;
		}
	  
		if (length>(c+neg))
		{
			for (int i=0; i<(length-c-neg); i++)
			{
				st[i+neg]=filler;
				f++;
			}
		}

		for (int i=0; i<c; i++)
		{
			st[i+neg+f]=buf[c-i-1];
		}
		st[c+neg+f]=0;

	}

   tft.setTextPadding(0);
   tft.setTextColor(tft.textcolor, TFT_BLACK);
   tft.drawString(st,x,y, tft.textfont);
  
}

void TFT_CONV::printNumF(double num, byte dec, int x, int y, char divider, int length, char filler)
{
	
	
	char st[27];
	boolean neg=false;

	if (dec<1)
		dec=1;
	else if (dec>5)
		dec=5;

	if (num<0)
		neg = true;

	_convert_float(st, num, length, dec);

	if (divider != '.')
	{
		for (int i=0; i<sizeof(st); i++)
			if (st[i]=='.')
				st[i]=divider;
	}

	if (filler != ' ')
	{
		if (neg)
		{
			st[0]='-';
			for (int i=1; i<sizeof(st); i++)
				if ((st[i]==' ') || (st[i]=='-'))
					st[i]=filler;
		}
		else
		{
			for (int i=0; i<sizeof(st); i++)
				if (st[i]==' ')
					st[i]=filler;
		}
	}
   
   
   
   //tft.setTextColor(tft.textcolor, TFT_BLACK);
   //int padding = tft.textWidth(st, tft.textfont);
   tft.setTextPadding(0);
   tft.drawString(st,x,y, tft.textfont);
}

void TFT_CONV::print(char *st, int x, int y)
{
	//int padding = tft.textWidth(st, tft.textfont);
	//tft.setTextPadding(padding);
	
	if (x == CENTER)
	{
	  tft.drawCentreString(st, 240, y, tft.textfont);
	}
	else if (x == RIGHT)
	{
	  tft.drawCentreString(st, 425, y, tft.textfont);
	}
	else
	{
	  tft.drawString(st, x, y, tft.textfont);
	}
}

void TFT_CONV::print(String st, int x, int y)
{
	char buf[st.length()+1];

	st.toCharArray(buf, st.length()+1);
	//print(buf, x, y);
	tft.drawString(buf, x, y, tft.textfont);
}

int TFT_CONV::getDisplayXSize()
{
		return tft.width();
}

int TFT_CONV::getDisplayYSize()
{
	 return tft.height();
}

//void TFT_CONV::drawRoundRect(int32_t x1, int32_t y1, int32_t x2, int32_t y2)
//{
 //  tft.drawRoundRect(x1, y1, x2, y2-26, 1, tft.textcolor);
//}

void TFT_CONV::drawRoundRect(int x1, int y1, int x2, int y2)
{
	if (x1>x2)
	{
		swap2(int, x1, x2);
	}
	if (y1>y2)
	{
		swap2(int, y1, y2);
	}
	if ((x2-x1)>4 && (y2-y1)>4)
	{
		tft.drawPixel(x1+1,y1+1, tft.textcolor);
		tft.drawPixel(x2-1,y1+1, tft.textcolor);
		tft.drawPixel(x1+1,y2-1, tft.textcolor);
		tft.drawPixel(x2-1,y2-1, tft.textcolor);
		tft.drawFastHLine(x1+2, y1,   x2-x1-4, tft.textcolor);
		tft.drawFastHLine(x1+2, y2,   x2-x1-4, tft.textcolor);
		tft.drawFastVLine(x1,   y1+2, y2-y1-4, tft.textcolor);
		tft.drawFastVLine(x2,   y1+2, y2-y1-4, tft.textcolor);
	}
}

void TFT_CONV::fillRoundRect(int x1, int y1, int x2, int y2)
{
	if (x1>x2)
	{
		swap2(int, x1, x2);
	}
	if (y1>y2)
	{
		swap2(int, y1, y2);
	}

	if ((x2-x1)>4 && (y2-y1)>4)
	{
		for (int i=0; i<((y2-y1)/2)+1; i++)
		{
			switch(i)
			{
			case 0:
				tft.drawFastHLine(x1+2, y1+i, x2-x1-4, tft.textcolor);
				tft.drawFastHLine(x1+2, y2-i, x2-x1-4, tft.textcolor);
				break;
			case 1:
				tft.drawFastHLine(x1+1, y1+i, x2-x1-2, tft.textcolor);
				tft.drawFastHLine(x1+1, y2-i, x2-x1-2, tft.textcolor);
				break;
			default:
				tft.drawFastHLine(x1, y1+i, x2-x1, tft.textcolor);
				tft.drawFastHLine(x1, y2-i, x2-x1, tft.textcolor);
			}
		}
	}
}
//void TFT_CONV::fillRoundRect(int32_t x1, int32_t y1, int32_t x2, int32_t y2)
//{
//	tft.fillRoundRect(x1, y1, x2, y2-26, 0, tft.textcolor);
//}

void TFT_CONV::drawBitmap(int32_t x, int32_t y, int32_t sx, int32_t sy, uint16_t *bitmap, int32_t scale)
{
	tft.setSwapBytes(true);
	tft.pushImage(x, y, sx, sy, bitmap, scale);
	
	//pushImage(x, y, sx, sy, data, scale);
	//drawBitmap(x, y, sx, sy, bitmap, textcolor);
	//void TFT_eSPI::pushImage(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t *data, uint16_t transp)
}

void TFT_CONV::drawCircle(int32_t x0, int32_t y0, int32_t r)
{
	tft.drawCircle(x0, y0, r, tft.textcolor);
}
void TFT_CONV::drawPixel(int32_t x, int32_t y)
{
	tft.drawPixel(x, y, tft.textcolor);
}

void TFT_CONV::fillCircle(int32_t x0, int32_t y0, int32_t r)
{
	tft.fillCircle(x0, y0, r, tft.textcolor);
}

void TFT_CONV::setBackColor(uint16_t color)
{
	tft.setTextColor(tft.textcolor, color);
}

void TFT_CONV::setColor(byte r, byte g, byte b)
{
	byte fch=((r&248)|g>>5);
	byte fcl=((g&28)<<3|b>>3);
	
	tft.setTextColor(fch << 8 | fcl);
}

#ifdef Set_Touch
uint8_t TFT_CONV::getTouchRaw(uint16_t *x, uint16_t *y)
{

//  uint8_t status, x, y;
//  uint16_t xx, yy;

//  status =  tft.getTouchRaw(&xx, &yy);  
//  sprintf(x, "%d", xx);
 // sprintf(y, "%d", yy);

	return tft.getTouchRaw(x, y);
}

uint8_t TFT_CONV::getTouch(uint16_t *x, uint16_t *y, uint16_t threshold)
{
	return tft.getTouch(x, y, threshold);
}

void TFT_CONV::calibrateTouch(uint16_t *parameters, uint32_t color_fg, uint32_t color_bg, uint8_t size)
{
  tft.calibrateTouch(parameters, color_fg, color_bg, size);
}

void TFT_CONV::setTouch(uint16_t *data)
{
  tft.setTouch(data);
}
#endif
