
GPIO27 (GPIO27, PIN13) => IRQ - IRQ for frame timing - !3S
SD17   (GPI025, PIN22) => DREQ_ACK - UNUSED
SD16   (GPI024, PIN18) => DREQ = /(/IORQ & /CAS) - triggers the SMI reads to DMA - !3S
SD15   (GPI023, PIN16) => D0 - 3S
SD14   (GPI022, PIN15) => D1 - 3S
SD13   (GPI021, PIN40) => D2 - 3S
SD12   (GPIO20, PIN38) => D3 - 3S
SD11   (GPIO19, PIN35) => D4 - 3S
SD10   (GPIO18, PIN12) => D5 - 3S
SD9    (GPIO17, PIN11) => D6 - 3S
SD8    (GPI016, PIN36) => D7 - 3S
SD7    (RXD0,   PIN10) <= UART for debug
SD6    (TXD0,   PIN8 ) => UART for debug 
SD5    (PWM1,   PIN33) == UNUSED
SD4    (PWM0,   PIN32) == UNUSED
SD3    (SCLK,   PIN23) => 
SD2    (MOSI,   PIN19) => /IORQ - When low, indicates an IO read or write - 3S
SD1    (MISO,   PIN21) => CAS - Used to detect writes to video memory - 3S
SD0    (CE0,    PIN24) => /WR - When low, indicates a write - 3S
SOE/SE (GPIO6,  PIN31) <= SMI CLK - Clock for SMI, Switch off except when debugging

OLD!!
SD3    (SCLK,   PIN23) => CAS - Used to detect writes to video memory - 3S
SD2    (MOSI,   PIN19) => A0 - Address bit 0 (/IOREQ = /IORQ | A0) ? is A0 inverted? - 3S
SD1    (MISO,   PIN21) => /WR - When low, indicates a write - 3S
SD0    (CE0,    PIN24) => /IORQ - When low, indicates an IO read or write - 3S
SOE/SE (GPIO6,  PIN31) <= SMI CLK - Clock for SMI, Switch off except when debugging

NOTES
- The /OE for the buffers should be generated before the DREQ for the SMI, so that the data is
available when the SMI is triggered. ACTUALLY, the timing is good the other way around....
The data will be valid at the 3rd sample, which is the sample before a high on /CAS which
indicates the end of the read.
- If there are 4 samples between the /CAS highs, then it is a video read. If there are more
then it is NOT a video read. There should never be less.
- There are 256*192 / 8 = 6144 bytes of pixels and 32*24 = 768 bytes of attributes. 
giving a total of 6912 bytes (0x1B00) of video memory.
- The Speccy reads the video memory in 2 byte chunks. 1 byte is the 8 pixesl, and the other
byte is the attribute bits for those pixel. This should result in 6144 * 2 = 12288 reads (0x3000)
- ULA CAS pluse is [~204ns (gap ~60ns) 226ns] x2 (with 86ns gap)
- Shortest Z80 read is ~296ns
- Therefore we only have a window of ~70ns difference to differentiate the 2 :/

