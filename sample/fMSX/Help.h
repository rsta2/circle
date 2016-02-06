/** fMSX: portable MSX emulator ******************************/
/**                                                         **/
/**                          Help.h                         **/
/**                                                         **/
/** This file contains help information printed out by the  **/
/** main() routine when started with option "-help".        **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 1994-2015                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/

static const char *HelpText[] =
{
  "Usage: fmsx [-option1 [-option2...]] [filename1] [filename2]",
  "[filename1] = name of file to load as cartridge A",
  "[filename2] = name of file to load as cartridge B",
#if defined(ZLIB)
  "  This program will transparently uncompress singular GZIPped",
  "  and PKZIPped files.",
#endif
  "[-option]   =",
  "  -verbose <level>    - Select debugging messages [1]",
  "                         0 - Silent       1 - Startup messages",
  "                         2 - V9938 ops    4 - Disk/Tape",
  "                         8 - Memory      16 - Illegal Z80 ops",
  "                        32 - I/O",
  "  -skip <percent>     - Percentage of frames to skip [25]",
  "  -pal/-ntsc          - Set PAL/NTSC HBlank/VBlank periods [NTSC]",
  "  -help               - Print this help page",
  "  -home <dirname>     - Set directory with system ROM files [off]",
  "  -printer <filename> - Redirect printer output to file [stdout]",
  "  -serial <filename>  - Redirect serial I/O to a file [stdin/stdout]",
  "  -diska <filename>   - Set disk image used for drive A: [DRIVEA.DSK]",
  "                        (multiple -diska options accepted)",
  "  -diskb <filename>   - Set disk image used for drive B: [DRIVEB.DSK]",
  "                        (multiple -diskb options accepted)",
  "  -tape <filename>    - Set tape image file [off]",
  "  -font <filename>    - Set fixed font for text modes [DEFAULT.FNT]",
  "  -logsnd <filename>  - Set soundtrack log file [LOG.MID]",
  "  -state <filename>   - Set emulation state save file [automatic]",
  "  -auto/-noauto       - Use autofire on SPACE [off]",
  "  -ram <pages>        - Number of 16kB RAM pages [4/8/8]",
  "  -vram <pages>       - Number of 16kB VRAM pages [2/8/8]",
  "  -rom <type>         - Select MegaROM mapper types [8,8]",
  "                        (two -rom options accepted)",
  "                        0 - Generic 8kB   1 - Generic 16kB (MSXDOS2)",
  "                        2 - Konami5 8kB   3 - Konami4 8kB",
  "                        4 - ASCII 8kB     5 - ASCII 16kB",
  "                        6 - GameMaster2   7 - FMPAC",
  "                        >7 - Try guessing mapper type",
  "  -msx1/-msx2/-msx2+  - Select MSX model [-msx2]",
  "  -joy <type>         - Select joystick types [0,0]",
  "                        (two -joy options accepted)",
  "                        0 - No joystick",
  "                        1 - Normal joystick",
  "                        2 - Mouse in joystick mode",
  "                        3 - Mouse in real mode",
  "  -simbdos/-wd1793    - Simulate DiskROM disk access calls [-wd1793]",
  "  -sound [<quality>]  - Sound emulation quality (Hz) [44100]",
  "  -nosound            - Same as '-sound 0'",

#if defined(DEBUG)
  "  -trap <address>     - Trap execution when PC reaches address [FFFFh]",
  "                        (when keyword 'now' is used in place of the",
  "                        <address>, execution will trap immediately)",
#endif /* DEBUG */

#if defined(MSDOS) || defined(UNIX) || defined(MAEMO)
  "  -sync <frequency>   - Sync screen updates to <frequency> [60]",
  "  -nosync             - Do not sync screen updates",
  "  -tv/-notv           - Simulate/Don't simulate TV raster [-notv]",
  "  -lcd/-nolcd         - Simulate/Don't simulate LCD raster [-nolcd]",
  "  -soft/-epx/-eagle   - Scale display with 2xSal, EPX, or EAGLE [off]",
  "  -cmy/-rgb           - Simulate CMY/RGB pixel raster [off]",
#endif /* MSDOS || UNIX || MAEMO */

#if defined(UNIX) || defined(MAEMO)
  "  -saver/-nosaver     - Save CPU when inactive [-saver]",
#endif /* UNIX || MAEMO */

#if defined(UNIX)
#ifdef MITSHM
  "  -shm/-noshm         - Use MIT SHM extensions for X [-shm]",
#endif
  "  -scale <factor>     - Scale window by <factor> [2]",
#endif /* UNIX */

#if defined(MSDOS)
  "  -vsync              - Sync screen updates to VBlank [-vsync]",
#if defined(BPP8)
  "  -static/-nostatic   - Use static color palette [-nostatic]",
#endif
#if defined(NARROW)
  "  -480/-200           - Use 640x480 or 320x200 VGA mode [-200]",
#endif
#endif /* MSDOS */

  "\nKeyboard bindings:",
  "  [CONTROL]       - CONTROL (also: joystick FIRE-A button)",
  "  [SHIFT]         - SHIFT (also: joystick FIRE-B button)",
  "  [ALT]           - GRAPH (also: swap joysticks)",
  "  [INSERT]        - INSERT",
  "  [DELETE]        - DELETE",
  "  [HOME]          - HOME/CLS",
  "  [END]           - SELECT",
  "  [PGUP]          - STOP/BREAK",
  "  [PGDOWN]        - COUNTRY",
  "  [F6]            - Load emulation from .STA file",
  "  [F7]            - Save emulation state to .STA file",
  "  [F8]            - Rewind emulation back in time",
  "  [F9]            - Fast-forward emulation",
  "  [F10]           - Invoke built-in configuration menu",
  "  [F11]           - Reset hardware",
  "  [F12]           - Quit emulation",
  "  [CONTROL]+[F8]  - Toggle scanlines on/off",
  "  [ALT]+[F8]      - Toggle screen softening on/off",
#if defined(DEBUG)
  "  [CONTROL]+[F10] - Go into the built-in debugger",
#endif
  0
};
