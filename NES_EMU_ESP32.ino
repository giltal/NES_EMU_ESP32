/*
 Name:		NES32_IYH.ino
 Created:	4/27/2020 12:02:28 PM
 Author:	giltal
*/

#include "freertos/FreeRTOS.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "src/psxcontroller.h"
#include "src/nofrendo.h"

#include "GeneralLib.h"
#include <Adafruit_FT6206.h>
#include "PCF8574.h"
#include <Wire.h>
#include <RTClib.h>
#include "graphics.h"
#include "NESrom.h"
#include "TV.h"

#define SPEAKER_PIN 5
#define MP_BUTTON_1	27
#define MP_BUTTON_2	33
#define TOUCH_1_PIN	15
#define TOUCH_2_PIN	32
#define JOY_X_PIN	36
#define JOY_Y_PIN	39

PCF8574 pcf8574(0x39);
Adafruit_FT6206 ts = Adafruit_FT6206();

analogJoyStick JS(JOY_X_PIN, JOY_Y_PIN, 5);

ILI9488SPI_264KC lcd(480, 320);

const unsigned char * ROMtoLoad;

// the setup function runs once when you press reset or power the board
void setup()
{
	Serial.begin(115200);

	// Our platform related code
	// Setup the IOs
	pinMode(34, INPUT);		// Touch pannel interrupt
	pinMode(36, ANALOG);	// Joystick X
	pinMode(39, ANALOG);	// Joystick Y
	pinMode(MP_BUTTON_1, INPUT);
	pinMode(MP_BUTTON_2, INPUT);
	// Setup the speaker
	pinMode(SPEAKER_PIN, OUTPUT);
	//pinMode(25, OUTPUT);

	lcd.init();

	unsigned long ticks;
	ticks = ESP.getCycleCount();
	lcd.fillScr(0, 0, 0);
	ticks = ESP.getCycleCount() - ticks;
	printf("LCD FPS: %d\n", 240000000 / ticks);

	printf("Joystick initialization, please don't touch it!!\n");
	JS.init();
	// Setup the touch pannel
	delay(250);

	if (!ts.begin(20))
	{
		Serial.println("Unable to start touchscreen.");
	}
	else
	{
		Serial.println("Touchscreen started.");
	}
}

#define ASSERT_ESP_OK(returnCode, message)                          \
	if (returnCode != ESP_OK)                                       \
	{                                                               \
		printf("%s. (%s)\n", message, esp_err_to_name(returnCode)); \
		return returnCode;                                          \
	}

char *osd_getromdata()
{
	return (char *)ROMtoLoad;
}

void esp_wake_deep_sleep()
{
	esp_restart();
}

esp_err_t event_handler(void *ctx, system_event_t *event)
{
	return ESP_OK;
}

void loop()
{
	// Pick a game menu
	lcd.fillScr(0, 0, 0);
	int xLoc = 20;
	lcd.drawCompressed24bitBitmap(xLoc, 20, SuperMarioBrosIcon);
	lcd.drawCompressed24bitBitmap(xLoc, 200, SpaceInvadersIcon);
	xLoc += 110;
	lcd.drawCompressed24bitBitmap(xLoc, 20, DonkeyKongIcon);
	lcd.drawCompressed24bitBitmap(xLoc, 200, PacManbmpIcon);
	xLoc += 110;
	lcd.drawCompressed24bitBitmap(xLoc, 20, GalaxianIcon);
	lcd.drawCompressed24bitBitmap(xLoc, 200, GalagaIcon);
	xLoc += 110;
	lcd.drawCompressed24bitBitmap(xLoc, 20, DonkeyKongJRIcon);
	lcd.drawCompressed24bitBitmap(xLoc, 200, ArkniodIcon);
	unsigned short x, y, x2, y2;
	while (1)
	{
		if (TOUCH_PANNEL_TOUCHED())
		{
			ts.getTouchedPoints(&x, &y, &x2, &y2);
			if (x > 20 && x < 120 && y > 200)
			{
				ROMtoLoad = spaceinvaders;
				break;
			}
		}
		if (TOUCH_PANNEL_TOUCHED())
		{
			ts.getTouchedPoints(&x, &y, &x2, &y2);
			if (x > 130 && x < 230 && y > 200)
			{
				ROMtoLoad = PacMan;
				break;
			}
		}
		if (TOUCH_PANNEL_TOUCHED())
		{
			ts.getTouchedPoints(&x, &y, &x2, &y2);
			if (x > 240 && x < 340 && y > 200)
			{
				ROMtoLoad = Galaga;
				break;
			}
		}
		if (TOUCH_PANNEL_TOUCHED())
		{
			ts.getTouchedPoints(&x, &y, &x2, &y2);
			if (x > 350 && x < 450 && y > 200)
			{
				ROMtoLoad = arkniod;
				break;
			}
		}

		if (TOUCH_PANNEL_TOUCHED())
		{
			ts.getTouchedPoints(&x, &y, &x2, &y2);
			if (x > 20 && x < 120 && y < 120)
			{
				ROMtoLoad = SuperMarioBros;
				break;
			}
		}
		if (TOUCH_PANNEL_TOUCHED())
		{
			ts.getTouchedPoints(&x, &y, &x2, &y2);
			if (x > 130 && x < 230 && y < 120)
			{
				ROMtoLoad = DonkeyKong;
				break;
			}
		}
		if (TOUCH_PANNEL_TOUCHED())
		{
			ts.getTouchedPoints(&x, &y, &x2, &y2);
			if (x > 240 && x < 340 && y < 120)
			{
				ROMtoLoad = Galaxian;
				break;
			}
		}
		if (TOUCH_PANNEL_TOUCHED())
		{
			ts.getTouchedPoints(&x, &y, &x2, &y2);
			if (x > 350 && x < 450 && y < 120)
			{
				ROMtoLoad = Donkeykongjr;
				break;
			}
		}
	}

	// 
	lcd.drawCompressedGrayScaleBitmap(0, 0, TV);
	
	printf("NoFrendo start!\n");
	nofrendo_main(0, NULL);
	printf("NoFrendo died? WtF?\n");
	asm("break.n 1");
	while (1);
}
