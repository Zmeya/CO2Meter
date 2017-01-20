#pragma once
#include <stm32f1xx.h>
#include "ITask.h"
#include "fonts.h"

// LCD settings
#define ILI9341_WIDTH				240
#define ILI9341_HEIGHT				320
#define ILI9341_PIXEL				76800

enum LDC_Commands
{
	Reset			= 0x01,	// Software Reset
	Sleep			= 0x10,	// Enter Sleep Mode
	SleepOut		= 0x11,	// Sleep Out
	Gamma			= 0x26, // Gamma Set
	DisplayOff		= 0x28, // Display OFF
	DisplayOn		= 0x29, // Display ON
	ColumnAddress	= 0x2A, // Column Address Set
	PageAddress		= 0x2B,	// Page Address Set
	GRAM			= 0x2C,	// Memory Write
	MAC				= 0x36, // Memory Access Control
	PixelFormat		= 0x3A, // Pixel Format Set
	WDB				= 0x51, // Write Display Brightness
	WCD				= 0x53, // Write CTRL Display
	RGBInterface	= 0xB0, // RGB Interface Signal Control
	FRC				= 0xB1, // Frame Control (In Normal Mode)
	BPC				= 0xB5, // Blanking Porch Control 
	DFC				= 0xB6, // Display Function Control
	Power1			= 0xC0, // Power Control 1
	Power2			= 0xC1,	// Power Control 2
	VCOM1			= 0xC5,	// VCOM Control 1
	VCOM2			= 0xC7, // VCOM Control 2
	PowerA			= 0xCB,
	PowerB			= 0xCF,
	PGamma			= 0xE0, // Positive Gamma Correction
	NGamma			= 0xE1,  // Negative Gamma Correction
	DTCA			= 0xE8,
	DTCB			= 0xEA,
	PowerSeqence	= 0xED,
	GammaEn			= 0xF2,
	Interface		= 0xF6,
	PRC				= 0xF7
};

enum Colors
{
	White			= 0xFFFF,
	Black			= 0x0000,
	Red				= 0xF800,
	Green			= 0x07E0,
	Blue			= 0x001F,
	Yellow			= 0xFFE0,
	Orange			= 0xFBE4,
	Cyan			= 0x07FF,
	Magenta			= 0xA254,
	Gray			= 0x7BEF,
	Brown			= 0xBBCA,
	Transparent		= 0x80000000,
};

enum Orientation
{
	Portrait = 0,
	Portrait_Mirror,
	Landscape,
	Landscape_Mirror,
};

class CDisplayDriver
{
private:
	static CDisplayDriver *m_Instance;
	uint32_t m_nCurrentX;
	uint32_t m_nCurrentY;
	uint8_t m_bLandscape;
	uint8_t m_bInitialize;

	void InitSPI();
	void InitLCD();

	void SetCSPin(uint8_t bEnable);
	void SetDCPin(uint8_t bEnable);

	uint8_t SendCommand(uint8_t bCommand);
	uint8_t SendData(uint8_t bData);
	void HardReset(void);
public:
	CDisplayDriver();
	~CDisplayDriver();

	void Initialize();

	void BacklightEnable(uint8_t bEnable);
	void DrawPixel(uint16_t x, uint16_t y, uint32_t color);
	void SetCursorPosition(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
	void Fill(uint16_t color);
	void FillRectangle(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color);
	void DrawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint32_t color);
	void DrawRectangle(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint32_t color);
	void DrawFilledRectangle(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint32_t color);
	void DrawCircle(int16_t x0, int16_t y0, int16_t r, uint32_t color);
	void DrawFilledCircle(int16_t x0, int16_t y0, int16_t r, uint32_t color);
	void Rotate(int orientation);
	void DrawString(uint16_t x, uint16_t y, char *str, Font_t *font, uint32_t foreground, uint32_t background);
	void GetStringSize(char *str, Font_t *font, uint16_t *width, uint16_t *height);
	void DrawChar(uint16_t x, uint16_t y, char c, Font_t *font, uint32_t foreground, uint32_t background);

	static CDisplayDriver* GetInstance(void);
};

