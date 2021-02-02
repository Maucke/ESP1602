/*
	OLedSPI.h
	OLED 1602 Library for the SPI
	OLED display from WIDE.HK
	Also my first attempt at a Library
	Phil Grant Sept 2013
	www.gadjet.co.uk
	Scrolling contributed by Nathan Chantrell http://nathan.chantrell.net/
	Updated 06/11/2013 to include the cursPos data within the sendString function.
*/
#ifndef OLedSPI_h
#define OLedSPI_h	
#include <stdio.h>
#include <stdlib.h> 
#include <string.h> 
#include <SPI.h>

class OLedSPI {
public:
		OLedSPI();
		~OLedSPI();

		unsigned char checkdiff(unsigned char* array1,unsigned char* array2,int len);
		void animotion(const char* ch,int motion,int font,int mode);

		void init();
		void refrash_Screen(int col, int row);
		void write_CGRAM(uint8_t  a[]);
		void sendCommand(unsigned char command);
		void sendString(const char *String, uint8_t col, uint8_t row);
		void display(const char* ch, int col, int row);
		void clear(char ch);
		void sendString(const char *String, uint8_t col, uint8_t row,char ch);
		void sendData(unsigned char data);
		void clearLcd();
		void cursPos(uint8_t, uint8_t); // cloumn, row
		void scrollString(char* message, uint8_t row, unsigned int time); //written by Nathan Chantrell http://nathan.chantrell.net/
		void lcdOff();
		void lcdOn();
		void setContrast(unsigned char contrast); // contrast should be the hex value between 0x00 and 0xFF
};
#endif
