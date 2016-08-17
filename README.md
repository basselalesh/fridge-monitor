#Fridge Monitor

The Arduino-HDK Shield was a prototype that later became a ready-for-production PCB that can be mounted on the WNC M18Q2 model for an LTE connection using AT Commands, UART, TDP sockets, and the AT&T M2X API. It was the end-of-internship project by me (Bassel Alesh)that utilized the different tests and mini-projects that were tackled over the summer and combined everything that was learned into a comprehensive, user-friendly project. The shield is a 2-layer PCB that containts the wiring for an Arduino Pro Mini and an Adafruit 4-bit Bi-Directional Level Shifter to the Hardware Development Kit (HDK) from WNC & their Qualcomm-based M18Q2 chipset. 

2.0.0 code includes: Set Time option, Posting every 1-1:30 minutes (depending on door closing and opening), Uses PIN0 and PIN1 for Hardware Serial UART communication, Reads from pins 5, 6, and 7 for sensor info and M2X
Pin 7 uses Wire and oneWire (based on I2C protocol) to post temperature data using a Dallas temperature sensor. Pins 5 and 6 use digitalRead to read IR sensors. 

