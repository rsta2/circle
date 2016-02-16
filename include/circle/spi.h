typedef unsigned int uint32_t;
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned long uint64_t;

#ifdef __cplusplus
extern "C" {
#endif

    /*! \defgroup init Library initialisation and management
      These functions allow you to intialise and control the bcm2835 library
      @{
    */

    /*! Initialise the library by opening /dev/mem and getting pointers to the 
      internal memory for BCM 2835 device registers. You must call this (successfully)
      before calling any other 
      functions in this library (except bcm2835_set_debug). 
      If bcm2835_init() fails by returning 0, 
      calling any other function may result in crashes or other failures.
      Prints messages to stderr in case of errors.
      \return 1 if successful else 0
    */
    extern int bcm2835_init(void);

    /*! Close the library, deallocating any allocated memory and closing /dev/mem
      \return 1 if successful else 0
    */
    extern int bcm2835_close(void);

    /*! Sets the debug level of the library.
      A value of 1 prevents mapping to /dev/mem, and makes the library print out
      what it would do, rather than accessing the GPIO registers.
      A value of 0, the default, causes normal operation.
      Call this before calling bcm2835_init();
      \param[in] debug The new debug level. 1 means debug
    */
    extern void  bcm2835_set_debug(uint8_t debug);

    /*! Returns the version number of the library, same as BCM2835_VERSION
       \return the current library version number
    */
    extern unsigned int bcm2835_version(void);

    /*! @} */

    /*! \defgroup lowlevel Low level register access
      These functions provide low level register access, and should not generally
      need to be used 
       
      @{
    */

    /*! Gets the base of a register
      \param[in] regbase You can use one of the common values BCM2835_REGBASE_*
      in \ref bcm2835RegisterBase
      \return the register base
      \sa Physical Addresses
    */
    extern uint32_t* bcm2835_regbase(uint8_t regbase);

    /*! Reads 32 bit value from a peripheral address WITH a memory barrier before and after each read.
      This is safe, but slow.  The MB before protects this read from any in-flight reads that didn't
      use a MB.  The MB after protects subsequent reads from another peripheral.

      \param[in] paddr Physical address to read from. See BCM2835_GPIO_BASE etc.
      \return the value read from the 32 bit register
      \sa Physical Addresses
    */
    extern uint32_t bcm2835_peri_read(volatile uint32_t* paddr);

    /*! Reads 32 bit value from a peripheral address WITHOUT the read barriers
      You should only use this when:
      o your code has previously called bcm2835_peri_read() for a register
      within the same peripheral, and no read or write to another peripheral has occurred since.
      o your code has called bcm2835_memory_barrier() since the last access to ANOTHER peripheral.

      \param[in] paddr Physical address to read from. See BCM2835_GPIO_BASE etc.
      \return the value read from the 32 bit register
      \sa Physical Addresses
    */
    extern uint32_t bcm2835_peri_read_nb(volatile uint32_t* paddr);


    /*! Writes 32 bit value from a peripheral address WITH a memory barrier before and after each write
      This is safe, but slow.  The MB before ensures that any in-flight write to another peripheral
      completes before this write is issued.  The MB after ensures that subsequent reads and writes
      to another peripheral will see the effect of this write.

      This is a tricky optimization; if you aren't sure, use the barrier version.

      \param[in] paddr Physical address to read from. See BCM2835_GPIO_BASE etc.
      \param[in] value The 32 bit value to write
      \sa Physical Addresses
    */
    extern void bcm2835_peri_write(volatile uint32_t* paddr, uint32_t value);

    /*! Writes 32 bit value from a peripheral address without the write barrier
      You should only use this when:
      o your code has previously called bcm2835_peri_write() for a register
      within the same peripheral, and no other peripheral access has occurred since.
      o your code has called bcm2835_memory_barrier() since the last access to ANOTHER peripheral.

      This is a tricky optimization; if you aren't sure, use the barrier version.

      \param[in] paddr Physical address to read from. See BCM2835_GPIO_BASE etc.
      \param[in] value The 32 bit value to write
      \sa Physical Addresses
    */
    extern void bcm2835_peri_write_nb(volatile uint32_t* paddr, uint32_t value);

    /*! Alters a number of bits in a 32 peripheral regsiter.
      It reads the current valu and then alters the bits defines as 1 in mask, 
      according to the bit value in value. 
      All other bits that are 0 in the mask are unaffected.
      Use this to alter a subset of the bits in a register.
      Memory barriers are used.  Note that this is not atomic; an interrupt
      routine can cause unexpected results.
      \param[in] paddr Physical address to read from. See BCM2835_GPIO_BASE etc.
      \param[in] value The 32 bit value to write, masked in by mask.
      \param[in] mask Bitmask that defines the bits that will be altered in the register.
      \sa Physical Addresses
    */
    extern void bcm2835_peri_set_bits(volatile uint32_t* paddr, uint32_t value, uint32_t mask);
    /*! @}    end of lowlevel */

    /*! \defgroup gpio GPIO register access
      These functions allow you to control the GPIO interface. You can set the 
      function of each GPIO pin, read the input state and set the output state.
      @{
    */

    /*! Sets the Function Select register for the given pin, which configures
      the pin as Input, Output or one of the 6 alternate functions.
      \param[in] pin GPIO number, or one of RPI_GPIO_P1_* from \ref RPiGPIOPin.
      \param[in] mode Mode to set the pin to, one of BCM2835_GPIO_FSEL_* from \ref bcm2835FunctionSelect
    */
    extern void bcm2835_gpio_fsel(uint8_t pin, uint8_t mode);

    /*! Sets the specified pin output to 
      HIGH.
      \param[in] pin GPIO number, or one of RPI_GPIO_P1_* from \ref RPiGPIOPin.
      \sa bcm2835_gpio_write()
    */
    extern void bcm2835_gpio_set(uint8_t pin);

    /*! Sets the specified pin output to 
      LOW.
      \param[in] pin GPIO number, or one of RPI_GPIO_P1_* from \ref RPiGPIOPin.
      \sa bcm2835_gpio_write()
    */
    extern void bcm2835_gpio_clr(uint8_t pin);

    /*! Sets any of the first 32 GPIO output pins specified in the mask to 
      HIGH.
      \param[in] mask Mask of pins to affect. Use eg: (1 << RPI_GPIO_P1_03) | (1 << RPI_GPIO_P1_05)
      \sa bcm2835_gpio_write_multi()
    */
    extern void bcm2835_gpio_set_multi(uint32_t mask);

    /*! Sets any of the first 32 GPIO output pins specified in the mask to 
      LOW.
      \param[in] mask Mask of pins to affect. Use eg: (1 << RPI_GPIO_P1_03) | (1 << RPI_GPIO_P1_05)
      \sa bcm2835_gpio_write_multi()
    */
    extern void bcm2835_gpio_clr_multi(uint32_t mask);

    /*! Reads the current level on the specified 
      pin and returns either HIGH or LOW. Works whether or not the pin
      is an input or an output.
      \param[in] pin GPIO number, or one of RPI_GPIO_P1_* from \ref RPiGPIOPin.
      \return the current level  either HIGH or LOW
    */
    extern uint8_t bcm2835_gpio_lev(uint8_t pin);

    /*! Event Detect Status.
      Tests whether the specified pin has detected a level or edge
      as requested by bcm2835_gpio_ren(), bcm2835_gpio_fen(), bcm2835_gpio_hen(), 
      bcm2835_gpio_len(), bcm2835_gpio_aren(), bcm2835_gpio_afen().
      Clear the flag for a given pin by calling bcm2835_gpio_set_eds(pin);
      \param[in] pin GPIO number, or one of RPI_GPIO_P1_* from \ref RPiGPIOPin.
      \return HIGH if the event detect status for the given pin is true.
    */
    extern uint8_t bcm2835_gpio_eds(uint8_t pin);

    /*! Same as bcm2835_gpio_eds() but checks if any of the pins specified in
      the mask have detected a level or edge.
      \param[in] mask Mask of pins to check. Use eg: (1 << RPI_GPIO_P1_03) | (1 << RPI_GPIO_P1_05)
      \return Mask of pins HIGH if the event detect status for the given pin is true.
    */
    extern uint32_t bcm2835_gpio_eds_multi(uint32_t mask);

    /*! Sets the Event Detect Status register for a given pin to 1, 
      which has the effect of clearing the flag. Use this afer seeing
      an Event Detect Status on the pin.
      \param[in] pin GPIO number, or one of RPI_GPIO_P1_* from \ref RPiGPIOPin.
    */
    extern void bcm2835_gpio_set_eds(uint8_t pin);

    /*! Same as bcm2835_gpio_set_eds() but clears the flag for any pin which
      is set in the mask.
      \param[in] mask Mask of pins to clear. Use eg: (1 << RPI_GPIO_P1_03) | (1 << RPI_GPIO_P1_05)
    */
    extern void bcm2835_gpio_set_eds_multi(uint32_t mask);
    
    /*! Enable Rising Edge Detect Enable for the specified pin.
      When a rising edge is detected, sets the appropriate pin in Event Detect Status.
      The GPRENn registers use
      synchronous edge detection. This means the input signal is sampled using the
      system clock and then it is looking for a ?011? pattern on the sampled signal. This
      has the effect of suppressing glitches.
      \param[in] pin GPIO number, or one of RPI_GPIO_P1_* from \ref RPiGPIOPin.
    */
    extern void bcm2835_gpio_ren(uint8_t pin);

    /*! Disable Rising Edge Detect Enable for the specified pin.
      \param[in] pin GPIO number, or one of RPI_GPIO_P1_* from \ref RPiGPIOPin.
    */
    extern void bcm2835_gpio_clr_ren(uint8_t pin);

    /*! Enable Falling Edge Detect Enable for the specified pin.
      When a falling edge is detected, sets the appropriate pin in Event Detect Status.
      The GPRENn registers use
      synchronous edge detection. This means the input signal is sampled using the
      system clock and then it is looking for a ?100? pattern on the sampled signal. This
      has the effect of suppressing glitches.
      \param[in] pin GPIO number, or one of RPI_GPIO_P1_* from \ref RPiGPIOPin.
    */
    extern void bcm2835_gpio_fen(uint8_t pin);

    /*! Disable Falling Edge Detect Enable for the specified pin.
      \param[in] pin GPIO number, or one of RPI_GPIO_P1_* from \ref RPiGPIOPin.
    */
    extern void bcm2835_gpio_clr_fen(uint8_t pin);

    /*! Enable High Detect Enable for the specified pin.
      When a HIGH level is detected on the pin, sets the appropriate pin in Event Detect Status.
      \param[in] pin GPIO number, or one of RPI_GPIO_P1_* from \ref RPiGPIOPin.
    */
    extern void bcm2835_gpio_hen(uint8_t pin);

    /*! Disable High Detect Enable for the specified pin.
      \param[in] pin GPIO number, or one of RPI_GPIO_P1_* from \ref RPiGPIOPin.
    */
    extern void bcm2835_gpio_clr_hen(uint8_t pin);

    /*! Enable Low Detect Enable for the specified pin.
      When a LOW level is detected on the pin, sets the appropriate pin in Event Detect Status.
      \param[in] pin GPIO number, or one of RPI_GPIO_P1_* from \ref RPiGPIOPin.
    */
    extern void bcm2835_gpio_len(uint8_t pin);

    /*! Disable Low Detect Enable for the specified pin.
      \param[in] pin GPIO number, or one of RPI_GPIO_P1_* from \ref RPiGPIOPin.
    */
    extern void bcm2835_gpio_clr_len(uint8_t pin);

    /*! Enable Asynchronous Rising Edge Detect Enable for the specified pin.
      When a rising edge is detected, sets the appropriate pin in Event Detect Status.
      Asynchronous means the incoming signal is not sampled by the system clock. As such
      rising edges of very short duration can be detected.
      \param[in] pin GPIO number, or one of RPI_GPIO_P1_* from \ref RPiGPIOPin.
    */
    extern void bcm2835_gpio_aren(uint8_t pin);

    /*! Disable Asynchronous Rising Edge Detect Enable for the specified pin.
      \param[in] pin GPIO number, or one of RPI_GPIO_P1_* from \ref RPiGPIOPin.
    */
    extern void bcm2835_gpio_clr_aren(uint8_t pin);

    /*! Enable Asynchronous Falling Edge Detect Enable for the specified pin.
      When a falling edge is detected, sets the appropriate pin in Event Detect Status.
      Asynchronous means the incoming signal is not sampled by the system clock. As such
      falling edges of very short duration can be detected.
      \param[in] pin GPIO number, or one of RPI_GPIO_P1_* from \ref RPiGPIOPin.
    */
    extern void bcm2835_gpio_afen(uint8_t pin);

    /*! Disable Asynchronous Falling Edge Detect Enable for the specified pin.
      \param[in] pin GPIO number, or one of RPI_GPIO_P1_* from \ref RPiGPIOPin.
    */
    extern void bcm2835_gpio_clr_afen(uint8_t pin);

    /*! Sets the Pull-up/down register for the given pin. This is
      used with bcm2835_gpio_pudclk() to set the  Pull-up/down resistor for the given pin.
      However, it is usually more convenient to use bcm2835_gpio_set_pud().
      \param[in] pud The desired Pull-up/down mode. One of BCM2835_GPIO_PUD_* from bcm2835PUDControl
      \sa bcm2835_gpio_set_pud()
    */
    extern void bcm2835_gpio_pud(uint8_t pud);

    /*! Clocks the Pull-up/down value set earlier by bcm2835_gpio_pud() into the pin.
      \param[in] pin GPIO number, or one of RPI_GPIO_P1_* from \ref RPiGPIOPin.
      \param[in] on HIGH to clock the value from bcm2835_gpio_pud() into the pin. 
      LOW to remove the clock. 
      \sa bcm2835_gpio_set_pud()
    */
    extern void bcm2835_gpio_pudclk(uint8_t pin, uint8_t on);

    /*! Reads and returns the Pad Control for the given GPIO group.
      \param[in] group The GPIO pad group number, one of BCM2835_PAD_GROUP_GPIO_*
      \return Mask of bits from BCM2835_PAD_* from \ref bcm2835PadGroup
    */
    extern uint32_t bcm2835_gpio_pad(uint8_t group);

    /*! Sets the Pad Control for the given GPIO group.
      \param[in] group The GPIO pad group number, one of BCM2835_PAD_GROUP_GPIO_*
      \param[in] control Mask of bits from BCM2835_PAD_* from \ref bcm2835PadGroup. Note 
      that it is not necessary to include BCM2835_PAD_PASSWRD in the mask as this
      is automatically included.
    */
    extern void bcm2835_gpio_set_pad(uint8_t group, uint32_t control);

    /*! Delays for the specified number of milliseconds.
      Uses nanosleep(), and therefore does not use CPU until the time is up.
      However, you are at the mercy of nanosleep(). From the manual for nanosleep():
      If the interval specified in req is not an exact multiple of the granularity  
      underlying  clock  (see  time(7)),  then the interval will be
      rounded up to the next multiple. Furthermore, after the sleep completes, 
      there may still be a delay before the CPU becomes free to once
      again execute the calling thread.
      \param[in] millis Delay in milliseconds
    */
    extern void bcm2835_delay (unsigned int millis);

    /*! Delays for the specified number of microseconds.
      Uses a combination of nanosleep() and a busy wait loop on the BCM2835 system timers,
      However, you are at the mercy of nanosleep(). From the manual for nanosleep():
      If the interval specified in req is not an exact multiple of the granularity  
      underlying  clock  (see  time(7)),  then the interval will be
      rounded up to the next multiple. Furthermore, after the sleep completes, 
      there may still be a delay before the CPU becomes free to once
      again execute the calling thread.
      For times less than about 450 microseconds, uses a busy wait on the System Timer.
      It is reported that a delay of 0 microseconds on RaspberryPi will in fact
      result in a delay of about 80 microseconds. Your mileage may vary.
      \param[in] micros Delay in microseconds
    */
    extern void bcm2835_delayMicroseconds (uint64_t micros);

    /*! Sets the output state of the specified pin
      \param[in] pin GPIO number, or one of RPI_GPIO_P1_* from \ref RPiGPIOPin.
      \param[in] on HIGH sets the output to HIGH and LOW to LOW.
    */
    extern void bcm2835_gpio_write(uint8_t pin, uint8_t on);

    /*! Sets any of the first 32 GPIO output pins specified in the mask to the state given by on
      \param[in] mask Mask of pins to affect. Use eg: (1 << RPI_GPIO_P1_03) | (1 << RPI_GPIO_P1_05)
      \param[in] on HIGH sets the output to HIGH and LOW to LOW.
    */
    extern void bcm2835_gpio_write_multi(uint32_t mask, uint8_t on);

    /*! Sets the first 32 GPIO output pins specified in the mask to the value given by value
      \param[in] value values required for each bit masked in by mask, eg: (1 << RPI_GPIO_P1_03) | (1 << RPI_GPIO_P1_05)
      \param[in] mask Mask of pins to affect. Use eg: (1 << RPI_GPIO_P1_03) | (1 << RPI_GPIO_P1_05)
    */
    extern void bcm2835_gpio_write_mask(uint32_t value, uint32_t mask);

    /*! Sets the Pull-up/down mode for the specified pin. This is more convenient than
      clocking the mode in with bcm2835_gpio_pud() and bcm2835_gpio_pudclk().
      \param[in] pin GPIO number, or one of RPI_GPIO_P1_* from \ref RPiGPIOPin.
      \param[in] pud The desired Pull-up/down mode. One of BCM2835_GPIO_PUD_* from bcm2835PUDControl
    */
    extern void bcm2835_gpio_set_pud(uint8_t pin, uint8_t pud);

    /*! @}  */

    /*! \defgroup spi SPI access
      These functions let you use SPI0 (Serial Peripheral Interface) to 
      interface with an external SPI device.
      @{
    */

    /*! Start SPI operations.
      Forces RPi SPI0 pins P1-19 (MOSI), P1-21 (MISO), P1-23 (CLK), P1-24 (CE0) and P1-26 (CE1)
      to alternate function ALT0, which enables those pins for SPI interface.
      You should call bcm2835_spi_end() when all SPI funcitons are complete to return the pins to 
      their default functions
      \sa  bcm2835_spi_end()
    */
    extern void bcm2835_spi_begin(void);

    /*! End SPI operations.
      SPI0 pins P1-19 (MOSI), P1-21 (MISO), P1-23 (CLK), P1-24 (CE0) and P1-26 (CE1)
      are returned to their default INPUT behaviour.
    */
    extern void bcm2835_spi_end(void);

    /*! Sets the SPI bit order
      NOTE: has no effect. Not supported by SPI0.
      Defaults to 
      \param[in] order The desired bit order, one of BCM2835_SPI_BIT_ORDER_*, 
      see \ref bcm2835SPIBitOrder
    */
    extern void bcm2835_spi_setBitOrder(uint8_t order);

    /*! Sets the SPI clock divider and therefore the 
      SPI clock speed. 
      \param[in] divider The desired SPI clock divider, one of BCM2835_SPI_CLOCK_DIVIDER_*, 
      see \ref bcm2835SPIClockDivider
    */
    extern void bcm2835_spi_setClockDivider(uint16_t divider);

    /*! Sets the SPI data mode
      Sets the clock polariy and phase
      \param[in] mode The desired data mode, one of BCM2835_SPI_MODE*, 
      see \ref bcm2835SPIMode
    */
    extern void bcm2835_spi_setDataMode(uint8_t mode);

    /*! Sets the chip select pin(s)
      When an bcm2835_spi_transfer() is made, the selected pin(s) will be asserted during the
      transfer.
      \param[in] cs Specifies the CS pins(s) that are used to activate the desired slave. 
      One of BCM2835_SPI_CS*, see \ref bcm2835SPIChipSelect
    */
    extern void bcm2835_spi_chipSelect(uint8_t cs);

    /*! Sets the chip select pin polarity for a given pin
      When an bcm2835_spi_transfer() occurs, the currently selected chip select pin(s) 
      will be asserted to the 
      value given by active. When transfers are not happening, the chip select pin(s) 
      return to the complement (inactive) value.
      \param[in] cs The chip select pin to affect
      \param[in] active Whether the chip select pin is to be active HIGH
    */
    extern void bcm2835_spi_setChipSelectPolarity(uint8_t cs, uint8_t active);

    /*! Transfers one byte to and from the currently selected SPI slave.
      Asserts the currently selected CS pins (as previously set by bcm2835_spi_chipSelect) 
      during the transfer.
      Clocks the 8 bit value out on MOSI, and simultaneously clocks in data from MISO. 
      Returns the read data byte from the slave.
      Uses polled transfer as per section 10.6.1 of the BCM 2835 ARM Peripherls manual
      \param[in] value The 8 bit data byte to write to MOSI
      \return The 8 bit byte simultaneously read from  MISO
      \sa bcm2835_spi_transfern()
    */
    extern uint8_t bcm2835_spi_transfer(uint8_t value);
    
    /*! Transfers any number of bytes to and from the currently selected SPI slave.
      Asserts the currently selected CS pins (as previously set by bcm2835_spi_chipSelect) 
      during the transfer.
      Clocks the len 8 bit bytes out on MOSI, and simultaneously clocks in data from MISO. 
      The data read read from the slave is placed into rbuf. rbuf must be at least len bytes long
      Uses polled transfer as per section 10.6.1 of the BCM 2835 ARM Peripherls manual
      \param[in] tbuf Buffer of bytes to send. 
      \param[out] rbuf Received bytes will by put in this buffer
      \param[in] len Number of bytes in the tbuf buffer, and the number of bytes to send/received
      \sa bcm2835_spi_transfer()
    */
    extern void bcm2835_spi_transfernb(char* tbuf, char* rbuf, uint32_t len);

    /*! Transfers any number of bytes to and from the currently selected SPI slave
      using bcm2835_spi_transfernb.
      The returned data from the slave replaces the transmitted data in the buffer.
      \param[in,out] buf Buffer of bytes to send. Received bytes will replace the contents
      \param[in] len Number of bytes int eh buffer, and the number of bytes to send/received
      \sa bcm2835_spi_transfer()
    */
    extern void bcm2835_spi_transfern(char* buf, uint32_t len);

    /*! Transfers any number of bytes to the currently selected SPI slave.
      Asserts the currently selected CS pins (as previously set by bcm2835_spi_chipSelect)
      during the transfer.
      \param[in] buf Buffer of bytes to send.
      \param[in] len Number of bytes in the tbuf buffer, and the number of bytes to send
    */
    extern void bcm2835_spi_writenb(char* buf, uint32_t len);

    /*! @} */

    /*! \defgroup i2c I2C access
      These functions let you use I2C (The Broadcom Serial Control bus with the Philips
      I2C bus/interface version 2.1 January 2000.) to interface with an external I2C device.
      @{
    */

    /*! Start I2C operations.
      Forces RPi I2C pins P1-03 (SDA) and P1-05 (SCL)
      to alternate function ALT0, which enables those pins for I2C interface.
      You should call bcm2835_i2c_end() when all I2C functions are complete to return the pins to
      their default functions
      \sa  bcm2835_i2c_end()
    */
    extern void bcm2835_i2c_begin(void);

    /*! End I2C operations.
      I2C pins P1-03 (SDA) and P1-05 (SCL)
      are returned to their default INPUT behaviour.
    */
    extern void bcm2835_i2c_end(void);

    /*! Sets the I2C slave address.
      \param[in] addr The I2C slave address.
    */
    extern void bcm2835_i2c_setSlaveAddress(uint8_t addr);

    /*! Sets the I2C clock divider and therefore the I2C clock speed.
      \param[in] divider The desired I2C clock divider, one of BCM2835_I2C_CLOCK_DIVIDER_*,
      see \ref bcm2835I2CClockDivider
    */
    extern void bcm2835_i2c_setClockDivider(uint16_t divider);

    /*! Sets the I2C clock divider by converting the baudrate parameter to
      the equivalent I2C clock divider. ( see \sa bcm2835_i2c_setClockDivider)
      For the I2C standard 100khz you would set baudrate to 100000
      The use of baudrate corresponds to its use in the I2C kernel device
      driver. (Of course, bcm2835 has nothing to do with the kernel driver)
    */
    extern void bcm2835_i2c_set_baudrate(uint32_t baudrate);

    /*! Transfers any number of bytes to the currently selected I2C slave.
      (as previously set by \sa bcm2835_i2c_setSlaveAddress)
      \param[in] buf Buffer of bytes to send.
      \param[in] len Number of bytes in the buf buffer, and the number of bytes to send.
      \return reason see \ref bcm2835I2CReasonCodes
    */
    extern uint8_t bcm2835_i2c_write(const char * buf, uint32_t len);

    /*! Transfers any number of bytes from the currently selected I2C slave.
      (as previously set by \sa bcm2835_i2c_setSlaveAddress)
      \param[in] buf Buffer of bytes to receive.
      \param[in] len Number of bytes in the buf buffer, and the number of bytes to received.
      \return reason see \ref bcm2835I2CReasonCodes
    */
    extern uint8_t bcm2835_i2c_read(char* buf, uint32_t len);

    /*! Allows reading from I2C slaves that require a repeated start (without any prior stop)
      to read after the required slave register has been set. For example, the popular
      MPL3115A2 pressure and temperature sensor. Note that your device must support or
      require this mode. If your device does not require this mode then the standard
      combined:
      \sa bcm2835_i2c_write
      \sa bcm2835_i2c_read
      are a better choice.
      Will read from the slave previously set by \sa bcm2835_i2c_setSlaveAddress
      \param[in] regaddr Buffer containing the slave register you wish to read from.
      \param[in] buf Buffer of bytes to receive.
      \param[in] len Number of bytes in the buf buffer, and the number of bytes to received.
      \return reason see \ref bcm2835I2CReasonCodes
    */
    extern uint8_t bcm2835_i2c_read_register_rs(char* regaddr, char* buf, uint32_t len);

    /*! Allows sending an arbitrary number of bytes to I2C slaves before issuing a repeated
      start (with no prior stop) and reading a response.
      Necessary for devices that require such behavior, such as the MLX90620.
      Will write to and read from the slave previously set by \sa bcm2835_i2c_setSlaveAddress
      \param[in] cmds Buffer containing the bytes to send before the repeated start condition.
      \param[in] cmds_len Number of bytes to send from cmds buffer
      \param[in] buf Buffer of bytes to receive.
      \param[in] buf_len Number of bytes to receive in the buf buffer.
      \return reason see \ref bcm2835I2CReasonCodes
    */
    extern uint8_t bcm2835_i2c_write_read_rs(char* cmds, uint32_t cmds_len, char* buf, uint32_t buf_len);

    /*! @} */

    /*! \defgroup st System Timer access
      Allows access to and delays using the System Timer Counter.
      @{
    */

    /*! Read the System Timer Counter register.
      \return the value read from the System Timer Counter Lower 32 bits register
    */
    extern uint64_t bcm2835_st_read(void);

    /*! Delays for the specified number of microseconds with offset.
      \param[in] offset_micros Offset in microseconds
      \param[in] micros Delay in microseconds
    */
    extern void bcm2835_st_delay(uint64_t offset_micros, uint64_t micros);

    /*! @}  */

    /*! \defgroup pwm Pulse Width Modulation
      Allows control of 2 independent PWM channels. A limited subset of GPIO pins
      can be connected to one of these 2 channels, allowing PWM control of GPIO pins.
      You have to set the desired pin into a particular Alt Fun to PWM output. See the PWM
      documentation on the Main Page.
      @{
    */

    /*! Sets the PWM clock divisor, 
      to control the basic PWM pulse widths.
      \param[in] divisor Divides the basic 19.2MHz PWM clock. You can use one of the common
      values BCM2835_PWM_CLOCK_DIVIDER_* in \ref bcm2835PWMClockDivider
    */
    extern void bcm2835_pwm_set_clock(uint32_t divisor);
    
    /*! Sets the mode of the given PWM channel,
      allowing you to control the PWM mode and enable/disable that channel
      \param[in] channel The PWM channel. 0 or 1.
      \param[in] markspace Set true if you want Mark-Space mode. 0 for Balanced mode.
      \param[in] enabled Set true to enable this channel and produce PWM pulses.
    */
    extern void bcm2835_pwm_set_mode(uint8_t channel, uint8_t markspace, uint8_t enabled);

    /*! Sets the maximum range of the PWM output.
      The data value can vary between 0 and this range to control PWM output
      \param[in] channel The PWM channel. 0 or 1.
      \param[in] range The maximum value permitted for DATA.
    */
    extern void bcm2835_pwm_set_range(uint8_t channel, uint32_t range);
    
    /*! Sets the PWM pulse ratio to emit to DATA/RANGE, 
      where RANGE is set by bcm2835_pwm_set_range().
      \param[in] channel The PWM channel. 0 or 1.
      \param[in] data Controls the PWM output ratio as a fraction of the range. 
      Can vary from 0 to RANGE.
    */
    extern void bcm2835_pwm_set_data(uint8_t channel, uint32_t data);

    /*! @}  */
#ifdef __cplusplus
}
#endif