#ifndef __UGUI_CONFIG_H
#define __UGUI_CONFIG_H

/* -------------------------------------------------------------------------------- */
/* -- CONFIG SECTION                                                             -- */
/* -------------------------------------------------------------------------------- */

//#define USE_MULTITASKING    

/* Enable color mode */
//#define USE_COLOR_RGB888   // RGB = 0xFF,0xFF,0xFF
#if DEPTH == 16
#define USE_COLOR_RGB565   // RGB = 0bRRRRRGGGGGGBBBBB
#elif DEPTH == 8
#define USE_COLOR_INDEX
#endif

/* Enable needed fonts here */
#define  USE_FONT_4X6
#define  USE_FONT_5X8
#define  USE_FONT_5X12
#define  USE_FONT_6X8
#define  USE_FONT_6X10
#define  USE_FONT_7X12
#define  USE_FONT_8X8
//#define  USE_FONT_8X12_CYRILLIC
#define  USE_FONT_8X12
#define  USE_FONT_8X14
#define  USE_FONT_10X16
#define  USE_FONT_12X16
#define  USE_FONT_12X20
//#define  USE_FONT_16X26
//#define  USE_FONT_22X36
//#define  USE_FONT_24X40
//#define  USE_FONT_32X53

/* Specify platform-dependent integer types here */

#define __UG_FONT_DATA const
typedef uint8_t      UG_U8;
typedef int8_t       UG_S8;
typedef uint16_t     UG_U16;
typedef int16_t      UG_S16;
typedef uint32_t     UG_U32;
typedef int32_t      UG_S32;


/* Example for dsPIC33
typedef unsigned char         UG_U8;
typedef signed char           UG_S8;
typedef unsigned int          UG_U16;
typedef signed int            UG_S16;
typedef unsigned long int     UG_U32;
typedef signed long int       UG_S32;
*/

/* -------------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------------- */


/* Feature enablers */
#define USE_PRERENDER_EVENT
#define USE_POSTRENDER_EVENT


#endif
