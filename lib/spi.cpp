#include <circle/bcm2835.h>
#include <circle/spi.h>
#include <circle/synchronize.h>

#define BCM2835_PERI_BASE ARM_IO_BASE
#define BCM2835_PERI_SIZE               0x01000000
#define MAP_FAILED 0

#define BCM2835_ST_BASE			0x3000
/*! Base Address of the Pads registers */
#define BCM2835_GPIO_PADS       0x100000
/*! Base Address of the Clock/timer registers */
#define BCM2835_CLOCK_BASE      0x101000
/*! Base Address of the GPIO registers */
#define BCM2835_GPIO_BASE       0x200000
/*! Base Address of the SPI0 registers */
#define BCM2835_SPI0_BASE       0x204000
/*! Base Address of the BSC0 registers */
#define BCM2835_BSC0_BASE 		0x205000
/*! Base Address of the PWM registers */
#define BCM2835_GPIO_PWM        0x20C000
/*! Base Address of the BSC1 registers */
#define BCM2835_BSC1_BASE		0x804000

#define ARM_GPIO_BASE		(ARM_IO_BASE + 0x200000)

#define ARM_GPIO_GPFSEL0	(ARM_GPIO_BASE + 0x00)
#define ARM_GPIO_GPFSEL1	(ARM_GPIO_BASE + 0x04)
#define ARM_GPIO_GPFSEL4	(ARM_GPIO_BASE + 0x10)
#define ARM_GPIO_GPSET0		(ARM_GPIO_BASE + 0x1C)
#define ARM_GPIO_GPCLR0		(ARM_GPIO_BASE + 0x28)
#define ARM_GPIO_GPLEV0		(ARM_GPIO_BASE + 0x34)
#define ARM_GPIO_GPEDS0		(ARM_GPIO_BASE + 0x40)
#define ARM_GPIO_GPREN0		(ARM_GPIO_BASE + 0x4C)
#define ARM_GPIO_GPFEN0		(ARM_GPIO_BASE + 0x58)
#define ARM_GPIO_GPHEN0		(ARM_GPIO_BASE + 0x64)
#define ARM_GPIO_GPLEN0		(ARM_GPIO_BASE + 0x70)
#define ARM_GPIO_GPAREN0	(ARM_GPIO_BASE + 0x7C)
#define ARM_GPIO_GPAFEN0	(ARM_GPIO_BASE + 0x88)
#define ARM_GPIO_GPPUD		(ARM_GPIO_BASE + 0x94)
#define ARM_GPIO_GPPUDCLK0	(ARM_GPIO_BASE + 0x98)

/* Defines for GPIO
   The BCM2835 has 54 GPIO pins.
   BCM2835 data sheet, Page 90 onwards.
*/
#define BCM2835_GPFSEL0                      0x0000 /*!< GPIO Function Select 0 */
#define BCM2835_GPFSEL1                      0x0004 /*!< GPIO Function Select 1 */
#define BCM2835_GPFSEL2                      0x0008 /*!< GPIO Function Select 2 */
#define BCM2835_GPFSEL3                      0x000c /*!< GPIO Function Select 3 */
#define BCM2835_GPFSEL4                      0x0010 /*!< GPIO Function Select 4 */
#define BCM2835_GPFSEL5                      0x0014 /*!< GPIO Function Select 5 */
#define BCM2835_GPSET0                       0x001c /*!< GPIO Pin Output Set 0 */
#define BCM2835_GPSET1                       0x0020 /*!< GPIO Pin Output Set 1 */
#define BCM2835_GPCLR0                       0x0028 /*!< GPIO Pin Output Clear 0 */
#define BCM2835_GPCLR1                       0x002c /*!< GPIO Pin Output Clear 1 */
#define BCM2835_GPLEV0                       0x0034 /*!< GPIO Pin Level 0 */
#define BCM2835_GPLEV1                       0x0038 /*!< GPIO Pin Level 1 */
#define BCM2835_GPEDS0                       0x0040 /*!< GPIO Pin Event Detect Status 0 */
#define BCM2835_GPEDS1                       0x0044 /*!< GPIO Pin Event Detect Status 1 */
#define BCM2835_GPREN0                       0x004c /*!< GPIO Pin Rising Edge Detect Enable 0 */
#define BCM2835_GPREN1                       0x0050 /*!< GPIO Pin Rising Edge Detect Enable 1 */
#define BCM2835_GPFEN0                       0x0058 /*!< GPIO Pin Falling Edge Detect Enable 0 */
#define BCM2835_GPFEN1                       0x005c /*!< GPIO Pin Falling Edge Detect Enable 1 */
#define BCM2835_GPHEN0                       0x0064 /*!< GPIO Pin High Detect Enable 0 */
#define BCM2835_GPHEN1                       0x0068 /*!< GPIO Pin High Detect Enable 1 */
#define BCM2835_GPLEN0                       0x0070 /*!< GPIO Pin Low Detect Enable 0 */
#define BCM2835_GPLEN1                       0x0074 /*!< GPIO Pin Low Detect Enable 1 */
#define BCM2835_GPAREN0                      0x007c /*!< GPIO Pin Async. Rising Edge Detect 0 */
#define BCM2835_GPAREN1                      0x0080 /*!< GPIO Pin Async. Rising Edge Detect 1 */
#define BCM2835_GPAFEN0                      0x0088 /*!< GPIO Pin Async. Falling Edge Detect 0 */
#define BCM2835_GPAFEN1                      0x008c /*!< GPIO Pin Async. Falling Edge Detect 1 */
#define BCM2835_GPPUD                        0x0094 /*!< GPIO Pin Pull-up/down Enable */
#define BCM2835_GPPUDCLK0                    0x0098 /*!< GPIO Pin Pull-up/down Enable Clock 0 */
#define BCM2835_GPPUDCLK1                    0x009c /*!< GPIO Pin Pull-up/down Enable Clock 1 */


typedef enum
{
    BCM2835_GPIO_FSEL_INPT  = 0x00,   /*!< Input 0b000 */
    BCM2835_GPIO_FSEL_OUTP  = 0x01,   /*!< Output 0b001 */
    BCM2835_GPIO_FSEL_ALT0  = 0x04,   /*!< Alternate function 0 0b100 */
    BCM2835_GPIO_FSEL_ALT1  = 0x05,   /*!< Alternate function 1 0b101 */
    BCM2835_GPIO_FSEL_ALT2  = 0x06,   /*!< Alternate function 2 0b110, */
    BCM2835_GPIO_FSEL_ALT3  = 0x07,   /*!< Alternate function 3 0b111 */
    BCM2835_GPIO_FSEL_ALT4  = 0x03,   /*!< Alternate function 4 0b011 */
    BCM2835_GPIO_FSEL_ALT5  = 0x02,   /*!< Alternate function 5 0b010 */
    BCM2835_GPIO_FSEL_MASK  = 0x07    /*!< Function select bits mask 0b111 */
} bcm2835FunctionSelect;

typedef enum
{
    RPI_GPIO_P1_03        =  0,  /*!< Version 1, Pin P1-03 */
    RPI_GPIO_P1_05        =  1,  /*!< Version 1, Pin P1-05 */
    RPI_GPIO_P1_07        =  4,  /*!< Version 1, Pin P1-07 */
    RPI_GPIO_P1_08        = 14,  /*!< Version 1, Pin P1-08, defaults to alt function 0 UART0_TXD */
    RPI_GPIO_P1_10        = 15,  /*!< Version 1, Pin P1-10, defaults to alt function 0 UART0_RXD */
    RPI_GPIO_P1_11        = 17,  /*!< Version 1, Pin P1-11 */
    RPI_GPIO_P1_12        = 18,  /*!< Version 1, Pin P1-12, can be PWM channel 0 in ALT FUN 5 */
    RPI_GPIO_P1_13        = 21,  /*!< Version 1, Pin P1-13 */
    RPI_GPIO_P1_15        = 22,  /*!< Version 1, Pin P1-15 */
    RPI_GPIO_P1_16        = 23,  /*!< Version 1, Pin P1-16 */
    RPI_GPIO_P1_18        = 24,  /*!< Version 1, Pin P1-18 */
    RPI_GPIO_P1_19        = 10,  /*!< Version 1, Pin P1-19, MOSI when SPI0 in use */
    RPI_GPIO_P1_21        =  9,  /*!< Version 1, Pin P1-21, MISO when SPI0 in use */
    RPI_GPIO_P1_22        = 25,  /*!< Version 1, Pin P1-22 */
    RPI_GPIO_P1_23        = 11,  /*!< Version 1, Pin P1-23, CLK when SPI0 in use */
    RPI_GPIO_P1_24        =  8,  /*!< Version 1, Pin P1-24, CE0 when SPI0 in use */
    RPI_GPIO_P1_26        =  7,  /*!< Version 1, Pin P1-26, CE1 when SPI0 in use */

    /* RPi Version 2 */
    RPI_V2_GPIO_P1_03     =  2,  /*!< Version 2, Pin P1-03 */
    RPI_V2_GPIO_P1_05     =  3,  /*!< Version 2, Pin P1-05 */
    RPI_V2_GPIO_P1_07     =  4,  /*!< Version 2, Pin P1-07 */
    RPI_V2_GPIO_P1_08     = 14,  /*!< Version 2, Pin P1-08, defaults to alt function 0 UART0_TXD */
    RPI_V2_GPIO_P1_10     = 15,  /*!< Version 2, Pin P1-10, defaults to alt function 0 UART0_RXD */
    RPI_V2_GPIO_P1_11     = 17,  /*!< Version 2, Pin P1-11 */
    RPI_V2_GPIO_P1_12     = 18,  /*!< Version 2, Pin P1-12, can be PWM channel 0 in ALT FUN 5 */
    RPI_V2_GPIO_P1_13     = 27,  /*!< Version 2, Pin P1-13 */
    RPI_V2_GPIO_P1_15     = 22,  /*!< Version 2, Pin P1-15 */
    RPI_V2_GPIO_P1_16     = 23,  /*!< Version 2, Pin P1-16 */
    RPI_V2_GPIO_P1_18     = 24,  /*!< Version 2, Pin P1-18 */
    RPI_V2_GPIO_P1_19     = 10,  /*!< Version 2, Pin P1-19, MOSI when SPI0 in use */
    RPI_V2_GPIO_P1_21     =  9,  /*!< Version 2, Pin P1-21, MISO when SPI0 in use */
    RPI_V2_GPIO_P1_22     = 25,  /*!< Version 2, Pin P1-22 */
    RPI_V2_GPIO_P1_23     = 11,  /*!< Version 2, Pin P1-23, CLK when SPI0 in use */
    RPI_V2_GPIO_P1_24     =  8,  /*!< Version 2, Pin P1-24, CE0 when SPI0 in use */
    RPI_V2_GPIO_P1_26     =  7,  /*!< Version 2, Pin P1-26, CE1 when SPI0 in use */
    RPI_V2_GPIO_P1_29     =  5,  /*!< Version 2, Pin P1-29 */
    RPI_V2_GPIO_P1_31     =  6,  /*!< Version 2, Pin P1-31 */
    RPI_V2_GPIO_P1_32     = 12,  /*!< Version 2, Pin P1-32 */
    RPI_V2_GPIO_P1_33     = 13,  /*!< Version 2, Pin P1-33 */
    RPI_V2_GPIO_P1_35     = 19,  /*!< Version 2, Pin P1-35 */
    RPI_V2_GPIO_P1_36     = 16,  /*!< Version 2, Pin P1-36 */
    RPI_V2_GPIO_P1_37     = 26,  /*!< Version 2, Pin P1-37 */
    RPI_V2_GPIO_P1_38     = 20,  /*!< Version 2, Pin P1-38 */
    RPI_V2_GPIO_P1_40     = 21,  /*!< Version 2, Pin P1-40 */

    /* RPi Version 2, new plug P5 */
    RPI_V2_GPIO_P5_03     = 28,  /*!< Version 2, Pin P5-03 */
    RPI_V2_GPIO_P5_04     = 29,  /*!< Version 2, Pin P5-04 */
    RPI_V2_GPIO_P5_05     = 30,  /*!< Version 2, Pin P5-05 */
    RPI_V2_GPIO_P5_06     = 31,  /*!< Version 2, Pin P5-06 */

    /* RPi B+ J8 header, also RPi 2 40 pin GPIO header */
    RPI_BPLUS_GPIO_J8_03     =  2,  /*!< B+, Pin J8-03 */
    RPI_BPLUS_GPIO_J8_05     =  3,  /*!< B+, Pin J8-05 */
    RPI_BPLUS_GPIO_J8_07     =  4,  /*!< B+, Pin J8-07 */
    RPI_BPLUS_GPIO_J8_08     = 14,  /*!< B+, Pin J8-08, defaults to alt function 0 UART0_TXD */
    RPI_BPLUS_GPIO_J8_10     = 15,  /*!< B+, Pin J8-10, defaults to alt function 0 UART0_RXD */
    RPI_BPLUS_GPIO_J8_11     = 17,  /*!< B+, Pin J8-11 */
    RPI_BPLUS_GPIO_J8_12     = 18,  /*!< B+, Pin J8-12, can be PWM channel 0 in ALT FUN 5 */
    RPI_BPLUS_GPIO_J8_13     = 27,  /*!< B+, Pin J8-13 */
    RPI_BPLUS_GPIO_J8_15     = 22,  /*!< B+, Pin J8-15 */
    RPI_BPLUS_GPIO_J8_16     = 23,  /*!< B+, Pin J8-16 */
    RPI_BPLUS_GPIO_J8_18     = 24,  /*!< B+, Pin J8-18 */
    RPI_BPLUS_GPIO_J8_19     = 10,  /*!< B+, Pin J8-19, MOSI when SPI0 in use */
    RPI_BPLUS_GPIO_J8_21     =  9,  /*!< B+, Pin J8-21, MISO when SPI0 in use */
    RPI_BPLUS_GPIO_J8_22     = 25,  /*!< B+, Pin J8-22 */
    RPI_BPLUS_GPIO_J8_23     = 11,  /*!< B+, Pin J8-23, CLK when SPI0 in use */
    RPI_BPLUS_GPIO_J8_24     =  8,  /*!< B+, Pin J8-24, CE0 when SPI0 in use */
    RPI_BPLUS_GPIO_J8_26     =  7,  /*!< B+, Pin J8-26, CE1 when SPI0 in use */
    RPI_BPLUS_GPIO_J8_29     =  5,  /*!< B+, Pin J8-29,  */
    RPI_BPLUS_GPIO_J8_31     =  6,  /*!< B+, Pin J8-31,  */
    RPI_BPLUS_GPIO_J8_32     = 12,  /*!< B+, Pin J8-32,  */
    RPI_BPLUS_GPIO_J8_33     = 13,  /*!< B+, Pin J8-33,  */
    RPI_BPLUS_GPIO_J8_35     = 19,  /*!< B+, Pin J8-35,  */
    RPI_BPLUS_GPIO_J8_36     = 16,  /*!< B+, Pin J8-36,  */
    RPI_BPLUS_GPIO_J8_37     = 26,  /*!< B+, Pin J8-37,  */
    RPI_BPLUS_GPIO_J8_38     = 20,  /*!< B+, Pin J8-38,  */
    RPI_BPLUS_GPIO_J8_40     = 21   /*!< B+, Pin J8-40,  */
} RPiGPIOPin;

uint32_t *bcm2835_peripherals_base = (uint32_t *)BCM2835_PERI_BASE;
uint32_t bcm2835_peripherals_size = BCM2835_PERI_SIZE;

/* Virtual memory address of the mapped peripherals block 
 */
uint32_t *bcm2835_peripherals = (uint32_t *)MAP_FAILED;

/* And the register bases within the peripherals block
 */
volatile uint32_t *bcm2835_gpio        = (uint32_t *)MAP_FAILED;
volatile uint32_t *bcm2835_pwm         = (uint32_t *)MAP_FAILED;
volatile uint32_t *bcm2835_clk         = (uint32_t *)MAP_FAILED;
volatile uint32_t *bcm2835_pads        = (uint32_t *)MAP_FAILED;
volatile uint32_t *bcm2835_spi0        = (uint32_t *)MAP_FAILED;
volatile uint32_t *bcm2835_bsc0        = (uint32_t *)MAP_FAILED;
volatile uint32_t *bcm2835_bsc1        = (uint32_t *)MAP_FAILED;
volatile uint32_t *bcm2835_st	       = (uint32_t *)MAP_FAILED;

#define __sync_synchronize DataMemBarrier
/* Read with memory barriers from peripheral
 *
 */
uint32_t bcm2835_peri_read(volatile uint32_t* paddr)
{
    uint32_t ret;
    {
       __sync_synchronize();
       ret = *paddr;
       __sync_synchronize();
       return ret;
    }

}

/* read from peripheral without the read barrier
 * This can only be used if more reads to THE SAME peripheral
 * will follow.  The sequence must terminate with memory barrier
 * before any read or write to another peripheral can occur.
 * The MB can be explicit, or one of the barrier read/write calls.
 */
uint32_t bcm2835_peri_read_nb(volatile uint32_t* paddr)
{
    {
	return *paddr;
    }
}

/* Write with memory barriers to peripheral
 */

void bcm2835_peri_write(volatile uint32_t* paddr, uint32_t value)
{
    {
        __sync_synchronize();
        *paddr = value;
        __sync_synchronize();
    }
}

/* write to peripheral without the write barrier */
void bcm2835_peri_write_nb(volatile uint32_t* paddr, uint32_t value)
{
    {
	*paddr = value;
    }
}


/* Set/clear only the bits in value covered by the mask
 * This is not atomic - can be interrupted.
 */
void bcm2835_peri_set_bits(volatile uint32_t* paddr, uint32_t value, uint32_t mask)
{
    uint32_t v = bcm2835_peri_read(paddr);
    v = (v & ~mask) | (value & mask);
    bcm2835_peri_write(paddr, v);
}

void bcm2835_gpio_fsel(uint8_t pin, uint8_t mode)
{
    /* Function selects are 10 pins per 32 bit word, 3 bits per pin */
    volatile uint32_t* paddr = bcm2835_gpio + BCM2835_GPFSEL0/4 + (pin/10);
    uint8_t   shift = (pin % 10) * 3;
    uint32_t  mask = BCM2835_GPIO_FSEL_MASK << shift;
    uint32_t  value = mode << shift;
    bcm2835_peri_set_bits(paddr, value, mask);
}

/* Initialise this library. */
int bcm2835_init(void)
{
    bcm2835_peripherals = (uint32_t*)BCM2835_PERI_BASE;
	bcm2835_pads = bcm2835_peripherals + BCM2835_GPIO_PADS/4;
	bcm2835_clk  = bcm2835_peripherals + BCM2835_CLOCK_BASE/4;
	bcm2835_gpio = bcm2835_peripherals + BCM2835_GPIO_BASE/4;
	bcm2835_pwm  = bcm2835_peripherals + BCM2835_GPIO_PWM/4;
	bcm2835_spi0 = bcm2835_peripherals + BCM2835_SPI0_BASE/4;
	bcm2835_bsc0 = bcm2835_peripherals + BCM2835_BSC0_BASE/4;
	bcm2835_bsc1 = bcm2835_peripherals + BCM2835_BSC1_BASE/4;
	bcm2835_st   = bcm2835_peripherals + BCM2835_ST_BASE/4;
	return 1; /* Success */
}

void bcm2835_spi_begin(void)
{
    volatile uint32_t* paddr;

    /* Set the SPI0 pins to the Alt 0 function to enable SPI0 access on them */
    bcm2835_gpio_fsel(RPI_GPIO_P1_26, BCM2835_GPIO_FSEL_ALT0); /* CE1 */
    bcm2835_gpio_fsel(RPI_GPIO_P1_24, BCM2835_GPIO_FSEL_ALT0); /* CE0 */
    bcm2835_gpio_fsel(RPI_GPIO_P1_21, BCM2835_GPIO_FSEL_ALT0); /* MISO */
    bcm2835_gpio_fsel(RPI_GPIO_P1_19, BCM2835_GPIO_FSEL_ALT0); /* MOSI */
    bcm2835_gpio_fsel(RPI_GPIO_P1_23, BCM2835_GPIO_FSEL_ALT0); /* CLK */
    
    /* Set the SPI CS register to the some sensible defaults */
    paddr = bcm2835_spi0 + BCM2835_SPI0_CS/4;
    bcm2835_peri_write(paddr, 0); /* All 0s */
    
    /* Clear TX and RX fifos */
    bcm2835_peri_write_nb(paddr, BCM2835_SPI0_CS_CLEAR);
}

void bcm2835_spi_end(void)
{  
    /* Set all the SPI0 pins back to input */
    bcm2835_gpio_fsel(RPI_GPIO_P1_26, BCM2835_GPIO_FSEL_INPT); /* CE1 */
    bcm2835_gpio_fsel(RPI_GPIO_P1_24, BCM2835_GPIO_FSEL_INPT); /* CE0 */
    bcm2835_gpio_fsel(RPI_GPIO_P1_21, BCM2835_GPIO_FSEL_INPT); /* MISO */
    bcm2835_gpio_fsel(RPI_GPIO_P1_19, BCM2835_GPIO_FSEL_INPT); /* MOSI */
    bcm2835_gpio_fsel(RPI_GPIO_P1_23, BCM2835_GPIO_FSEL_INPT); /* CLK */
}

void bcm2835_spi_setBitOrder(uint8_t __attribute__((unused)) order)
{
    /* BCM2835_SPI_BIT_ORDER_MSBFIRST is the only one suported by SPI0 */
}

/* defaults to 0, which means a divider of 65536.
// The divisor must be a power of 2. Odd numbers
// rounded down. The maximum SPI clock rate is
// of the APB clock
*/
void bcm2835_spi_setClockDivider(uint16_t divider)
{
    volatile uint32_t* paddr = bcm2835_spi0 + BCM2835_SPI0_CLK/4;
    bcm2835_peri_write(paddr, divider);
}

void bcm2835_spi_setDataMode(uint8_t mode)
{
    volatile uint32_t* paddr = bcm2835_spi0 + BCM2835_SPI0_CS/4;
    /* Mask in the CPO and CPHA bits of CS */
    bcm2835_peri_set_bits(paddr, mode << 2, BCM2835_SPI0_CS_CPOL | BCM2835_SPI0_CS_CPHA);
}

/* Writes (and reads) a single byte to SPI */
uint8_t bcm2835_spi_transfer(uint8_t value)
{
    volatile uint32_t* paddr = bcm2835_spi0 + BCM2835_SPI0_CS/4;
    volatile uint32_t* fifo = bcm2835_spi0 + BCM2835_SPI0_FIFO/4;
    uint32_t ret;

    /* This is Polled transfer as per section 10.6.1
    // BUG ALERT: what happens if we get interupted in this section, and someone else
    // accesses a different peripheral? 
    // Clear TX and RX fifos
    */
    bcm2835_peri_set_bits(paddr, BCM2835_SPI0_CS_CLEAR, BCM2835_SPI0_CS_CLEAR);

    /* Set TA = 1 */
    bcm2835_peri_set_bits(paddr, BCM2835_SPI0_CS_TA, BCM2835_SPI0_CS_TA);

    /* Maybe wait for TXD */
    while (!(bcm2835_peri_read(paddr) & BCM2835_SPI0_CS_TXD))
	;

    /* Write to FIFO, no barrier */
    bcm2835_peri_write_nb(fifo, value);

    /* Wait for DONE to be set */
    while (!(bcm2835_peri_read_nb(paddr) & BCM2835_SPI0_CS_DONE))
	;

    /* Read any byte that was sent back by the slave while we sere sending to it */
    ret = bcm2835_peri_read_nb(fifo);

    /* Set TA = 0, and also set the barrier */
    bcm2835_peri_set_bits(paddr, 0, BCM2835_SPI0_CS_TA);

    return ret;
}

/* Writes (and reads) an number of bytes to SPI */
void bcm2835_spi_transfernb(char* tbuf, char* rbuf, uint32_t len)
{
    volatile uint32_t* paddr = bcm2835_spi0 + BCM2835_SPI0_CS/4;
    volatile uint32_t* fifo = bcm2835_spi0 + BCM2835_SPI0_FIFO/4;
    uint32_t TXCnt=0;
    uint32_t RXCnt=0;

    /* This is Polled transfer as per section 10.6.1
    // BUG ALERT: what happens if we get interupted in this section, and someone else
    // accesses a different peripheral? 
    */

    /* Clear TX and RX fifos */
    bcm2835_peri_set_bits(paddr, BCM2835_SPI0_CS_CLEAR, BCM2835_SPI0_CS_CLEAR);

    /* Set TA = 1 */
    bcm2835_peri_set_bits(paddr, BCM2835_SPI0_CS_TA, BCM2835_SPI0_CS_TA);

    /* Use the FIFO's to reduce the interbyte times */
    while((TXCnt < len)||(RXCnt < len))
    {
        /* TX fifo not full, so add some more bytes */
        while(((bcm2835_peri_read(paddr) & BCM2835_SPI0_CS_TXD))&&(TXCnt < len ))
        {
           bcm2835_peri_write_nb(fifo, tbuf[TXCnt]);
           TXCnt++;
        }
        /* Rx fifo not empty, so get the next received bytes */
        while(((bcm2835_peri_read(paddr) & BCM2835_SPI0_CS_RXD))&&( RXCnt < len ))
        {
           rbuf[RXCnt] = bcm2835_peri_read_nb(fifo);
           RXCnt++;
        }
    }
    /* Wait for DONE to be set */
    while (!(bcm2835_peri_read_nb(paddr) & BCM2835_SPI0_CS_DONE))
	;

    /* Set TA = 0, and also set the barrier */
    bcm2835_peri_set_bits(paddr, 0, BCM2835_SPI0_CS_TA);
}

/* Writes an number of bytes to SPI */
void bcm2835_spi_writenb(char* tbuf, uint32_t len)
{
    volatile uint32_t* paddr = bcm2835_spi0 + BCM2835_SPI0_CS/4;
    volatile uint32_t* fifo = bcm2835_spi0 + BCM2835_SPI0_FIFO/4;
    uint32_t i;

    /* This is Polled transfer as per section 10.6.1
    // BUG ALERT: what happens if we get interupted in this section, and someone else
    // accesses a different peripheral?
    // Answer: an ISR is required to issue the required memory barriers.
    */

    /* Clear TX and RX fifos */
    bcm2835_peri_set_bits(paddr, BCM2835_SPI0_CS_CLEAR, BCM2835_SPI0_CS_CLEAR);

    /* Set TA = 1 */
    bcm2835_peri_set_bits(paddr, BCM2835_SPI0_CS_TA, BCM2835_SPI0_CS_TA);

    for (i = 0; i < len; i++)
    {
	/* Maybe wait for TXD */
	while (!(bcm2835_peri_read(paddr) & BCM2835_SPI0_CS_TXD))
	    ;
	
	/* Write to FIFO, no barrier */
	bcm2835_peri_write_nb(fifo, tbuf[i]);
	
	/* Read from FIFO to prevent stalling */
	while (bcm2835_peri_read(paddr) & BCM2835_SPI0_CS_RXD)
	    (void) bcm2835_peri_read_nb(fifo);
    }
    
    /* Wait for DONE to be set */
    while (!(bcm2835_peri_read_nb(paddr) & BCM2835_SPI0_CS_DONE)) {
	while (bcm2835_peri_read(paddr) & BCM2835_SPI0_CS_RXD)
		(void) bcm2835_peri_read_nb(fifo);
    };

    /* Set TA = 0, and also set the barrier */
    bcm2835_peri_set_bits(paddr, 0, BCM2835_SPI0_CS_TA);
}

/* Writes (and reads) an number of bytes to SPI
// Read bytes are copied over onto the transmit buffer
*/
void bcm2835_spi_transfern(char* buf, uint32_t len)
{
    bcm2835_spi_transfernb(buf, buf, len);
}

void bcm2835_spi_chipSelect(uint8_t cs)
{
    volatile uint32_t* paddr = bcm2835_spi0 + BCM2835_SPI0_CS/4;
    /* Mask in the CS bits of CS */
    bcm2835_peri_set_bits(paddr, cs, BCM2835_SPI0_CS_CS);
}

void bcm2835_spi_setChipSelectPolarity(uint8_t cs, uint8_t active)
{
    volatile uint32_t* paddr = bcm2835_spi0 + BCM2835_SPI0_CS/4;
    uint8_t shift = 21 + cs;
    /* Mask in the appropriate CSPOLn bit */
    bcm2835_peri_set_bits(paddr, active << shift, 1 << shift);
}