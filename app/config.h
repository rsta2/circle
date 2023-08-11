/**
 * config.h
 *
 * Configuration options for the HD Speccy project
 * 
 * NOTE: REALTIME / USE_SDHOST / SCHEDULER_IN_FIQ must all be defined for the project to work correctly
 * 
 * NOTE: TODO This file is included by the main project Makefile, so it must be valid C code
 * 
 */
// #ifndef _config_h
// #define _config_h


//
// Hardware configuration
//

// Board definitions (do not change)
#define HD_SPECCYS_BOARD_PROTOTYPE_1		0
#define HD_SPECCYS_BOARD_REV_1_0A			1

// IRQ definitions (do not change)
#define HD_SPECCYS_ZX_SMI_INT_IRQ			0
#define HD_SPECCYS_ZX_SMI_BORDER_IRQ		1
#define HD_SPECCYS_ZX_TAPE_TIMER_IRQ		3


/**
 * HD_SPECCYS_BOARD_REV
 * 
 * Defines hardware to compile for
 * 
 * Set to one of: 
 * HD_SPECCYS_BOARD_PROTOTYPE_1
 * HD_SPECCYS_BOARD_REV_1_0A
 * 
 */
#ifndef HD_SPECCYS_BOARD_REV
#define HD_SPECCYS_BOARD_REV HD_SPECCYS_BOARD_REV_1_0A
#endif

/**
 * HD_SPECCYS_FIQ
 * 
 * Defines which hardware is connected to the single available FIQ
 * 
 * Set to one of:
 * HD_SPECCYS_ZX_SMI_INT_IRQ <-- hangs as soon as border IRQ occurs
 * HD_SPECCYS_ZX_SMI_BORDER_IRQ <-- no screen hangs for some reason?
 * HD_SPECCYS_ZX_TAPE_TIMER_IRQ <-- works
 * FALSE - No hardware connected to the FIQ <-- hangs as soon as border IRQ occurs
 * 
 * If tape is disabled, the behaviour is actually the same, tape was a red herring - it just triggered border IRQs:
 * 
 * Something UGLY is going on!
 * 
 * I think I have solved the mystery - the GPIO interrupts are actually really only 1 single interrupt, the 
 * 'GPIO' interrupt (which is set in gpiomanager.cpp, ARM_IRQ_GPIO3). This means that if ANY GPIO is set to FIQ,
 * all GPIO interrupts will be FIQ, and must be handled as such!
 * 
 * Unfortunately, circle does not support multiple GPIO lines on the FIQ with a gpio manager - maybe because
 * the FIQ should be ultra fast, and the GPIO manager is not?
 * 
 * Therefore, either we cannot use FIQ for GPIOs when using more than one, or we have to write our own FIQ GPIO manager.
 * A quick look at the code suggests that the FIQ GPIO manager should be possible! 
 * FIQ could be a flag to the original GPIOManager, or a new set of managed pins with an FIQ manager could be created.
 * The best would be a GPIO manager flag, as then it would be easy to switch and the existing single FIQ GPIO pin 
 * code could still be used for a super fast, but very limiting GPIO FIQ.
 * 
 */
#ifndef HD_SPECCYS_FIQ
#define HD_SPECCYS_FIQ HD_SPECCYS_ZX_SMI_INT_IRQ
#endif

//
// HD Speccys feature configuration
//

/**
 * HD_SPECCYS_FEATURE_NETWORK
 * 
 * Use network and WiFi features
 * 
 * TRUE - Compile in network and WiFi features
 * FALSE - No network or WiFi features
 */
#ifndef HD_SPECCYS_FEATURE_NETWORK
#define HD_SPECCYS_FEATURE_NETWORK FALSE
#endif

/**
 * HD_SPECCYS_FEATURE_OPENGL
 * 
 * Use OpenGL or the framebuffer for screen output
 * 
 * TRUE - Compile in OpenGL drivers and use OpenGL for the screen output
 * FALSE - No OpenGL drivers, use the framebuffer for screen output
 */
#ifndef HD_SPECCYS_FEATURE_OPENGL
#define HD_SPECCYS_FEATURE_OPENGL FALSE
#endif


//
// Serial configuration
//

/**
 * HD_SPECCYS_SERIAL_BAUD_RATE
 * 
 * Defines baud rate to use for serial console
 * 
 * Set to one of: 
 * 115200
 * 230400
 * 460800 <== Fastest baud rate that works reliably on MacOS with the FTDI USB serial driver
 * 576000
 * 921600
 * 
 */
#ifndef HD_SPECCYS_SERIAL_BAUD_RATE
#define HD_SPECCYS_SERIAL_BAUD_RATE 460800
#endif


//
// Network configuration
//

/**
 * HD_SPECCYS_NETWORK_USE_DHCP
 * 
 * Use DHCP or a fixed IP address
 * 
 * TRUE - Use DCHP
 * FALSE - Use a fixed IP address
 */
#ifndef HD_SPECCYS_NETWORK_USE_DHCP
#define HD_SPECCYS_NETWORK_USE_DHCP TRUE
#endif


//
// ZX screen configuration
//

/**
 * ZX_SCREEN_DEPTH
 * 
 * 8 - 8-bit colour
 * 16 - 16-bit colour
 * 32 - 32-bit colour
 */
#ifndef ZX_SCREEN_DEPTH
#define ZX_SCREEN_DEPTH 16
#endif

/**
 * ZX_SCREEN_COLOUR
 * 
 * TRUE - colour
 * FALSE - black and white (for testing)
 */
#ifndef ZX_SCREEN_COLOUR
#define ZX_SCREEN_COLOUR TRUE
#endif

/**
 * ZX_SCREEN_DMA
 * 
 * TRUE - DMA used for copying data to the framebuffer
 * FALSE - memcpy used for copying data to the framebuffer (faster as don't have to wait for DMA to complete)
 */
#ifndef ZX_SCREEN_DMA
#define ZX_SCREEN_DMA FALSE	// Disabled as waiting for DMA to complete is slower than just doing a memcpy().
#endif

/**
 * ZX_SCREEN_DMA_BURST_COUNT
 * 
 * number - burst count for DMA
 */
#ifndef ZX_SCREEN_DMA_BURST_COUNT
#define ZX_SCREEN_DMA_BURST_COUNT 0
#endif

/**
 * ZX_SCREEN_REVERSE_DATA_BITS
 * 
 * DO NOT SET DIRECTLY
 * 
 * TRUE - data bits will be reversed (fixes a hardware bug)
 * FALSE - data bits will not be reversed
 */
#ifndef ZX_SCREEN_REVERSE_DATA_BITS
#define ZX_SCREEN_REVERSE_DATA_BITS (HD_SPECCYS_BOARD_REV == HD_SPECCYS_BOARD_REV_1_0A)
#endif

//
// ZX tape configuration
//

/**
 * ZX_TAPE_GPIO_OUTPUT_PIN
 * 
 * DO NOT SET DIRECTLY
 * 
 * Selects the GPIO output pin to use for the tape playback
 * 
 * TODO - set from HW configuration
 */
#ifndef ZX_TAPE_GPIO_OUTPUT_PIN
#define ZX_TAPE_GPIO_OUTPUT_PIN 	7	// GPIO 7 (HW PIN 26) - Rev 1.0a
// #define ZX_TAPE_GPIO_OUTPUT_PIN	0	// GPIO 0 (HW PIN 27) - Hand wired prototype board
#endif

/**
 * ZX_TAPE_USE_FIQ
 * 
 * DO NOT SET DIRECTLY
 * 
 * Selects whether to use a FIQ or a standard IRQ for the tape timer
 * 
 * TRUE - Use FIQ 
 * FALSE - Use a standard IRQ
 */
#ifndef ZX_TAPE_USE_FIQ
#define ZX_TAPE_USE_FIQ 	(HD_SPECCYS_FIQ == HD_SPECCYS_ZX_TAPE_TIMER_IRQ)
#endif

//
// ZX SMI configuration
//

/**
 * ZX_SMI_GPIO_INT_INPUT_PIN
 * 
 * DO NOT SET DIRECTLY
 * 
 * Selects the GPIO output pin to use for the tape playback
 * 
 * TODO - set from HW configuration
 */
#ifndef ZX_SMI_GPIO_INT_INPUT_PIN
#define ZX_SMI_GPIO_INT_INPUT_PIN 	27 // GPIO 27 (HW PIN 13) - Rev 1.0a
#endif

/**
 * ZX_SMI_GPIO_BORDER_INPUT_PIN
 * 
 * DO NOT SET DIRECTLY
 * 
 * Selects the GPIO output pin to use for the tape playback
 * 
 * TODO - set from HW configuration
 */
#ifndef ZX_SMI_GPIO_BORDER_INPUT_PIN
#define ZX_SMI_GPIO_BORDER_INPUT_PIN 	26 // GPIO 26 (HW PIN 37) - Rev 1.0a
#endif

/**
 * ZX_SMI_INT_USE_FIQ
 * 
 * DO NOT SET DIRECTLY
 * 
 * Selects whether to use a FIQ or a standard IRQ for the SMI screen vsync interrupt (Zx Spectrum INT signal)
 * 
 * TRUE - Use FIQ 
 * FALSE - Use a standard IRQ
 */
#ifndef ZX_SMI_INT_USE_FIQ
#define ZX_SMI_INT_USE_FIQ 	(HD_SPECCYS_FIQ == HD_SPECCYS_ZX_SMI_INT_IRQ)
#endif

/**
 * ZX_SMI_BORDER_USE_FIQ
 * 
 * DO NOT SET DIRECTLY
 * 
 * Selects whether to use a FIQ or a standard IRQ for the SMI border interrupt (Zx Spectrum IOREQ & WR signal)
 * 
 * TRUE - Use FIQ 
 * FALSE - Use a standard IRQ
 */
#ifndef ZX_SMI_BORDER_USE_FIQ
#define ZX_SMI_BORDER_USE_FIQ 	(HD_SPECCYS_FIQ == HD_SPECCYS_ZX_SMI_BORDER_IRQ)
#endif

// #endif // _config_h
