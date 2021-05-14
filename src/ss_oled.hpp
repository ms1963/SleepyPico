#pragma once
#include "BitBang_I2C.h"
#include "ss_oled.h"
#include "pico/stdlib.h"
#include "hardware/i2c.h"

class picoSSOLED {
	
private:

	SSOLED oled ;
	bool invert ;
	int32_t speed ;


public:
//constructor

	picoSSOLED(int iType, int iAddr, bool bFlip, bool bInvert, i2c_inst_t * pI2C, int sda, int scl, int32_t iSpeed) {
		oled.oled_type = iType ;
		oled.oled_addr = iAddr ;
		oled.oled_flip = (int) bFlip ;
		invert = bInvert ;
    	oled.bbi2c.picoI2C = pI2C;
    	oled.bbi2c.iSDA = sda ;
    	oled.bbi2c.iSCL = scl ;
		speed = iSpeed ;
	};

	
	int init () {
		return __oledInit(&oled, (int) invert, (int32_t) speed);
	};


//
// Provide or revoke a back buffer for your OLED graphics
// This allows you to manage the RAM used by ss_oled on tiny
// embedded platforms like the ATmega series
// Pass NULL to revoke the buffer. Make sure you provide a buffer
// large enough for your display (e.g. 128x64 needs 1K - 1024 bytes)
//
	void set_back_buffer(uint8_t * pBuffer) {
		__oledSetBackBuffer(&oled, pBuffer);
	};

//
// Sets the brightness (0=off, 255=brightest)
//
	void set_contrast(uint ucContrast) {
		__oledSetContrast(&oled, (unsigned char) ucContrast);
	};

//
// Load a 128x64 1-bpp Windows bitmap
// Pass the pointer to the beginning of the BMP file
// First pass version assumes a full screen bitmap
//
	int load_bmp(uint8_t *pBMP, bool bInvert, bool bRender) {
		return __oledLoadBMP(&oled, pBMP, (int) bInvert, (int) bRender);
	};

//
// Power up/down the display
// useful for low power situations
//
	void power(bool bON) {
		__oledPower(&oled, (uint8_t) bON);
	};

//
// Set the current cursor position
// The column represents the pixel column (0-127)
// The row represents the text row (0-7)
//
	void set_cursor(int x, int y) {
		__oledSetCursor(&oled, x, y);
	};

//
// Turn text wrap on or off for the oldWriteString() function
//
	void set_textWrap(bool bWrap) {
		__oledSetTextWrap(&oled, (int) bWrap);
	};

//
// Draw a string of normal (8x8), small (6x8) or large (16x32) characters
// At the given col+row with the given scroll offset. The scroll offset allows you to
// horizontally scroll text which does not fit on the width of the display. The offset
// represents the pixels to skip when drawing the text. An offset of 0 starts at the beginning
// of the text.
// The system remembers where the last text was written (the cursor position)
// To continue writing from the last position, set the x,y values to -1
// The text can optionally wrap around to the next line by calling oledSetTextWrap(true);
// otherwise text which would go off the right edge will not be drawn and the cursor will
// be left "off screen" until set to a new position explicitly
//
//  Returns 0 for success, -1 for invalid parameter
//
	int write_string(int iScrollX, int x, int y, char *szMsg, int iSize, bool bInvert, bool bRender) {
		return __oledWriteString(&oled, iScrollX, x, y, szMsg, iSize, (int) bInvert, (int) bRender);
	};

//
// Fill the frame buffer with a byte pattern
// e.g. all off (0x00) or all on (0xff)
//
	void fill(unsigned char ucData, bool bRender) {
		__oledFill(&oled, ucData, (int) bRender);
	}

//
// Set (or clear) an individual pixel
// The local copy of the frame buffer is used to avoid
// reading data from the display controller
// (which isn't possible in most configurations)
// This function needs the USE_BACKBUFFER macro to be defined
// otherwise, new pixels will erase old pixels within the same byte
//
	int set_pixel(int x, int y, unsigned char ucColor, bool bRender) {
		return __oledSetPixel(&oled, x, y, ucColor, (int) bRender);
	};

//
// Dump an entire custom buffer to the display
// useful for custom animation effects
//
	void dump_buffer(uint8_t *pBuffer) {
		__oledDumpBuffer(&oled, pBuffer);
	};

//
// Render a window of pixels from a provided buffer or the library's internal buffer
// to the display. The row values refer to byte rows, not pixel rows due to the memory
// layout of OLEDs. Pass a src pointer of NULL to use the internal backing buffer
// returns 0 for success, -1 for invalid parameter
//
	int draw_GFX(uint8_t *pSrc, int iSrcCol, int iSrcRow, int iDestCol, int iDestRow, int iWidth, int iHeight, int iSrcPitch) {
		return __oledDrawGFX(&oled, pSrc, iSrcCol, iSrcRow, iDestCol, iDestRow, iWidth, iHeight, iSrcPitch);
	};

//
// Draw a line between 2 points
//
	void draw_line(int x1, int y1, int x2, int y2, bool bRender) {
		__oledDrawLine(&oled, x1, y1, x2, y2, (int) bRender);
	};

//
// Play a frame of animation data
// The animation data is assumed to be encoded for a full frame of the display
// Given the pointer to the start of the compressed data,
// it returns the pointer to the start of the next frame
// Frame rate control is up to the calling program to manage
// When it finishes the last frame, it will start again from the beginning
//
	uint8_t * play_anim_frame(uint8_t *pAnimation, uint8_t *pCurrent, int iLen) {
		return __oledPlayAnimFrame(&oled, pAnimation, pCurrent, iLen);
	};

//
// Scroll the internal buffer by 1 scanline (up/down)
// width is in pixels, lines is group of 8 rows
// Returns 0 for success, -1 for invalid parameter
//
	int scroll_buffer(int iStartCol, int iEndCol, int iStartRow, int iEndRow, bool bUp) {
		return __oledScrollBuffer(&oled, iStartCol,iEndCol, iStartRow, iEndRow, (int) bUp);
	};

//
// Draw a sprite of any size in any position
// If it goes beyond the left/right or top/bottom edges
// it's trimmed to show the valid parts
// This function requires a back buffer to be defined
// The priority color (0 or 1) determines which color is painted 
// when a 1 is encountered in the source image.
// e.g. when 0, the input bitmap acts like a mask to clear
// the destination where bits are set.
//
	void draw_sprite(uint8_t *pSprite, int cx, int cy, int iPitch, int x, int y, uint8_t iPriority) {
		__oledDrawSprite(&oled, pSprite, cx,  cy,  iPitch,  x,  y,  iPriority);
	};

//
// Draw a 16x16 tile in any of 4 rotated positions
// Assumes input image is laid out like "normal" graphics with
// the MSB on the left and 2 bytes per line
// On AVR, the source image is assumed to be in FLASH memory
// The function can draw the tile on byte boundaries, so the x value
// can be from 0 to 112 and y can be from 0 to 6
//
	void draw_tile(const uint8_t *pTile, int x, int y, int iRotation, bool bInvert, bool bRender) {
		__oledDrawTile(&oled, (const uint8_t *) pTile,  x,  y,  iRotation, (int) bInvert, (int) bRender);
	};

//
// Draw an outline or filled ellipse
//
	void draw_ellipse(int iCenterX, int iCenterY, int32_t iRadiusX, int32_t iRadiusY, uint8_t ucColor, bool bFilled) {
		__oledEllipse(&oled, iCenterX, iCenterY, iRadiusX, iRadiusY, ucColor, (uint8_t) bFilled);
	};

//
// Draw an outline or filled rectangle
//
	void draw_rectangle(int x1, int y1, int x2, int y2, uint8_t ucColor, bool bFilled) {
		__oledRectangle(&oled,  x1,  y1,  x2,  y2, ucColor, (uint8_t) bFilled);
	};

};


