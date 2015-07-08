ESP8266 UART Multicast Bridge
=============================

Transmit UART data as multicast package.


Information
-----------

### Requirements
- Arduino sketch with ESP8266 support: https://github.com/esp8266/Arduino
- python3 environment
- USB to UART Adapter to flash the ESP8266

### General
 - UART Baudrate: 115200
 - Multicast IP: 239.0.0.57
 - Milticast Port: 12345
 - TCP Port: 9999
 - UDP Port: 9999

The Controller listen for Broadcast messages on th given UDP Port.
This conenction can be used to find the controller in your local network.<br>
The controller transmit all information which comes over the TCP connection directly to UART.

### Usage

Try the `first_try.py` with python3.
This script will set the esid and epass to connect to and wait for multicast packages.


License
-------
-- englisch version below -- <br>
Dieses Projekt ist lizensiert als Inhalt der Creative Commons Namensnennung - Weitergabe unter gleichen Bedingungen 3.0 Unported-Lizenz. <br>
Um eine Kopie der Lizenz zu sehen, besuchen Sie http://creativecommons.org/licenses/by-sa/3.0/.<br><br>
-- englisch version --<br>
This work is licensed under the Creative Commons Attribution-ShareAlike 3.0 Germany License. <br>
To view a copy of this license, visit http://creativecommons.org/licenses/by-sa/3.0/ or send a letter to
Creative Commons, 444 Castro Street, Suite 900, Mountain View, California, 94041, USA.<br><br>
by-sa Pascal Gollor (pascal@pgollor.de)<br><br>


Other libraries and sources
---------------------------
- EEProm: https://github.com/chriscook8/esp-arduino-apboot
- General ESP8266 information: https://github.com/esp8266/Arduino
