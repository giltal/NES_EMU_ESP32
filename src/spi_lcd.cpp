// Copyright 2015-2016 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <string.h>
#include <stdio.h>

#include "rom/ets_sys.h"
#include "rom/gpio.h"
#include "soc/gpio_reg.h"
#include "soc/gpio_sig_map.h"
#include "soc/gpio_struct.h"
#include "soc/io_mux_reg.h"
#include "soc/spi_reg.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/periph_ctrl.h"
#include "spi_lcd.h"
#include "psxcontroller.h"
#include "driver/ledc.h"
#include "nes.h"

// Gil
#include "graphics.h"
#include "SPI.h"
// Gil

void setBrightness(int bright)
{

}

#define U16x2toU32(m, l) ((((uint32_t)(l >> 8 | (l & 0xFF) << 8)) << 16) | (m >> 8 | (m & 0xFF) << 8))

extern uint16_t myPalette[];
// Gil
extern unsigned char Palette24[256][3];
extern ILI9488SPI_264KC lcd;

char *menuText[10] = {
    "Brightness 46 0.",
    "Volume     82 9.",
    " .",
    "Turbo  3 $  1 @.",
    " .",
    "These cause lag.",
    "Horiz Scale 1 5.",
    "Vert Scale  3 7.",
    " .",
    "*"};
bool arrow[9][9] = {{0, 0, 0, 0, 0, 0, 0, 0, 0},
                    {0, 0, 0, 0, 1, 0, 0, 0, 0},
                    {0, 0, 0, 1, 1, 1, 0, 0, 0},
                    {0, 0, 1, 1, 1, 1, 1, 0, 0},
                    {0, 1, 1, 1, 1, 1, 1, 1, 0},
                    {0, 0, 0, 1, 1, 1, 0, 0, 0},
                    {0, 0, 0, 1, 1, 1, 0, 0, 0},
                    {0, 0, 0, 1, 1, 1, 0, 0, 0},
                    {0, 0, 0, 0, 0, 0, 0, 0, 0}};
bool buttonA[9][9] = {{0, 0, 0, 0, 0, 0, 0, 0, 0},
                      {0, 0, 1, 1, 1, 1, 0, 0, 0},
                      {0, 1, 1, 0, 0, 1, 1, 0, 0},
                      {1, 1, 0, 1, 1, 0, 1, 1, 0},
                      {1, 1, 0, 1, 1, 0, 1, 1, 0},
                      {1, 1, 0, 0, 0, 0, 1, 1, 0},
                      {1, 1, 0, 1, 1, 0, 1, 1, 0},
                      {0, 1, 0, 1, 1, 0, 1, 0, 0},
                      {0, 0, 1, 1, 1, 1, 0, 0, 0}};
bool buttonB[9][9] = {{0, 0, 0, 0, 0, 0, 0, 0, 0},
                      {0, 0, 1, 1, 1, 1, 0, 0, 0},
                      {0, 1, 0, 0, 0, 1, 1, 0, 0},
                      {1, 1, 0, 1, 1, 0, 1, 1, 0},
                      {1, 1, 0, 0, 0, 1, 1, 1, 0},
                      {1, 1, 0, 1, 1, 0, 1, 1, 0},
                      {1, 1, 0, 1, 1, 0, 1, 1, 0},
                      {0, 1, 0, 0, 0, 1, 1, 0, 0},
                      {0, 0, 1, 1, 1, 1, 0, 0, 0}};
bool disabled[9][9] = {{0, 0, 0, 0, 0, 0, 0, 0, 0},
                       {0, 0, 1, 1, 1, 1, 0, 0, 0},
                       {0, 1, 0, 0, 0, 0, 1, 0, 0},
                       {1, 0, 0, 0, 0, 1, 0, 1, 0},
                       {1, 0, 0, 0, 1, 0, 0, 1, 0},
                       {1, 0, 0, 1, 0, 0, 0, 1, 0},
                       {1, 0, 1, 0, 0, 0, 0, 1, 0},
                       {0, 1, 0, 0, 0, 0, 1, 0, 0},
                       {0, 0, 1, 1, 1, 1, 0, 0, 0}};
bool enabled[9][9] = {{0, 0, 0, 0, 0, 0, 0, 0, 1},
                      {0, 0, 0, 0, 0, 0, 0, 1, 1},
                      {0, 0, 0, 0, 0, 0, 1, 1, 0},
                      {0, 0, 0, 0, 0, 1, 1, 0, 0},
                      {1, 0, 0, 0, 1, 1, 0, 0, 0},
                      {1, 1, 0, 1, 1, 0, 0, 0, 0},
                      {0, 1, 1, 1, 0, 0, 0, 0, 0},
                      {0, 0, 1, 0, 0, 0, 0, 0, 0},
                      {0, 0, 0, 0, 0, 0, 0, 0, 0}};
bool scale[9][9] = {{0, 0, 0, 0, 0, 0, 0, 0, 0},
                    {0, 0, 0, 0, 0, 0, 1, 1, 0},
                    {0, 0, 0, 0, 0, 0, 1, 1, 0},
                    {0, 0, 0, 0, 1, 1, 1, 1, 0},
                    {0, 0, 0, 0, 1, 1, 1, 1, 0},
                    {0, 0, 1, 1, 1, 1, 1, 1, 0},
                    {0, 0, 1, 1, 1, 1, 1, 1, 0},
                    {1, 1, 1, 1, 1, 1, 1, 1, 0},
                    {1, 1, 1, 1, 1, 1, 1, 1, 0}};
static bool lineEnd;
static bool textEnd;
#define BRIGHTNESS '0'
#define A_BUTTON '1'
#define DOWN_ARROW '2'
#define B_BUTTON '3'
#define LEFT_ARROW '4'
#define HORIZ_SCALE '5'
#define RIGHT_ARROW '6'
#define VERT_SCALE '7'
#define UP_ARROW '8'
#define VOL_METER '9'
#define TURBO_A '@'
#define TURBO_B '$'
#define EOL_MARKER '.'
#define EOF_MARKER '*'

#define ignore_border 4

static int lastShowMenu = 0;
static int counter = 0;
#define noSCALE_TO_320x280
#ifdef SCALE_TO_320x280
unsigned char tempLine[320 * 3];
void ili9488_write_frame(const uint16_t xs, const uint16_t ys, const uint16_t width, const uint16_t height, const uint8_t *data[], bool xStr, bool yStr)
{
	// Gil
	//printf("ili9488_write_frame %d %d\n", width, height);
	counter++;

	// 256x224
		if (data == NULL)
		{
			printf("No Data to display\n");
			return;
		}
	lcd.setXY(80, 40, 80 + 320 - 1, 40 + 280 - 1);
	unsigned int index,scaleX = 0, scaleY = 0;
	for (size_t i = 0; i < 224; i++)
	{
		index = 0;
		for (int j = 0; j < 256; j++)
		{
			//printf("%d\n", data[i][j]);
			tempLine[index + 0] = Palette24[(unsigned char)data[i][j]][0];
			tempLine[index + 1] = Palette24[(unsigned char)data[i][j]][1];
			tempLine[index + 2] = Palette24[(unsigned char)data[i][j]][2];
			index += 3;
			if (scaleX == 3)
			{
				scaleX = 0;
				tempLine[index + 0] = Palette24[(unsigned char)data[i][j]][0];
				tempLine[index + 1] = Palette24[(unsigned char)data[i][j]][1];
				tempLine[index + 2] = Palette24[(unsigned char)data[i][j]][2];
				index += 3;
			}
			else
				scaleX++;
		}
		SPI.writeBuffer((unsigned int *)tempLine, 320 * 3 / 4);
		if (scaleY == 3)
		{
			scaleY = 0;
			SPI.writeBuffer((unsigned int *)tempLine, 320 * 3 / 4);
		}
		else
			scaleY++;

	}
}
#else

unsigned char tempLine[256 * 3];

void ili9488_write_frame(const uint16_t xs, const uint16_t ys, const uint16_t width, const uint16_t height, const uint8_t *data[], bool xStr, bool yStr)
{

	// Gil
	counter++;
	int i;
	static unsigned long counter;
	//counter = ESP.getCycleCount();
	if (data == NULL)
	{
		printf("No Data to display\n");
		return;
	}
	lcd.setXY(70, 36, 70 + NES_SCREEN_WIDTH - 1, 36 + NES_SCREEN_HEIGHT - 1);
	unsigned int index;

	index = 0;
	unsigned char * buffer = (unsigned char*)data[0];
	unsigned int * intPointer;
	for (size_t i = 0; i < NES_SCREEN_HEIGHT * NES_SCREEN_WIDTH; i+=12)
	{
		tempLine[0] = Palette24[(unsigned char)buffer[i]][0];
		tempLine[1] = Palette24[(unsigned char)buffer[i]][1];
		tempLine[2] = Palette24[(unsigned char)buffer[i]][2];
		tempLine[3] = Palette24[(unsigned char)buffer[i + 1]][0];
		tempLine[4] = Palette24[(unsigned char)buffer[i + 1]][1];
		tempLine[5] = Palette24[(unsigned char)buffer[i + 1]][2];
		tempLine[6] = Palette24[(unsigned char)buffer[i + 2]][0];
		tempLine[7] = Palette24[(unsigned char)buffer[i + 2]][1];
		tempLine[8] = Palette24[(unsigned char)buffer[i + 2]][2];
		tempLine[9] = Palette24[(unsigned char)buffer[i + 3]][0];
		tempLine[10] = Palette24[(unsigned char)buffer[i + 3]][1];
		tempLine[11] = Palette24[(unsigned char)buffer[i + 3]][2];
		tempLine[12] = Palette24[(unsigned char)buffer[i + 4]][0];
		tempLine[13] = Palette24[(unsigned char)buffer[i + 4]][1];
		tempLine[14] = Palette24[(unsigned char)buffer[i + 4]][2];
		tempLine[15] = Palette24[(unsigned char)buffer[i + 5]][0];
		tempLine[16] = Palette24[(unsigned char)buffer[i + 5]][1];
		tempLine[17] = Palette24[(unsigned char)buffer[i + 5]][2];
		tempLine[18] = Palette24[(unsigned char)buffer[i + 6]][0];
		tempLine[19] = Palette24[(unsigned char)buffer[i + 6]][1];
		tempLine[20] = Palette24[(unsigned char)buffer[i + 6]][2];
		tempLine[21] = Palette24[(unsigned char)buffer[i + 7]][0];
		tempLine[22] = Palette24[(unsigned char)buffer[i + 7]][1];
		tempLine[23] = Palette24[(unsigned char)buffer[i + 7]][2];
		tempLine[24] = Palette24[(unsigned char)buffer[i + 8]][0];
		tempLine[25] = Palette24[(unsigned char)buffer[i + 8]][1];
		tempLine[26] = Palette24[(unsigned char)buffer[i + 8]][2];
		tempLine[27] = Palette24[(unsigned char)buffer[i + 9]][0];
		tempLine[28] = Palette24[(unsigned char)buffer[i + 9]][1];
		tempLine[29] = Palette24[(unsigned char)buffer[i + 9]][2];
		tempLine[30] = Palette24[(unsigned char)buffer[i + 10]][0];
		tempLine[31] = Palette24[(unsigned char)buffer[i + 10]][1];
		tempLine[32] = Palette24[(unsigned char)buffer[i + 10]][2];
		tempLine[33] = Palette24[(unsigned char)buffer[i + 11]][0];
		tempLine[34] = Palette24[(unsigned char)buffer[i + 11]][1];
		tempLine[35] = Palette24[(unsigned char)buffer[i + 11]][2];


		intPointer = (unsigned int *)tempLine;
		while (ESP32_SPI_USER_CMD_STATUS(ESP32_VSPI));
		ESP32_SPI_DATA_LEN(ESP32_VSPI, 36);
		ESP32_SPI_FIFO_WRITE(ESP32_VSPI, 0, intPointer[0]);
		ESP32_SPI_FIFO_WRITE(ESP32_VSPI, 4, intPointer[1]);
		ESP32_SPI_FIFO_WRITE(ESP32_VSPI, 8, intPointer[2]);
		ESP32_SPI_FIFO_WRITE(ESP32_VSPI, 12, intPointer[3]);
		ESP32_SPI_FIFO_WRITE(ESP32_VSPI, 16, intPointer[4]);
		ESP32_SPI_FIFO_WRITE(ESP32_VSPI, 20, intPointer[5]);
		ESP32_SPI_FIFO_WRITE(ESP32_VSPI, 24, intPointer[6]);
		ESP32_SPI_FIFO_WRITE(ESP32_VSPI, 28, intPointer[7]);
		ESP32_SPI_FIFO_WRITE(ESP32_VSPI, 32, intPointer[8]);

		ESP32_SPI_EXECUTE_USER_CMD(ESP32_VSPI);
	}
	//printf("Frames cycles: %d\n", ESP.getCycleCount() - counter);
}

#endif
void ili9488_init()
{
    lineEnd = textEnd = 0;
}
