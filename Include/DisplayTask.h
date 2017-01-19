#pragma once
#include "ITask.h"
#include "fonts.h"

/* ILI9341 library for STM32F4xx with SPI communication

ILI9341		STM32F1xx	DESCRIPTION

WRX or D/C	PB0		Data/Command register
LED			PB1		Backlight
SCK			PB3		SPI clock
SDO (MISO)	PB4		Output from LCD for SPI.
SDI (MOSI)	PB5		SPI master output
RESET		PC14	Reset LCD
CS			PA15	Chip select for SPI
GND			GND		Ground
VCC			3.3V	Positive power supply
*/

#define ILI9341_DC_PORT			GPIOA
#define ILI9341_DC_PIN			GPIO_PIN_2

#define ILI9341_LED_PORT		GPIOA
#define ILI9341_LED_PIN			GPIO_PIN_0

#define ILI9341_RESET_PORT		GPIOA
#define ILI9341_RESET_PIN		GPIO_PIN_1

#define ILI9341_CS_PORT			GPIOA
#define ILI9341_CS_PIN			GPIO_PIN_4

/* Private defines */
#define ILI9341_RESET				0x01
#define ILI9341_SLEEP_OUT			0x11
#define ILI9341_GAMMA				0x26
#define ILI9341_DISPLAY_OFF			0x28
#define ILI9341_DISPLAY_ON			0x29
#define ILI9341_COLUMN_ADDR			0x2A
#define ILI9341_PAGE_ADDR			0x2B
#define ILI9341_GRAM				0x2C
#define ILI9341_MAC					0x36
#define ILI9341_PIXEL_FORMAT		0x3A
#define ILI9341_WDB					0x51
#define ILI9341_WCD					0x53
#define ILI9341_RGB_INTERFACE		0xB0
#define ILI9341_FRC					0xB1
#define ILI9341_BPC					0xB5
#define ILI9341_DFC					0xB6
#define ILI9341_POWER1				0xC0
#define ILI9341_POWER2				0xC1
#define ILI9341_VCOM1				0xC5
#define ILI9341_VCOM2				0xC7
#define ILI9341_POWERA				0xCB
#define ILI9341_POWERB				0xCF
#define ILI9341_PGAMMA				0xE0
#define ILI9341_NGAMMA				0xE1
#define ILI9341_DTCA				0xE8
#define ILI9341_DTCB				0xEA
#define ILI9341_POWER_SEQ			0xED
#define ILI9341_3GAMMA_EN			0xF2
#define ILI9341_INTERFACE			0xF6
#define ILI9341_PRC					0xF7

// LCD settings
#define ILI9341_WIDTH				240
#define ILI9341_HEIGHT				320
#define ILI9341_PIXEL				76800

// Colors
#define ILI9341_COLOR_WHITE			0xFFFF
#define ILI9341_COLOR_BLACK			0x0000
#define ILI9341_COLOR_RED			0xF800
#define ILI9341_COLOR_GREEN			0x07E0
#define ILI9341_COLOR_GREEN2		0xB723
#define ILI9341_COLOR_BLUE			0x001F
#define ILI9341_COLOR_BLUE2			0x051D
#define ILI9341_COLOR_YELLOW		0xFFE0
#define ILI9341_COLOR_ORANGE		0xFBE4
#define ILI9341_COLOR_CYAN			0x07FF
#define ILI9341_COLOR_MAGENTA		0xA254
#define ILI9341_COLOR_GRAY			0x7BEF
#define ILI9341_COLOR_BROWN			0xBBCA

// Transparent background, only for strings and chars
#define ILI9341_TRANSPARENT			0x80000000


#define Orientation_Portrait			0
#define Orientation_Portrait_Mirror		1
#define Orientation_Landscape			2
#define Orientation_Landscape_Mirror	3
class CDisplayTask : public iTask
{
private:
	static CDisplayTask *m_Instance;

	uint8_t m_bLandscape;
	uint8_t m_bInitialize;
	void SetCSPin(uint8_t bEnable);
	void SetDCPin(uint8_t bEnable);

	uint8_t SendCommand(uint8_t bCommand);
	uint8_t SendData(uint8_t bData);

	void InitializeLCD();
public:
	CDisplayTask();
	~CDisplayTask();
	void Run(void const *pParam);

	void HardReset(void);
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


	static CDisplayTask * GetInstance(void);
};
