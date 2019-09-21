# interference-monitor

Sorry for the native language on tte code; i will translate to english on the way.
This project is in an early development status, and i'm relatively new in programming.

This circuit prevent generation or radio interferences to a nearby air traffic control center, something close to this https://img.over-blog-kiwi.com/2/73/49/23/20180527/ob_16258d_enav-acc.jpg

Include functions:

Read rtx status from an Yaesu FT991A via CAT.  
Calculate harmonics and their spectrum based on transmission mode and frequency. (TODO)  
Compare with air traffic control frequency for a potential interference and stop transmission via CAT.  
Calculate the usability of the air traffic control frequency based on time and date; air area sectors has different period usage.  
Send a log message via udp containig details of every transmission longer than one second to a remote web server.  
Translate from FT950 to FT991 protocols between two serials ports. (INCOMPLETE)  
Share the rtx cat to multiple programs (EARLY STAGE)  
Manage the rtx via a serial port connected to an bluetooth master/slave device. (TODO)  
Generate a reference clock using a si5351 between 1mhz ad 200mhz, frequency from 0,??? hz to 1mhz can also be generated but have jitter due to fractional divider on pll.  

Hardware used:
stm32f103c8t6 , bluepill version.
Tft display ili9488 using dma with spi2 and touchscreen using spi2 WITHOUT dma. (NEED SOME FIXES)  
Ethernet controller w5500 using dma with spi2 to send transmission logs via udp.  
Usart1 with tx/rx using irq to communicate with rs232 pc, port is electrically isolated with an adum1201.
Usart2 with dma tx and irq rx, to communicate with rs232 rtx (Yaesu FT991A), port is electrically isolated with an adum1201.  
Usart3 with dma tx and irq rx, to communicate with any device using a bluetooth hc-05 module in master or slave.  (TODO)   
Silab SI5351 via i2c to generate reference frequency for general pourpose.  
24c64 to store nonvolatile data. (suspended, i'm missing hardware because i had to reuse the chip elsewhere)
mcp23017 to manage 2 illuminated pushbutton with rgb led, and an alarm buzzer. (TODO)  

About the bluepill, i had to replace the two capacitors on the 32768Hz crystal because of the clock instability and fast discharging of a CR2032 battery; to original value is unknown: i had same 13pf capacitors but i think that 12pf or 15 pf are fine too; the clock need a calibration to slow down, the default value in the code it's the best for my hardware.
