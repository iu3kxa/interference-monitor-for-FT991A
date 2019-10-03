# interference-monitor

Sorry for the native language on the code; i will translate to english on the way.
This project is in an early development status, and i'm relatively new in programming.

This circuit prevent generation or radio interferences to a nearby air traffic control center, something close to this https://img.over-blog-kiwi.com/2/73/49/23/20180527/ob_16258d_enav-acc.jpg

Include functions:

Read rtx status from an Yaesu FT991A via rs232 CAT.  
Calculate harmonics and their spectrum based on transmission mode and frequency. (working, need same tweaks)  
Compare with air traffic control frequency for a potential interference and stop transmission via CAT:  
note that the command TX0; doesn't work if the transmission is originated from a different source like the usb/serial cat or ptt so the only way is to interact with the mic ptt or another way (need more testing);  
calculate the usability of the air traffic control frequency based on day time and date (the air traffic area is splitted in sectors where the coverage area is dinamic and depend on the traffic load that may vary a lot, usually only 1-2 on night hours).  
Send a log message via udp containig details of every transmission longer than one second, to a remote web server that can be viewed from anywhere knowing the exact url.  
Translate from FT950 to FT991 protocols between two serials ports. (partial, working with hrd5)  
Share the rtx cat to multiple programs (partial)  
Manage the rtx via a serial port connected to an bluetooth master/slave device. (no code at the moment)  
Generate a reference clock using a si5351 between 1,5khz and 200mhz. 

Hardware used:
stm32f103c8t6 , bluepill version.
Tft display ili9488 using dma with spi2 and touchscreen using spi2 WITHOUT dma. (NEED SOME FIXES)  
Ethernet controller w5500 using dma with spi2 to send transmission logs via udp.  
Usart1 with tx/rx using irq to communicate with rs232 pc, port is electrically isolated with an adum1201.
Usart2 with dma tx and irq rx, to communicate with rs232 rtx (Yaesu FT991A), port is electrically isolated with an adum1201.  
Usart3 with dma tx and irq rx, to communicate with any device using a bluetooth hc-05 module in master or slave.  (TODO)   
Silab SI5351 via i2c to generate reference frequency for general pourpose.  
24c64 to store nonvolatile data.
mcp23017 to manage 2 illuminated pushbutton with rgb led, and an alarm buzzer. (partial)  

About the bluepill, i had to replace the two capacitors on the 32768Hz crystal because of the clock instability and fast discharging of a CR2032 battery; to original value is unknown: i had same 13pf capacitors but i think that 12pf or 15 pf are fine too; the clock need a calibration to slow down, the default value in the code it's the best for my hardware.

last note: the bus abp1 is overclocked!!!, the datasheet sugget a max clock of 18mhz for the spi bus but the code set it to 36mhz; the signal on the oscilloscope it's not exactly square but all spi devices work anyway without any issue and the speed of the display is doubled when using dma transfers.
