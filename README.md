This folder contains the current version of the LoRaTracker Receiver2 programs, PCB layouts and circuit diagrams. 

This receiver program is designed as bothe a bench terminal based receiver and a poratble receiver with GPS and display.
The receiver runs on a ATMEGA1284P processor at 3.3V and 8Mhz. 

The terminal part of the program provides the remote control interface to the HAB tracker that allows it to
be controlled and configured whilst it is in flight.

The programs requre the installation of the current LoRaTracker Library files see here;

https://github.com/LoRaTracker/LoRaTracker-Library

The programs in here are;

Receiver2_xxxxxx - A HAB tracker receiver for bench terminal and portable use

I2C_Scanner - a basic I2C scanner, reports any I2C devices found
