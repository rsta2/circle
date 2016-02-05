


                                fMSX-SDL 2.6.0.40


                    Contact address: vincentd@kip.sateh.com
                Official site: http://www.igr.nl/users/vincentd



   This is another port of the great MSX emulator fMSX by Marat Fayzullin. Most
important features  of this  emulator  is that  it supports  the MSX-AUDIO  and 
MSX-MUSIC  sounds systems. Together with  the great VDP emulation  of fMSX this
port is very suitable to emulate demo's.



                              System Requirements
                              -------------------

   To run the win32 version in full flavour (with MSX-AUDIO/MSX-MUSIC emulation
in high  resolution)  a Pentium-II-300 is  required  as a  minimum.  For normal
emulation (in  low resolution) it  should run  fine with  a Pentium-200 or even
less. DirectX is required.


                                  How to use
                                  ----------

   The win32 binary still has most of the command line options available from
the original fMSX distribution, however for your convenience this  distribution 
includes  a front end with  which you can control all  basic features. To start
the front end execute the "launcher.exe" program, select your favorite disk  or
rom image and push the "Go!" button. As a  reference all the  available command 
line options are listed at the end of this document. 


                                Multiple Disks
                                --------------

   If you use the win32 binary with front end you can select the multiple disks
for both the drives  by adding disks with the add button. The eject button will
erase  all selected disks.  If you are not  using the win32  front end  you can 
select disks by giving multiple -diska arguments (as described in "Command line
options"). In both cases you can change the disk during emulation with CTRL-F10
and CTRL-F11. A notification with the current disk will appear.


                                 Choppy Sound
                                 ------------

   If the sound sounds choppy, you can fix it by setting the audio  buffer to a
greater size. This  can  be  done in  the front-end by  pressing  the configure 
button. A new window will appear in which you  can change the size of the audio
buffer, a value of 1024 should be sufficient, but lesser values  sound  better.
If possible, I would  recomend a buffer size of 512 bytes. The settings will be
saved when you press ok.


                               Keyboard mapping
                               ----------------

                    CTRL   = Left Ctrl
                    GRAPH  = Left Alt
                    CODE   = Right Alt
                    DEAD   = Right Ctrl
                    SELECT = F7
                    STOP   = F8

                   CTRL+F6 = Autofire on/off
                        F9 = Load emulation state file
                   CTRL+F9 = Save emulation state file
                  CTRL+F10 = next disk driva a: 
                  CTRL+F11 = next disk drive b:
                       END = switch filter mode
                  CTRL+END = switch mono/stereo
                  SCRL-LCK = reset
             CTRL+SCRL-LCK = hard reset
                       F12 = Exit



                             Command line options
                             --------------------

  -diska <filename>   - Set disk image used for drive A: [DRIVEA.DSK]
                        (multiple -diska options accepted)
  -diskb <filename>   - Set disk image used for drive B: [DRIVEB.DSK]
                        (multiple -diskb options accepted)
  -tape <filename>    - Set tape image file [off]
  -auto/-noauto       - Use autofire on SPACE [off]
  -ram <pages>        - Number of 16kB RAM pages [4/8/8]
  -vram <pages>       - Number of 16kB VRAM pages [2/8/8]
  -rom <type>         - Select MegaROM mapper types [6,6]
                        (two -rom options accepted)
                         0 - Generic 8kB   1 - Generic 16kB (MSXDOS2)
                         2 - Konami5 8kB   3 - Konami4 8kB
                         4 - ASCII 8kB     5 - ASCII 16kB
                         6 - GameMaster2
                         >6 - Try guessing mapper type
  -state <filename>   - Set emulation state save file [automatic]
  -msx1/-msx2/-msx2+  - Select MSX model [-msx2]
  -msxmusic           - Enable MSX-MUSIC emulation
  -msxaudio           - Enable MSX-AUDIO emulation
  -stereo             - Enable pseudo stereo sound
  -buffer <size>      - Buffer size used for audio emulation, if the 
                        size of the buffer is too small it will result
                        in ticks in the audio stream. A small buffer
                        size will result in more accurate sound emulation. 
  -filter <mode>      - Select filter mode
                         0 - full scanlines    3 - no scanlines
                         1 - half scanlines    4 - half blur
                         2 - mix scanlines     5 - full blur
  -lowres             - For low resolution (faster) graphics.


                         Acknowlegements and thanks
                         --------------------------

fMSX-SDL port by Vincent van Dam (2001).
Original fMSX by Marat Fayzullin (1994-2001).
YM2413/PSG/SCC emulation by Mitsutaka Okazaki (2001).
Y8950 emulation by Tatsuyuki Satoh (1999/2000).

Thanks to Arnaud de Klerk for his great PR work.
Thanks to Caz for compiling the code on BeOS.
Thanks to Benoit Delvaux for his enthousiasm and support.


                              Version history
                              ---------------

[2002/03/10] 2.6.0.40
---------------------
* added reset (scroll lock/ctrl+scroll lock)
* added tape rewind (ctrl+f7)
* improved emulation timing
* stereo
* 256K sampleram
* added filters
* fix midi bug port bug
* replaced psg/scc emulation by Mitsutaka Okazaki's psg/scc emulators
* front-end diskb image bug
* upgraded base fMSX source from 2.5 to 2.6 


[2001/07/22] 2.5.0.33
---------------------
* fixed sprite bug (in MSX-1 screens)
* added notification for disk switching, autofire and state file
* added functionality for multiple disks in the front end
* added ability to save default configuration of front end
* changed the default buffer size to 1024 in the configuration
* changed audio output from 8 bits to 16 bits
* upgraded emu2413 emulation from 0.38 to 0.50
* upgraded base fMSX source from 2.4 to 2.5
* published source code as patch on fMSX 2.5 code

[2001/06/30] 2.4.0.32
---------------------
* first public release
