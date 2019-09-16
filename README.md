# interference-monitor
Prevent transmission to interfere with a close air traffic control center

Include functions:

Function1:
Read rtx status from an Yaesu FT991A via CAT.
Calculate harmonix based on transmission frequency.
Calculate harmonics spread spectrum based on transmission type.
Compare with air traffic control frequency for a potential interference and stop transmission via CAT.

Function2:
Calculate the usability of the air traffic control frequency based on time and date; air area sectors has different period usage.
Send a udp message containig details of every transmission longer than one second to a remote machine and display them in a webserver.

Function3:
Translate from FT950 to FT991 protocols between two serials ports.

Function4:
Manage the rtx via a serial port connected to an bluetooth master/slave device.


Function5:
Generate a reference clock using a si5351 between 1mhz ad 200mhz.
Frequency from 0,00n hz to 1mhz can be generated with same jitter.

Hardware used:
Tft display ili9488 using dma with spi2 and touchscreen using spi2 without dma.
Ethernet controller w5500 using dma with spi2 to send transmission logs via udp .
Usart1 with tx/rx using irq to communicate with rs232 pc, port is electrically isolated with an adum1201.
Usart2 with dma tx and irq rx, to communicate with rs232 rtx (Yaesu FT991A), port is electrically isolated with an adum1201.
Usart3 with dma tx and irq rx, to communicate with any device using a bluetooth hc-05 module in master or slave.
Silab SI5351 via i2c to generate reference fequency for general pourpose.
24c64 to store air traffic frequency with possible usage based on time/date.
mcp23017 to manage 2 illuminated pushbutton with rgb led, and an alarm buzzer.

