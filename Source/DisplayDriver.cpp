#include "DisplayDriver.h"

uint16_t pCharBuffer[17 * 27] = { 0 };	// Буфер для выводимого символа

CDisplayDriver * CDisplayDriver::m_Instance = 0;
CDisplayDriver::CDisplayDriver()
{
	m_nCurrentX = 0;
	m_nCurrentY = 0;
	m_bLandscape = 0;
	m_bInitialize = 0;
}

CDisplayDriver::~CDisplayDriver()
{
}

CDisplayDriver * CDisplayDriver::GetInstance(void)
{
	if (m_Instance == 0)
	{
		m_Instance = new CDisplayDriver();
		m_Instance->Initialize();
	}
	return m_Instance;
}

void CDisplayDriver::InitSPI()
{
	// Линии SPI1 (Master)
	// Тактирование портов А, В, C и альтернативных функций
	RCC->APB2ENR |= (RCC_APB2ENR_AFIOEN | RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPBEN | RCC_APB2ENR_IOPCEN);

	//Очистка бит выбора режима
	GPIOA->CRL &= ~(GPIO_CRL_CNF7 | GPIO_CRL_CNF6 | GPIO_CRL_CNF5 | GPIO_CRL_CNF4 | GPIO_CRL_CNF2 | GPIO_CRL_CNF1 | GPIO_CRL_CNF0);

	// PA7(MOSI), PA5(SCK) - AF, Output, PP
	// PA4(NSS), PA2(DC), PA1(Reset), PA0(LED) - Output, PP
	GPIOA->CRL |= GPIO_CRL_CNF7_1 | GPIO_CRL_CNF6_0 | GPIO_CRL_CNF5_1;

	GPIOA->CRL |= GPIO_CRL_MODE7 | GPIO_CRL_MODE5 | GPIO_CRL_MODE4 | GPIO_CRL_MODE2 | GPIO_CRL_MODE1 | GPIO_CRL_MODE0;

	//Настройка SPI1 (Master) 8 бит данных, MSB передается первым, программный режим управления NSS вывод NSS (PA4) разрешено использовать в качестве выхода
	RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;		//Тактирование модуля SPI1
	SPI1->CR1 &= ~SPI_CR1_BR;				//Baud rate = Fpclk/2
	SPI1->CR1 &= ~SPI_CR1_CPOL;				//Полярность тактового сигнала
	SPI1->CR1 &= ~SPI_CR1_CPHA;				//Фаза тактового сигнала
	SPI1->CR1 &= ~SPI_CR1_DFF;				//8 бит данных
	SPI1->CR1 &= ~SPI_CR1_LSBFIRST;			//MSB передается первым
	SPI1->CR1 |= SPI_CR1_SSM;				//Программный режим NSS
	SPI1->CR1 |= SPI_CR1_SSI;				//Аналогично состоянию, когда на входе NSS высокий уровень
	SPI1->CR1 |= SPI_CR1_MSTR;				//Режим Master
	SPI1->CR2 |= SPI_CR2_SSOE;				//Вывод NSS - выход управления slave select
	SPI1->CR2 |= SPI_CR2_TXDMAEN;

	SPI1->CR1 |= SPI_CR1_SPE;				//Включаем SPI1

	// Инициализация DMA
	RCC->AHBENR |= RCC_AHBENR_DMA1EN;

	DMA1_Channel3->CCR |= DMA_CCR_MSIZE_0;
	DMA1_Channel3->CCR |= DMA_CCR_PSIZE_0;
	DMA1_Channel3->CCR |= DMA_CCR_MINC;
	DMA1_Channel3->CCR |= DMA_CCR_DIR;

	DMA1_Channel3->CMAR = (uint32_t)pCharBuffer;
	DMA1_Channel3->CPAR = (uint32_t) &(SPI1->DR);
}

void CDisplayDriver::SetCSPin(uint8_t bEnable)
{
	GPIOA->BSRR = bEnable ? GPIO_BSRR_BS4 : GPIO_BSRR_BR4;
}

void CDisplayDriver::SetDCPin(uint8_t bEnable)
{
	GPIOA->BSRR = bEnable ? GPIO_BSRR_BS2 : GPIO_BSRR_BR2;
}

void CDisplayDriver::BacklightEnable(uint8_t bEnable)
{
	GPIOA->BSRR = bEnable ? GPIO_BSRR_BS0 : GPIO_BSRR_BR0;
}

void CDisplayDriver::HardReset(void)
{
	GPIOA->BSRR = GPIO_BSRR_BS1;
	vTaskDelay(100);
	GPIOA->BSRR = GPIO_BSRR_BS1;
}

uint8_t CDisplayDriver::SendCommand(uint8_t bCommand)
{
	taskENTER_CRITICAL();
	SetDCPin(0);
	SetCSPin(0);

	SPI1->CR1 |= SPI_CR1_SPE;

	SPI1->DR = bCommand;
	while ((SPI1->SR & SPI_SR_TXE) != SPI_SR_TXE);

	SPI1->CR1 &= ~SPI_CR1_SPE;

	SetCSPin(1);
	taskEXIT_CRITICAL();
	return SPI1->DR;
}

uint8_t CDisplayDriver::SendData(uint8_t bData)
{
	taskENTER_CRITICAL();
	SetDCPin(1);
	SetCSPin(0);

	SPI1->CR1 |= SPI_CR1_SPE;

	SPI1->DR = bData;
	while ((SPI1->SR & SPI_SR_TXE) != SPI_SR_TXE);

	SPI1->CR1 &= ~SPI_CR1_SPE;

	SetCSPin(1);
	taskEXIT_CRITICAL();
	return SPI1->DR;
}

void CDisplayDriver::InitLCD()
{
	/* Force reset */
	HardReset();

	/* Delay for RST response */
	vTaskDelay(100);

	/* Software reset */
	SendCommand(Reset);

	vTaskDelay(100);

	SendCommand(PowerA);
	SendData(0x39); SendData(0x2C); SendData(0x00); SendData(0x34); SendData(0x02);
	SendCommand(PowerB);
	SendData(0x00); SendData(0xC1); SendData(0x30);
	SendCommand(DTCA);
	SendData(0x85); SendData(0x00); SendData(0x78);
	SendCommand(DTCB);
	SendData(0x00); SendData(0x00);
	SendCommand(PowerSeqence);
	SendData(0x64); SendData(0x03); SendData(0x12); SendData(0x81);
	SendCommand(PRC);
	SendData(0x20);
	SendCommand(Power1);
	SendData(0x23);
	SendCommand(Power2);
	SendData(0x10);
	SendCommand(VCOM1);
	SendData(0x3E); SendData(0x28);
	SendCommand(VCOM2);
	SendData(0x86);
	SendCommand(MAC);
	SendData(0x48);
	SendCommand(PixelFormat);
	SendData(0x55);
	SendCommand(FRC);
	SendData(0x00); SendData(0x18);
	SendCommand(DFC);
	SendData(0x08);	SendData(0x82);	SendData(0x27);
	SendCommand(GammaEn);
	SendData(0x00);
	SendCommand(ColumnAddress);
	SendData(0x00);	SendData(0x00);	SendData(0x00);	SendData(0xEF);
	SendCommand(PageAddress);
	SendData(0x00);	SendData(0x00);	SendData(0x01);	SendData(0x3F);
	SendCommand(Gamma);
	SendData(0x01);
	SendCommand(PGamma);
	SendData(0x0F);	SendData(0x31);	SendData(0x2B);	SendData(0x0C);	SendData(0x0E);
	SendData(0x08);	SendData(0x4E);	SendData(0xF1);	SendData(0x37);	SendData(0x07);
	SendData(0x10);	SendData(0x03);	SendData(0x0E);	SendData(0x09);	SendData(0x00);
	SendCommand(NGamma);
	SendData(0x00);	SendData(0x0E);	SendData(0x14);	SendData(0x03);	SendData(0x11);
	SendData(0x07);	SendData(0x31);	SendData(0xC1);	SendData(0x48);	SendData(0x08);
	SendData(0x0F);	SendData(0x0C);	SendData(0x31);	SendData(0x36);	SendData(0x0F);
	SendCommand(SleepOut);

	vTaskDelay(100);

	SendCommand(DisplayOn);
	SendCommand(GRAM);
}

void CDisplayDriver::Initialize()
{
	InitSPI();
	InitLCD();
	Fill(White);
	Rotate(Landscape);

	BacklightEnable(1);
	m_bInitialize = 1;
}

void CDisplayDriver::DrawPixel(uint16_t x, uint16_t y, uint32_t color) {
	SetCursorPosition(x, y, x, y);

	SendCommand(GRAM);
	SendData(color >> 8);
	SendData(color & 0xFF);
}
void CDisplayDriver::SetCursorPosition(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) {
	SendCommand(ColumnAddress);
	SendData(x1 >> 8);
	SendData(x1 & 0xFF);
	SendData(x2 >> 8);
	SendData(x2 & 0xFF);

	SendCommand(PageAddress);
	SendData(y1 >> 8);
	SendData(y1 & 0xFF);
	SendData(y2 >> 8);
	SendData(y2 & 0xFF);
}
void CDisplayDriver::Fill(uint16_t color)
{
	FillRectangle(0, 0, ILI9341_WIDTH - 1, ILI9341_HEIGHT, color);
}
void CDisplayDriver::FillRectangle(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color)
{
	uint32_t pixels_count;

	/* Set cursor position */
	SetCursorPosition(x0, y0, x1, y1);

	/* Calculate pixels count */
	pixels_count = (x1 - x0 + 1) * (y1 - y0 + 1);

	/* Set command for GRAM data */
	SendCommand(GRAM);
	taskENTER_CRITICAL();
	/* Send everything */
	SetCSPin(0);
	SetDCPin(1);

	DMA1_Channel3->CNDTR = (pixels_count > 0xFFFF) ? 0xFFFF : pixels_count;

	SPI1->CR1 |= SPI_CR1_DFF;
	SPI1->CR1 |= SPI_CR1_SPE;
	DMA1_Channel3->CMAR = (uint32_t)&color;
	DMA1_Channel3->CCR &= ~DMA_CCR_MINC;
	DMA1_Channel3->CCR |= DMA_CCR_EN;

	while ((SPI1->SR & SPI_SR_TXE) != SPI_SR_TXE);
	while ((SPI1->SR & SPI_SR_BSY) == SPI_SR_BSY);
	while ((DMA1->ISR & DMA_ISR_TCIF3) != DMA_ISR_TCIF3);
	if (pixels_count > 0xFFFF) {
		DMA1_Channel3->CCR &= ~DMA_CCR_EN;
		DMA1_Channel3->CNDTR = pixels_count - 0xFFFF;
		DMA1_Channel3->CCR |= DMA_CCR_EN;

		while ((SPI1->SR & SPI_SR_TXE) != SPI_SR_TXE);
		while ((SPI1->SR & SPI_SR_BSY) == SPI_SR_BSY);
		while ((DMA1->ISR & DMA_ISR_TCIF3) != DMA_ISR_TCIF3);
	}

	DMA1_Channel3->CCR &= ~DMA_CCR_EN;
	DMA1_Channel3->CCR |= DMA_CCR_MINC;
	SPI1->CR1 &= ~SPI_CR1_SPE;
	SPI1->CR1 &= ~SPI_CR1_DFF;
	taskEXIT_CRITICAL();
	SetCSPin(1);
}
void CDisplayDriver::DrawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint32_t color) {
	int16_t dx, dy, sx, sy, err, e2;
	uint16_t tmp;

	if (x0 > x1) {
		tmp = x0;
		x0 = x1;
		x1 = tmp;
	}
	if (y0 > y1) {
		tmp = y0;
		y0 = y1;
		y1 = tmp;
	}

	dx = x1 - x0;
	dy = y1 - y0;

	/* Vertical or horizontal line */
	if (dx == 0 || dy == 0) {
		FillRectangle(x0, y0, x1, y1, color);
		return;
	}

	sx = (x0 < x1) ? 1 : -1;
	sy = (y0 < y1) ? 1 : -1;
	err = ((dx > dy) ? dx : -dy) / 2;

	while (1) {
		DrawPixel(x0, y0, color);
		if (x0 == x1 && y0 == y1) {
			break;
		}
		e2 = err;
		if (e2 > -dx) {
			err -= dy;
			x0 += sx;
		}
		if (e2 < dy) {
			err += dx;
			y0 += sy;
		}
	}
}
void CDisplayDriver::DrawRectangle(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint32_t color) {
	DrawLine(x0, y0, x1, y0, color);
	DrawLine(x0, y0, x0, y1, color);
	DrawLine(x1, y0, x1, y1, color);
	DrawLine(x0, y1, x1, y1, color);
}
void CDisplayDriver::DrawFilledRectangle(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint32_t color) {
	uint16_t tmp;

	/* Check correction */
	if (x0 > x1) {
		tmp = x0;
		x0 = x1;
		x1 = tmp;
	}
	if (y0 > y1) {
		tmp = y0;
		y0 = y1;
		y1 = tmp;
	}

	/* Fill rectangle */
	FillRectangle(x0, y0, x1, y1, color);
}
void CDisplayDriver::DrawCircle(int16_t x0, int16_t y0, int16_t r, uint32_t color) {
	int16_t f = 1 - r;
	int16_t ddF_x = 1;
	int16_t ddF_y = -2 * r;
	int16_t x = 0;
	int16_t y = r;

	DrawPixel(x0, y0 + r, color);
	DrawPixel(x0, y0 - r, color);
	DrawPixel(x0 + r, y0, color);
	DrawPixel(x0 - r, y0, color);

	while (x < y) {
		if (f >= 0) {
			y--;
			ddF_y += 2;
			f += ddF_y;
		}
		x++;
		ddF_x += 2;
		f += ddF_x;

		DrawPixel(x0 + x, y0 + y, color);
		DrawPixel(x0 - x, y0 + y, color);
		DrawPixel(x0 + x, y0 - y, color);
		DrawPixel(x0 - x, y0 - y, color);

		DrawPixel(x0 + y, y0 + x, color);
		DrawPixel(x0 - y, y0 + x, color);
		DrawPixel(x0 + y, y0 - x, color);
		DrawPixel(x0 - y, y0 - x, color);
	}
}
void CDisplayDriver::DrawFilledCircle(int16_t x0, int16_t y0, int16_t r, uint32_t color) {
	int16_t f = 1 - r;
	int16_t ddF_x = 1;
	int16_t ddF_y = -2 * r;
	int16_t x = 0;
	int16_t y = r;

	DrawPixel(x0, y0 + r, color);
	DrawPixel(x0, y0 - r, color);
	DrawPixel(x0 + r, y0, color);
	DrawPixel(x0 - r, y0, color);
	DrawLine(x0 - r, y0, x0 + r, y0, color);

	while (x < y) {
		if (f >= 0) {
			y--;
			ddF_y += 2;
			f += ddF_y;
		}
		x++;
		ddF_x += 2;
		f += ddF_x;

		DrawLine(x0 - x, y0 + y, x0 + x, y0 + y, color);
		DrawLine(x0 + x, y0 - y, x0 - x, y0 - y, color);

		DrawLine(x0 + y, y0 + x, x0 - y, y0 + x, color);
		DrawLine(x0 + y, y0 - x, x0 - y, y0 - x, color);
	}
}
void CDisplayDriver::Rotate(int orientation) {
	SendCommand(MAC);
	if (orientation == Portrait) {
		SendData(0x58);
		m_bLandscape = 0;
	}
	else if (orientation == Portrait_Mirror) {
		SendData(0x88);
		m_bLandscape = 0;
	}
	else if (orientation == Landscape) {
		SendData(0x28);
		m_bLandscape = 1;
	}
	else if (orientation == Landscape_Mirror) {
		SendData(0xE8);
		m_bLandscape = 1;
	}
}
void CDisplayDriver::DrawString(uint16_t x, uint16_t y, char *str, Font_t *font, uint32_t foreground, uint32_t background) {
	uint16_t startX = x;

	/* Set X and Y coordinates */
	m_nCurrentX = x;
	m_nCurrentY = y;

	if (m_bInitialize != 1)
		return;
	while (*str) {
		/* New line */
		if (*str == '\n') {
			m_nCurrentY += font->FontHeight + 1;
			/* if after \n is also \r, than go to the left of the screen */
			if (*(str + 1) == '\r') {
				m_nCurrentX = 0;
				str++;
			}
			else {
				m_nCurrentX = startX;
			}
			str++;
			continue;
		}
		else if (*str == '\r') {
			str++;
			continue;
		}

		/* Put character to LCD */
		DrawChar(m_nCurrentX, m_nCurrentY, *str++, font, foreground, background);
	}
}
void CDisplayDriver::GetStringSize(char *str, Font_t *font, uint16_t *width, uint16_t *height) {
	uint16_t w = 0;
	*height = font->FontHeight;
	while (*str++) {
		w += font->FontWidth;
		if (*str == '\n')
			*height += font->FontHeight;
	}
	*width = w;
}
void CDisplayDriver::DrawChar(uint16_t x, uint16_t y, char c, Font_t *font, uint32_t foreground, uint32_t background) {
	m_nCurrentX = x;
	m_nCurrentY = y;

	if ((m_nCurrentX + font->FontWidth) > (m_bLandscape ? ILI9341_HEIGHT : ILI9341_WIDTH)) {
		m_nCurrentY += font->FontHeight;
		m_nCurrentX = 0;
	}

	memset(pCharBuffer, background, 17 * 27 * sizeof(uint16_t) - 1);
	for (uint8_t CharY = 0; CharY < font->FontHeight; CharY++) {
		uint16_t b = font->data[(c - 32) * font->FontHeight + CharY];
		for (uint8_t CharX = 0; CharX < font->FontWidth; CharX++) {
			if ((b << CharX) & 0x8000)
				pCharBuffer[CharY * (font->FontWidth) + CharX] = foreground;
		}
	}

	SetCursorPosition(m_nCurrentX, m_nCurrentY, m_nCurrentX + font->FontWidth - 1, m_nCurrentY + font->FontHeight - 1);

	SendCommand(GRAM);
	taskENTER_CRITICAL();
	SetDCPin(1);
	SetCSPin(0);
	DMA1_Channel3->CNDTR = font->FontHeight * font->FontWidth;
	DMA1_Channel3->CMAR = (uint32_t) pCharBuffer;
	SPI1->CR1 |= SPI_CR1_DFF;
	SPI1->CR1 |= SPI_CR1_SPE;
	DMA1_Channel3->CCR |= DMA_CCR_EN;

	while ((SPI1->SR & SPI_SR_TXE) != SPI_SR_TXE);
	while ((SPI1->SR & SPI_SR_BSY) == SPI_SR_BSY);

	DMA1_Channel3->CCR &= ~DMA_CCR_EN;
	SPI1->CR1 &= ~SPI_CR1_SPE;
	SPI1->CR1 &= ~SPI_CR1_DFF;
	SetCSPin(1);
	taskEXIT_CRITICAL();
	/* Set new pointer */
	m_nCurrentX += font->FontWidth;
}