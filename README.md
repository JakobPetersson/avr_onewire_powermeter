AVR 1-Wire Power Meter
======================
## Background
This project builds upon the work of Tobias Müller which implemented the functionality of a few 1-Wire devices using an AVR microprocessor. The original project can be found here: [http://tobynet.de/index.php/1-wire-device-mit-avr](http://tobynet.de/index.php/1-wire-device-mit-avr)

I thank him greatly for his work. 
It is important, especially since the DS2423 has been discontinued.

## New features
I hope to add a few new features, and I have already implemented an (experimental) version of the DS2423 which is aimed at measuring power consumption. The DS2423 was popularly used for counting LED impulses on home electricity meters to keep track of their consumption over longer period of time.
This version allows you to keep doing that, but also adds the functionality of calculating the current consumption in Watts.

Since the DS2423 has two counters one of them are used to keep the count and the other one has the last measured power consumption. The measurements are based on the duration of time between the impulses.