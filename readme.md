## Enhanced Digital GPS Clock stand with satellite tracking
Out of all the clock models available, atomic clocks are the most accurate. However, they are obviously too expensive for everyday use. Quartz clocks are affordable, but they require manual adjustments from time to time. This can be a pain during daylight saving time (DST). That's why I've decided to build a GPS clock. It offloads all the time-related adjustments to satellites and is inexpensive to create as a DIY project. 

https://github.com/cookieDroid/GPS_clock/assets/69075360/3920e605-b263-456f-96a0-0024066878fb

In addition to the GPS receiver, the DIY clock build requires a microcontroller for processing the received data and a display for showcasing the date/time.

GPS Module
> For this project I am using the GPS that comes integrated with Waveshare SIM7600 LTE RPI hat module.
![sim7600_lte_gps](https://github.com/cookieDroid/GPS_clock/assets/69075360/a4d77835-bdd0-4bf5-9905-e5c8abece2ad)

Microcontroller
> Since I needed a 3 UART for processing debug information, connection with GPS module and the display, I used ESP32 as my MCU. Which to be honest is an overkill for this project.
![esp32](https://github.com/cookieDroid/GPS_clock/assets/69075360/1b4048c7-3cff-4153-a4c2-78c01c60a442)

Display
> While you can use even a simple LCD display, I wanted the product to look visually pleasing and decided to use DWIN HMI display. It supports UART interfacing, has capacitive touch and inbuilt buzzer. 
![dwin_display](https://github.com/cookieDroid/GPS_clock/assets/69075360/ef488589-d8e4-4467-9f21-d2ea3bfd87d4)

Power Supply
> Now that we have acquired all the materials, only thing we need to provide is power supply. Since both the display and the LTE modem consumes significant amount of power, I decided to power them up using Waveshare USB to UART/SPI/I2C converter. I highly recommend buying one for testing any boards/kits that supports the mentioned protocols. It is a low cost product that can be used to communicate with 2 UART devices at same time and has support for JTAG, limited supported to communicate with SPI/I2C devices.
![waveshare_usb](https://github.com/cookieDroid/GPS_clock/assets/69075360/ae08dc13-03f6-4a4d-9037-1e39bd9039ca)

Stand
> Since I had a rectangular display, I checked online for various clock models that can be used as a casing. Few number of models that could serve the purpose are of different size and could not fit the display. So I decided to turn a 3D printed pen stand with mobile holder that I already had into a clock stand. I printed this 3D model for this stand from thingiverse(It has been some time since I printed this stand, will attribute once I find the link to the 3D model). The display fit snuggly in the holder.
![mobile_holder](https://github.com/cookieDroid/GPS_clock/assets/69075360/de4b9601-1881-4a5b-b7a0-1c74a1aad365)

## Putting it all together
Now we will start wiring all the devices together.
Both the display and GPS module are powered by the Waveshare USB converter module, while the ESP32 is powered by a Type C to Micro USB B cable. All of them are connected to my laptop.
![esp32_gps_display](https://github.com/cookieDroid/GPS_clock/assets/69075360/58150625-1a8f-42c4-afb7-7993b2c758ea)

Since the antenna needs to be exposed towards the sky, I placed it near the window. Unlike the short cabled Ublox GPS modules, we get a really long antenna cable with Waveshare LTE hat module which is really convenient if you want to keep the module indoor.

![antenna](https://github.com/cookieDroid/GPS_clock/assets/69075360/e60dadbf-954c-43c5-b552-bccc9fc64482)

![esp32_gps_display_antenna](https://github.com/cookieDroid/GPS_clock/assets/69075360/c20ea369-9ebd-4730-a7d1-0caa7b2b360f)

Once everything is put together and powered up, we will be greeted with the current time in UTC format after few seconds.
![time_utc](https://github.com/cookieDroid/GPS_clock/assets/69075360/2563e7e4-c3f1-43b0-acfa-b39cbcb1abc3)

## Features
## Changing timezone
As mentioned above, the datetime is always in UTC format. I have added few more time zones so that anyone who wants to recreate it can change it to their local timezone. Current IST, PST and JST are supported.
## GPS co-ordinate
Since I wanted to make the clock offline, I have no means to query the location data using co-ordinates, so I hardcoded the location. You can see the latitude and longitude in Nerd stats page. The altitude represents how high the land is above sea level. The speed section shows how fast you are moving, which is useful if you keep the clock in a moving vehicle.
## Satelite tracking
I have added an extra page to show the number of GPS/GLONASS/BEIDOU satelites orbiting above. If you keep the page open for a long time, you can actually see the values changing. Unfortunately when I was testing, the values did not change much.
## Alarm [work in progress]
1. User selected ringtone
2. Customisable timing
3. On/Off
4. Snooze option
5. Snooze period customisation
I wanted to add alarm option as well, but all the features that comes with alarm will take some time so I have left it as mockup for now.

https://github.com/cookieDroid/GPS_clock/assets/69075360/ea8a9a78-a746-4f85-85d2-3d0309cff026

# Understanding the GNSS statement
The GNSS information received has a variety of information such as 
1. Day and Date Time in UTC formtat
2. Number of satellites orbiting above
3. Latitude and Longitude
4. Speed if the GPS receiver is in transit
5. Altitude above the sea level
6. Some precision details.
   
String format is specified as below, taken from SIMCOM's website.
![image](https://github.com/cookieDroid/GPS_clock/assets/69075360/7d8aeb0c-5c83-4701-8695-80b8ae5a95a5)

## Reference
1. https://github.com/davidledwards/gps-clock
2. https://stackoverflow.com/a/2413463
3. https://www.thingiverse.com/
4. https://www.flaticon.com/
5. https://www.youtube.com/playlist?list=PLKfWyFPPaoDoNWXWGr1tCCIvTxLw5O634
6. https://github.com/cookieDroid/GPS_clock
