# miDisplay - Xiaomi m365
# Change Log
1.43 fix light and cruise control settings in menu

1.42 nobus, display sv

1.41 Lock image fix

image generator http://www.majer.ch/lcd/adf_bitmap.php

1.40 12s battery info on fsbattinfo, if only 10s then display exit info.

1.39 lock code info bug fixed - lock/unlock code works !

1.35 lock info more

1.32  lock info

1.31: bugs

1.30: unlock Code 3digit

1.25 temp battery

1.2: Lock, save settings m365.

1.1: Hibernate

1.0: Wheel size

# Products Used  
Arduino Pro Mini    
I2C OLED 0.96" Screen    
FTD1232 Usb Programmer   
3d Printed Bracket  
1N4148 Diode  
0.25w 120ohm Resistor       

Estimated price is around 10$ (Inluding Printed Parts).

# Flashing  
![alt text](https://i.imgur.com/DpPkvJz.jpg)  
Please install the libraries I provided in the files, install them to you arduino library folder, usually              
  C:\Users\\%username%\Documents\Arduino\libraries  
I'd recommend you to use Arduino 1.6.6  
https://www.arduino.cc/en/Main/OldSoftwareReleases  

# Physical Connections  
![alt text](https://camo.githubusercontent.com/a912641249173768ae60670e843c62294d06da4f/68747470733a2f2f656c656374726f2e636c75622f646174612f666f72756d2f6d657373616765732f34323633312f696d616765732f31313636302d313238302e6a7067)  

# Known Issues  
Sometimes the Arduino Freezes, a watchdog is in place but doesn't always trigger.  

# Screen caps
# Soldering, soldered directly to the cable coming from the MCU
5V To Red    
GND To Black  
BUS To Yellow  
![alt text](https://i.imgur.com/3ZwcrIJ.jpg)  
A video on how everything is soldered may come soon.

Meanwhile you can enable subtitles in English in this YouTube video produced in Spanish languaje
https://www.youtube.com/watch?v=JQUNXCyj2Fs

# UI
UI pictures from version 0.2  
![alt text](https://i.imgur.com/8ekMdIo.jpg)  
![alt text](https://i.imgur.com/AHLVTcu.jpg)  
More pictures are coming soon.

# Contributors
First Developer made hard work with dataFSM - https://electro.club/forum/displey_dlya_syaokata 

and his version on GitHub https://github.com/fogbox/m365_display

Second Developer _xxx_ made such nice Menu, nice Font and fix bugs(cruise control). Thanks to him it works very good, and looks very good.

Third Augisbud, make languages support and fix https://github.com/augisbud/m365_dashboard

help him "Trankilloman" and "Саша" sasha1804sk@gmail.com and nebman(remove custom library, add fonts to project)

and I only make some addons ;) (PL Lang, WheelSize, Lock Code, Battery 12s ...)

