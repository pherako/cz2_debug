# cz2_debug
original code from borrowed with MIT graciousness from jwracd/CZII_to_MQTT

This was a bridge for openHabian using MQTT protocol and JSON. This project
is to provide a simple way to read and write packets in and out of the CZII
master controller. This is address 9 on the bus.

I am using a FTDI USB-RS485-WE-1800-BT CABLE from Amazon and a RPI for
development.

To read/write packets, configure the tty port for 9600 baud. I wrote a bash
script `setup_tty` to speed things up.

Reference Docs: http://tinyurl.com/y93zlnyk (http://esmithair.com/wp-content/uploads/2015/03/Comfort_Zone_II.pdf)
Installer Guide: http://tinyurl.com/ycldjnep (http://s7d2.scene7.com/is/content/Watscocom/Gemaire/carrier_zonecc2kit01-b_article_1391689392285_en_ii1.pdf?fmt=pdf)

Temperature Sensors if needed are 10k-ohm  NTC type (common refridgeration).
There are jillions at most electronics supply houses. Pick one you like.
Digi-Key: BC2647-ND (NTCLE413E2103F102L) to get starteda

# using the tool

`cat /dev/ttyUSB0 > log.bin` write a binary file of transactions

`./cz2_debug log.bin` to parse the file

`./cz2_debug /dev/ttyUSB0` to monitor traffic in real-time

I'm using cz2_debug to parse the file and work with completing the API. Every installation is different - see the installer guide. There may be multiple thermostats or just one.
  - the newest children in a strange and uncertain cosmos should listen quietly for a long time, patiently learning about the universe and comparing notes, before shouting into an unknown jungle that we do not understand.
