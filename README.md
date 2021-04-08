# ceci
Our IoT sensor/actuator nodes based around the ESP8266

## Folder "ceci-hardware"
Contains the PCB files, desgined in kicad. The files are stored in a subrepo
available here:
https://github.com/broken-giobbe/ceci-hardware

### Required Libraries:
**TODO**

## Folder "cece-firmware"
Contains the firmware for the Arduino IDE.

### Required Libraries:
**TODO**

To write data into ESP8266 SPIFFS the
[Arduino ESP8266 filesystem uploader](https://github.com/esp8266/arduino-esp8266fs-plugin)
plugin is required.

After flashing the firmware rename data/config-example.ini to data/config.ini
and edit it to suit your flavour.

## Folder "octave-sim"
Contains the Octave simulation files to test the various heating algorithms.

