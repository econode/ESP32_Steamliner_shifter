# ESP32 Streamliner shifter

ESP32StreamlinerShifter WiFi enabled gear shifter for streamliner motorcycle.
Written in Arduino C for the ESP32 embeded microcontroller.

The function of the gear shifter is to change through the gears on a landspeed streamliner motorcycle.
~1 second press on 'up' button to change up a gear, ~1 second press on 'down' button to change down a gear.

Long press on up when in 1st gear to 'half change' into neutral.

The gearbox shifter is connected via a linkage to a servo motor.

Paramaters need to be adjusted for this process to work smoothly, most notably the servo positons required for a gear change.

These paramaters are set through the ESP32's WiFi interface. So the whole process can be done trackside.

Once paramaters are set they are maintained between reboots in the ESP32's NVS storage.

Author: Matt Way 2023 (https://github.com/matt-kiwi)
This project is done in the spirt of OpenSource and the memory of Burt Munro the orginal DIY streamliner.

Please see Landspeed New Zealand (https://www.landspeed.org.nz/)

**Reason for writing this code sample**
This code is meant to be a simple working example of using the following technologies.
* ESP32 as WiFi access point
* Captive portal / DNS
* ESP32 EasyAsync Web Server
* * Serving files from LittleFS file system
* * HTML Template processing
* * Processing HTML input forms
* * Async HTML5 Web Sockets
* Arduino JSON
* Storing paramaters in non volatile NVS flash
* Interrupt driven button debouncing
* Servo control

I hope this sample code can be used as a starting point for other WiFi controlled ESP32 projects.

## Requirements
* VSCode with PlatformIO extension. See the following on how to install VSCode/PlatformIO (https://platformio.org/platformio-ide)
* Suitable USB cable
* Two push button switches and servo motor
* ESP32 Dev board (https://docs.platformio.org/en/latest/boards/espressif32/esp32dev.html)

## Building
After installing VSCode / PlatformIO IDE / Git
Clone the repository
```BASH
git clone https://github.com/econode/ESP32_Steamliner_shifter.git
```

Open the newly created project in PlatformIO IDE.


**Upload file system**

The project uses the LittleFS (https://randomnerdtutorials.com/esp32-write-data-littlefs-arduino/).
You will need to format the ESP32's flash partition system and upload the littleFS file system image. The image is created from the files in the project/data directory.

Each time you edit any of the files in project/data you will need to reflash the ESP32.

Select PlatformIO icon in the left pain then click Project Tasks / Platform / Upload Filesystem Image.

![Screenshot upload file system](docs/images/upload_file_system.png)

Change the pin definitions to match your hardware.
```C++
// PIN Defintions
#define PIN_BUTTON_UP 12
#define PIN_BUTTON_DOWN 13
#define PIN_SHIFTER_SERVO 14
#define BUTTON_DEBOUNCE_MS 50
```

**Compile and upload the firmware to the ESP32.**

Note: Most ESP32 development boards include a reset circuit, some boards you will need to manually.

Click on the compile & upload icon on the bottom of the IDE screen.

![Screenshot upload icon](docs/images/upload_firmware.png)



## To use
Connect to ESP32 WiFi access point.
Captive portal should redirect to 192.168.4.1 ,if not manually enter http://192.168.4.1 into your browser.

Note debug messages are outputed to the serial port @ 115,200 baud.

**Screenshot from SmartPhone**

![SmartPhone screenshot of shifter](docs/images/shifter_screenshot_19-12-2023.png)

## Change internal web server SSID / Password 
```C++
#define WIFI_SSID "AutoShifter"
#define WIFI_PASSWORD "123456789"
```

## License

[MIT](https://choosealicense.com/licenses/mit/)
