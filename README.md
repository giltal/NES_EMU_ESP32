# NES_EMU_ESP32
 NES emulator with ESP32 and ILI9488

Yet another NES emulator for the ESP32
This one is fully working! :-)
It is running on Intel's makers platform but can be very easily ported to run with any ESP32 with an ILI9488 (480x320) + Touch screen (not mandatory).
This module is similar to what we use on our platform:
https://www.buydisplay.com/lcd-3-5-inch-320x480-tft-display-module-optl-touch-screen-w-breakout-board

You will need to provide controls, we are using an analog joystick + 2 physical buttons, select and start are input from the touch screen.
Audio is driven through DAC25 and DAC26, you will need an amplified speaker (PC speakers for example)

The project is compiled in visual studio with visual micro plug in, or in the Arduino SDK (application)

It contains 8 ROMs within the code you can choose to launch.

You will need few more libraries located here: 
https://github.com/giltal/ESP32_Course/tree/main/Drivers

Just locate them in the Arduino library folder.
The modified SPI.h\c should be copied to the ESP32 location:

c:\Users\giltal\AppData\Local\arduino15\packages\esp32\hardware\esp32\1.0.4\libraries\SPI\src\

giltal is your specific user.

Have fun :-)
