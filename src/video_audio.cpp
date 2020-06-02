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

#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include "soc/timer_group_struct.h"
#include "soc/timer_group_reg.h"
//#include "pretty_effect.h"
#include <math.h>
#include <string.h>
#include "noftypes.h"
#include "bitmap.h"
#include "nofconfig.h"
#include "event.h"
#include "gui.h"
#include "log.h"
#include "nes.h"
#include "nes_apu.h"
#include "nes_pal.h"
#include "nesinput.h"
#include "osd.h"
#include <stdint.h>
#include "driver/i2s.h"
// Gil
//#include "sdkconfig.h"
#include "spi_lcd.h"
#include "psxcontroller.h"
#include "graphics.h"

// Gil
#define I2S_DEVICE_ID 0
#define CONFIG_SOUND_ENABLED 1

#define _NO_I2S

#ifdef NO_I2S
#define NUM_OF_SAMPLES_PER_FRAME 368//256
#define DEFAULT_SAMPLERATE  (NUM_OF_SAMPLES_PER_FRAME*60)
#define AUDIO_BUFFER_LENGTH (DEFAULT_SAMPLERATE/60)
#define BITS_PER_SAMPLE 8
#else
#define DEFAULT_SAMPLERATE  22080
#define NUM_OF_SAMPLES_PER_FRAME DEFAULT_SAMPLERATE/60
#define BITS_PER_SAMPLE 16
#define BYTES_PER_SAMPLE 2
#define AUDIO_BUFFER_LENGTH (BYTES_PER_SAMPLE*DEFAULT_SAMPLERATE/60)
#endif
#define DEFAULT_WIDTH 256  //256
#define DEFAULT_HEIGHT 224 //NES_VISIBLE_HEIGHT

int xWidth;
int yHight;
float floatVolume;

TimerHandle_t timer;

//Seemingly, this will be called only once. Should call func with a freq of frequency,
int osd_installtimer(int frequency, void *func, int funcsize, void *counter, int countersize)
{
	printf("Timer install, freq=%d\n", frequency);
	timer = xTimerCreate("nes", configTICK_RATE_HZ / frequency, pdTRUE, NULL, (TimerCallbackFunction_t)func); // configTICK_RATE_HZ = CONFIG_FREERTOS_HZ
	xTimerStart(timer, 0);
	return 0;
}

/*
** Audio
*/
static int samplesPerPlayback = -1;
static void(*audio_callback)(void *buffer, int length) = NULL;
#if CONFIG_SOUND_ENABLED
QueueHandle_t queue;
static unsigned short *audio_buffer;
#endif

#ifdef NO_I2S
#include "esp32-hal-timer.h"
hw_timer_t * soundTimer = NULL;

unsigned char	* globalMusicData;
unsigned int	musicCounter = 0;
unsigned int	globalNumOfSamples = 0;
bool			musicIsPlaying = false;

void IRAM_ATTR onTimer()
{
	if (musicCounter == NUM_OF_SAMPLES_PER_FRAME)
	{
		if (soundTimer != NULL)
		{
			timerAlarmDisable(soundTimer);
			timerDetachInterrupt(soundTimer);
			timerEnd(soundTimer);
		}
		musicIsPlaying = false;
	}
	else
	{
		dacWrite(25, globalMusicData[musicCounter]);
		musicCounter++;
	}
}

unsigned char soundData[NUM_OF_SAMPLES_PER_FRAME];
#endif

void do_audio_frame()
{
#ifdef NO_I2S
	if (soundTimer != NULL)
	{
		while (timerAlarmEnabled(soundTimer));
		//printf("AS\n");
	}
	if (musicIsPlaying)
	{
		timerAlarmDisable(soundTimer);
		timerDetachInterrupt(soundTimer);
		timerEnd(soundTimer);
		musicIsPlaying = false;
		//dacWrite(25, 0);
		musicCounter = 0;
	}
	musicCounter = 0;
	musicIsPlaying = true;
	// Get Data
	apu_process((void *)soundData, NUM_OF_SAMPLES_PER_FRAME);

	globalNumOfSamples = NUM_OF_SAMPLES_PER_FRAME;
	globalMusicData = soundData;

	soundTimer = timerBegin(1, 80, true); // Set to run @ 1MHz
	timerAttachInterrupt(soundTimer, &onTimer, true);
	timerAlarmWrite(soundTimer, 1000000 / DEFAULT_SAMPLERATE, true);
	timerAlarmEnable(soundTimer);
#else
	apu_process((void *)audio_buffer, NUM_OF_SAMPLES_PER_FRAME);
	size_t written;
	i2s_write((i2s_port_t)I2S_DEVICE_ID, audio_buffer, AUDIO_BUFFER_LENGTH, &written, portMAX_DELAY);
#endif
}

void osd_setsound(void(*playfunc)(void *buffer, int length))
{
	audio_callback = playfunc;
}

static void osd_stopsound(void)
{
	audio_callback = NULL;
	printf("Sound stopped.\n");
	i2s_stop((i2s_port_t)I2S_DEVICE_ID);
	free(audio_buffer);
}

static int osd_init_sound(void)
{
	audio_buffer = (unsigned short *)malloc(AUDIO_BUFFER_LENGTH);
#ifdef NO_I2S
	musicCounter = 0;
	globalNumOfSamples = 0;
	musicIsPlaying = false;
#else
	i2s_config_t cfg = {
		.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_DAC_BUILT_IN),
		.sample_rate = DEFAULT_SAMPLERATE,
		.bits_per_sample = (i2s_bits_per_sample_t)BITS_PER_SAMPLE,
		.channel_format = (i2s_channel_fmt_t)I2S_CHANNEL_FMT_ONLY_RIGHT,
		.communication_format = (i2s_comm_format_t)I2S_COMM_FORMAT_I2S_MSB,
		.intr_alloc_flags = ESP_INTR_FLAG_INTRDISABLED,
		.dma_buf_count = 2,
		.dma_buf_len = NUM_OF_SAMPLES_PER_FRAME,
		.use_apll = false };
	i2s_driver_install((i2s_port_t)I2S_DEVICE_ID, &cfg, 0, NULL);
	i2s_set_pin((i2s_port_t)I2S_DEVICE_ID, NULL);
	i2s_set_sample_rates((i2s_port_t)I2S_DEVICE_ID, DEFAULT_SAMPLERATE);
	printf("Finished initializing sound\n");
#endif
	audio_callback = NULL;

	return 0;
}

void osd_getsoundinfo(sndinfo_t *info)
{
	info->sample_rate = DEFAULT_SAMPLERATE;
	info->bps = BITS_PER_SAMPLE;
}
/*
** Video
*/

static int init(int width, int height);
static void shutdown(void);
static int set_mode(int width, int height);
static void set_palette(rgb_t *pal);
static void clear(uint8 color);
static bitmap_t *lock_write(void);
static void free_write(int num_dirties, rect_t *dirty_rects);
static void custom_blit(bitmap_t *bmp, int num_dirties, rect_t *dirty_rects);
static char fb[1]; //dummy

QueueHandle_t vidQueue;

viddriver_t sdlDriver =
	{
		"Simple DirectMedia Layer", /* name */
		init,						/* init */
		shutdown,					/* shutdown */
		set_mode,					/* set_mode */
		set_palette,				/* set_palette */
		clear,						/* clear */
		lock_write,					/* lock_write */
		free_write,					/* free_write */
		custom_blit,				/* custom_blit */
		false						/* invalidate flag */
};

bitmap_t *myBitmap;

void osd_getvideoinfo(vidinfo_t *info)
{
	info->default_width = DEFAULT_WIDTH;
	info->default_height = DEFAULT_HEIGHT;
	info->driver = &sdlDriver;
}

/* flip between full screen and windowed */
void osd_togglefullscreen(int code)
{
}

/* initialise video */
static int init(int width, int height)
{
	return 0;
}

static void shutdown(void)
{
}

/* set a video mode */
static int set_mode(int width, int height)
{
	return 0;
}

//uint16 myPalette[256];
unsigned char Palette24[256][3];

/* copy nes palette over to hardware */
static void set_palette(rgb_t *pal)
{
	uint16 c;

	int i;

	for (i = 0; i < 256; i++)
	{
		//c = (pal[i].b >> 3) + ((pal[i].g >> 2) << 5) + ((pal[i].r >> 3) << 11);
		//myPalette[i] = c;
		// Gil
		Palette24[i][0] = pal[i].r; // 8 bit to 6 bit
		Palette24[i][1] = pal[i].g;
		Palette24[i][2] = pal[i].b;
	}
}

/* clear all frames to a particular color */
static void clear(uint8 color)
{
	//   SDL_FillRect(mySurface, 0, color);
}

/* acquire the directbuffer for writing */
static bitmap_t *lock_write(void)
{
	//   SDL_LockSurface(mySurface);
	myBitmap = bmp_createhw((uint8 *)fb, xWidth, yHight, xWidth * 2); //DEFAULT_WIDTH, DEFAULT_HEIGHT, DEFAULT_WIDTH*2);
	return myBitmap;
}

/* release the resource */
static void free_write(int num_dirties, rect_t *dirty_rects)
{
	bmp_destroy(&myBitmap);
}

static int counter = 0;
bool videoTaskInProgress;
//*(volatile unsigned int*)0x50000000

static void custom_blit(bitmap_t *bmp, int num_dirties, rect_t *dirty_rects)
{
	if (videoTaskInProgress == false)
	{
		xQueueSend(vidQueue, &bmp, 0);
	}
	do_audio_frame();
}

//This runs on core 1.
static void videoTask(void *arg)
{
	int x, y;
	bitmap_t *bmp = NULL;
	
	xWidth = DEFAULT_WIDTH;
	yHight = DEFAULT_HEIGHT;
	x = (DEFAULT_WIDTH - xWidth) / 2;
	y = ((DEFAULT_HEIGHT - yHight) / 2);
	while (1)
	{
		xQueueReceive(vidQueue, &bmp, portMAX_DELAY);
		videoTaskInProgress = true;
		ili9488_write_frame(x, y, xWidth, yHight, (const uint8_t **)bmp->line, /*getXStretch()*/0, /*getYStretch()*/0);
		videoTaskInProgress = false;
	}
}

/*
** Input
*/

static void osd_initinput()
{
	psxcontrollerInit();
}

void osd_getinput(void)
{
	// Note: These are in the order of PSX controller bitmasks (see psxcontroller.c)
	const int ev[16] = {
		event_joypad1_select, 0, 0, event_joypad1_start, event_joypad1_up, event_joypad1_right, event_joypad1_down, event_joypad1_left,
		0, event_hard_reset, 0, event_soft_reset, 0, event_joypad1_a, event_joypad1_b, 0};
	static int oldb = 0xffff;
	int b = psxReadInput();
	int chg = b ^ oldb;
	int x;
	oldb = b;
	event_t evh;
	//	printf("Input: %x\n", b);
	for (x = 0; x < 16; x++)
	{
		if (chg & 1)
		{
			evh = event_get(ev[x]);
			if (evh)
				evh((b & 1) ? INP_STATE_BREAK : INP_STATE_MAKE);
		}
		chg >>= 1;
		b >>= 1;
	}
}

static void osd_freeinput(void)
{
}

void osd_getmouse(int *x, int *y, int *button)
{
}

/*
** Shutdown
*/

/* this is at the bottom, to eliminate warnings */
void osd_shutdown()
{
	osd_stopsound();
	osd_freeinput();
}

static int logprint(const char *string)
{
	return printf("%s", string);
}

/*
** Startup
*/

int osd_init()
{
	log_chain_logfunc(logprint);

	if (osd_init_sound())
		return -1;
	printf("free heap after sound init: %d\n", xPortGetFreeHeapSize());
	videoTaskInProgress = false;
	floatVolume = 1.0;
	ili9488_init();
	ili9488_write_frame(0, 0, DEFAULT_WIDTH, DEFAULT_HEIGHT, NULL, 0, 0);
	vidQueue = xQueueCreate(1, sizeof(bitmap_t *));
	xTaskCreatePinnedToCore(&videoTask, "videoTask", 2048, NULL, 0, NULL, 0);
	osd_initinput();
	printf("free heap after input init: %d\n", xPortGetFreeHeapSize());
	return 0;
}
