# interference-monitor

Sorry for the native language on hte code; i will translate to english on the way.
This project is in an early development status, and i'm relatively new in programming.

This circuit prevent generation or radio interferences to a nearby air traffic control center.

Include functions:

Function:
Read rtx status from an Yaesu FT991A via CAT.  
Calculate harmonics and their spectrum based on transmission mode and frequency. (TODO)  
Compare with air traffic control frequency for a potential interference and stop transmission via CAT.  

Function:  
Calculate the usability of the air traffic control frequency based on time and date; air area sectors has different period usage.  
Send a udp message containig details of every transmission longer than one second to a remote machine, and display them in a webserver.  

Function:  
Translate from FT950 to FT991 protocols between two serials ports. (INCOMPLETE)  

Function:  
Manage the rtx via a serial port connected to an bluetooth master/slave device. (TODO)  

Function:  
Generate a reference clock using a si5351 between 1mhz ad 200mhz.  
Frequency from 0,00? hz to 1mhz can be generated but have jitter due to fractional divider on pll.  

Hardware used:
stm32f103c8t6 , bluepill version.
Tft display ili9488 using dma with spi2 and touchscreen using spi2 with dma. (NEED SOME FIXES)  
Ethernet controller w5500 using dma with spi2 to send transmission logs via udp.  
Usart1 with tx/rx using irq to communicate with rs232 pc, port is electrically isolated with an adum1201.
Usart2 with dma tx and irq rx, to communicate with rs232 rtx (Yaesu FT991A), port is electrically isolated with an adum1201.  
Usart3 with dma tx and irq rx, to communicate with any device using a bluetooth hc-05 module in master or slave.   
Silab SI5351 via i2c to generate reference frequency for general pourpose.  
24c64 to store air traffic frequency with possible usage based on time/date. (missing hardware, i had to reuse the chip elsewhere)
mcp23017 to manage 2 illuminated pushbutton with rgb led, and an alarm buzzer. (TODO)  

About the bluepill, i had to replace the two capacitors on the 32768Hz crystal because of the sclocck instability and fast discharging of a CR2032 battery; to original value is unknown, i had saome 13pf capacitors but i think that 12pf or 15 pf are fine; the clock need a calibration to slow, the default value i nthe code it's the best for my hardware.
