/* HD_SPECCYS: START */

// Source: https://github.com/sadken/TZXDuino/blob/master/TZXProcessing.ino



#include "tzx-types.h"
#include "tzx-utils.h"
#include "tzx-api.h"
#include "tzx.h"



/* HD_SPECCYS: END */

#ifdef HD_SPECCYS_TZXDUINO

// There are lots of these warnings in the TZXDuiino code, so we'll ignore them
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wparentheses"
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#pragma GCC diagnostic ignored "-Warray-bounds"

// Private functions
void clearBuffer();
word TickToUs(word ticks);
void checkForEXT (char *filename);
// void TZXPlay();
bool checkForTap(char *filename);
bool checkForP(char *filename);
bool checkForO(char *filename);
bool checkForAY(char *filename);
bool checkForUEF(char *filename);
// void TZXStop();
// void TZXPause();
// void TZXLoop();
// void TZXSetup();
void TZXProcess();  // File processing loop
void StandardBlock();
void PureToneBlock();
void PulseSequenceBlock();
void PureDataBlock();
void writeData4B();
void DirectRecording();
void ZX81FilenameBlock();
void ZX8081DataBlock();
void ZX80ByteWrite();
void writeData();
void writeHeader();
// void wave();
int ReadByte(unsigned long pos);
int ReadWord(unsigned long pos);
int ReadLong(unsigned long pos);
int ReadDword(unsigned long pos);
void ReadTZXHeader();
void ReadAYHeader();
void writeSampleData();

#endif // HD_SPECCYS_TZXDUINO

void clearBuffer()
{
  
  for(int i=0;i<=buffsize;i++)
  {
    wbuffer[i][0]=0;
    wbuffer[i][1]=0;
  } 
}

word TickToUs(word ticks) {
  return (word) ((((float) ticks)/3.5)+0.5);
}


void checkForEXT (char *filename) {
  if(checkForTap(filename)) {                 //Check for Tap File.  As these have no header we can skip straight to playing data
    currentTask=PROCESSID;
    currentID=TAP;
    if((readfile(1,bytesRead))==1) {
         if (input[0] == 0x16) {
            currentID=ORIC;
         }
      }
    //printtextF(PSTR("TAP Playing"),0);
  }
  if(checkForP(filename)) {                 //Check for P File.  As these have no header we can skip straight to playing data
    currentTask=PROCESSID;
    currentID=ZXP;
    //printtextF(PSTR("ZX81 P Playing"),0);
  }
  if(checkForO(filename)) {                 //Check for O File.  As these have no header we can skip straight to playing data
    currentTask=PROCESSID;
    currentID=ZXO;
    //printtextF(PSTR("ZX80 O Playing"),0);
  }
  if(checkForAY(filename)) {                 //Check for AY File.  As these have no TAP header we must create it and send AY DATA Block after
    currentTask=GETAYHEADER;
    currentID=AYO;
    AYPASS = 0;                             // Reset AY PASS flags
    hdrptr = HDRSTART;                      // Start reading from position 1 -> 0x13 [0x00]
    //printtextF(PSTR("AY Playing"),0);
  }
  if(checkForUEF(filename)) {                 //Check for UEF File.  As these have no TAP header we must create it and send AY DATA Block after
    currentTask=GETUEFHEADER;
    currentID=UEF;
    //Serial.println(F("UEF playing"));
    //printtext("AY Playing",0);
  }  
}

void TZXPlay() {
  Timer.stop();                              //Stop timer interrupt

  // on entry, fileIndex is already pointing to the file entry you want to play
  // and fileName has already been set accordingly
  entry.close();
  entry.open(&dir, fileIndex, O_RDONLY);

  bytesRead=0;                                //start of file
  currentTask=GETFILEHEADER;                  //First task: search for header
  checkForEXT (fileName);
  currentBlockTask = READPARAM;               //First block task is to read in parameters
  clearBuffer();
  isStopped=false;
  pinState=LOW;                               //Always Start on a LOW output for simplicity
  count = 255;                                //End of file buffer flush
  EndOfFile=false;
  
  if(pinState==LOW)
  {
    LowWrite();
  }
  else
  {
    HighWrite();
  }

#ifdef HD_SPECCYS_TZXDUINO    
  Timer.setPeriod(1000);                   //set 1ms wait at start of a file (to fill initial buffer).
#else
  Timer.setPeriod(1000);                     //set 1ms wait at start of a file.
#endif

}

bool checkForTap(char *filename) {
  //Check for TAP file extensions as these have no header
  byte len = strlen(filename);
  if(strstr_P(strlwr(filename + (len-4)), PSTR(".tap"))) {
    return true;
  }
  return false;
}

bool checkForP(char *filename) {
  //Check for TAP file extensions as these have no header
  byte len = strlen(filename);
  if(strstr_P(strlwr(filename + (len-2)), PSTR(".p"))) {
    return true;
  }
  return false;
}

bool checkForO(char *filename) {
  //Check for TAP file extensions as these have no header
  byte len = strlen(filename);
  if(strstr_P(strlwr(filename + (len-2)), PSTR(".o"))) {
    return true;
  }
  return false;
}

bool checkForAY(char *filename) {
  //Check for AY file extensions as these have no header
  byte len = strlen(filename);
  if(strstr_P(strlwr(filename + (len-3)), PSTR(".ay"))) {
    return true;
  }
  return false;
}

bool checkForUEF(char *filename) {
  //Serial.println(F("checkForUEF"));
  byte len = strlen(filename);
  if(strstr_P(strlwr(filename + (len-4)), PSTR(".uef"))) {
    return true;
  }
  return false;
}

void TZXStop() {
  Timer.stop();                              //Stop timer
  isStopped=true;
  entry.close();                              //Close file
                                                                                // DEBUGGING Stuff
  //lcd.setCursor(0,1);
  //lcd.print(blkchksum,HEX); lcd.print("ck "); lcd.print(bytesRead); lcd.print(" "); lcd.print(ayblklen);
   
  bytesRead=0;                                // reset read bytes PlayBytes
  blkchksum = 0;                              // reset block chksum byte for AY loading routine
  AYPASS = 0;                                 // reset AY flag
  ID15switch = 0;                              // ID15switch
}

void TZXPause() {
  isStopped=pauseOn;
}


void TZXLoop() {
    noInterrupts();                           //Pause interrupts to prevent var reads and copy values out
    copybuff = morebuff;
    morebuff = LOW;
    isStopped = pauseOn;
    interrupts();
    if(copybuff==HIGH) {
      btemppos=0;                             //Buffer has swapped, start from the beginning of the new page
      copybuff=LOW;
    }

    if(btemppos<=buffsize)                    // Keep filling until full
    {
      TZXProcess();                           //generate the next period to add to the buffer
      if(currentPeriod>0) {
        noInterrupts();                       //Pause interrupts while we add a period to the buffer
        wbuffer[btemppos][workingBuffer ^ 1] = currentPeriod;   //add period to the buffer
        interrupts();
        btemppos+=1;
      }
    } else {
        
        if ((pauseOn == 0)&& (currpct<100)) lcdTime();  
        newpct=(100 * bytesRead)/filesize;                   
        if (currpct == 100){
            currpct = 0;
            Counter2();
        }
        if ((newpct >currpct)&& (newpct % 1 == 0)) {
          
            Counter2();
            
            currpct = newpct;
         }
    } 
}

void TZXSetup() {
    pinMode(outputPin, OUTPUT);               //Set output pin
    LowWrite();                               //Start output LOW
    isStopped=true;
    pinState=LOW;
    Timer.initialize();
}

void TZXProcess() {
    byte r = 0;
    currentPeriod = 0;
    if(currentTask == GETFILEHEADER) {
      //grab 7 byte string
      ReadTZXHeader();
      //set current task to GETID
      currentTask = GETID;
    }
    if(currentTask == GETAYHEADER) {
      //grab 8 byte string
      ReadAYHeader();
      //set current task to PROCESSID
      currentTask = PROCESSID;
    }
    if (currentTask == GETUEFHEADER) {
      //grab 12 byte string
      ReadUEFHeader();
      //set current task to GETCHUNKID
      currentTask = GETCHUNKID;
    }
    if(currentTask == GETCHUNKID) {
     
      if(r=ReadWord(bytesRead)==2) {
         chunkID = outWord;
         if(r=ReadDword(bytesRead)==4) {
            bytesToRead = outLong;
            parity = 0;

         if (chunkID == ID0104) {
              //bytesRead+= 3;
              bytesToRead+= -3;
              bytesRead+= 1;
              //grab 1 byte Parity
              if(ReadByte(bytesRead)==1) {
                if (outByte == 'O') parity = wibble ? 2 : 1;
                else if (outByte == 'E') parity = wibble ? 1 : 2;
                else parity = 0 ;  // 'N'
              }
              bytesRead+= 1;                                         
            }

         } else {
             chunkID = IDCHUNKEOF;
         }
      } else {
        chunkID = IDCHUNKEOF;
      }
      if (!uefTurboMode) {
         zeroPulse = UEFZEROPULSE;
         onePulse = UEFONEPULSE;
      } else {
         zeroPulse = UEFTURBOZEROPULSE;
         onePulse = UEFTURBOONEPULSE;  
      }
      lastByte=0;
      
      //reset data block values
      currentBit=0;
      pass=0;
      //set current task to PROCESSCHUNKID
      currentTask = PROCESSCHUNKID;
      currentBlockTask = READPARAM;
      UEFPASS = 0;
    }
    if(currentTask == PROCESSCHUNKID) {
      //CHUNKID Processing

      switch(chunkID) {
        case ID0000:
          bytesRead+=bytesToRead;
          currentTask = GETCHUNKID;
          break;
          
        case ID0100:
          
          //bytesRead+=bytesToRead;
          writeUEFData();
        break;

        case ID0104:          
          //parity = 1; // ParityOdd i.e complete with value to get Odd number of ones
          /* stopBits = */ //stopBitPulses = 1;
          writeUEFData();
          //bytesRead+=bytesToRead;
          break;
        
        case ID0110:
          if(currentBlockTask==READPARAM){
            if(r=ReadWord(bytesRead)==2) {
              if (!uefTurboMode) {     
                 pilotPulses = UEFPILOTPULSES;
                 pilotLength = UEFPILOTLENGTH;
              } else {
                // turbo mode    
                 pilotPulses = UEFTURBOPILOTPULSES;
                 pilotLength = UEFTURBOPILOTLENGTH;
                
              }
            }
            currentBlockTask = PILOT;
          } else {
            UEFCarrierToneBlock();
          }
          //bytesRead+=bytesToRead;
          //currentTask = GETCHUNKID;
        break;

        case ID0111:
          if(currentBlockTask==READPARAM){
            if(r=ReadWord(bytesRead)==2) {             
                pilotPulses = UEFPILOTPULSES; // for TURBOBAUD1500 is outWord<<2
                pilotLength = UEFPILOTLENGTH;                      
            }
            currentBlockTask = PILOT;
            UEFPASS+=1;  
          } else if (UEFPASS == 1){
              UEFCarrierToneBlock();
              if(pilotPulses==0) {
                currentTask = PROCESSCHUNKID;
                currentByte = 0xAA;
                lastByte = 1;
                currentBit = 10;
                pass=0;
                UEFPASS = 2;
              }
          } else if (UEFPASS == 2){
              parity = 0; // NoParity
              writeUEFData();
              if (currentBit==0) {
                currentTask = PROCESSCHUNKID;
                currentBlockTask = READPARAM;
              }          
          } else if (UEFPASS == 3){
            UEFCarrierToneBlock();
          }      
        break;

        case ID0112:
          //if(currentBlockTask==READPARAM){
            if(r=ReadWord(bytesRead)==2) {
              if (outWord>0) {
                //Serial.print(F("delay="));
                //Serial.println(outWord,DEC);
                temppause = outWord;
                
                currentID = IDPAUSE;
                currentPeriod = temppause;
                bitSet(currentPeriod, 15);
                currentTask = GETCHUNKID;
              } else {
                currentTask = GETCHUNKID;
              }     
            }
          //} 
        break;

        case ID0114: 
          if(r=ReadWord(bytesRead)==2) {
            pilotPulses = UEFPILOTPULSES;
            //pilotLength = UEFPILOTLENGTH;
            bytesRead-=2; 
          }
          UEFCarrierToneBlock();
          bytesRead+=bytesToRead;
          currentTask = GETCHUNKID;
        break;

        case ID0116:
          //if(currentBlockTask==READPARAM){
            if(r=ReadDword(bytesRead)==4) {
              byte * FloatB = (byte *) &outLong;
              outWord = (((*(FloatB+2)&0x80) >> 7) | (*(FloatB+3)&0x7f) << 1) + 10;
              outWord = *FloatB | (*(FloatB+1))<<8  | ((outWord&1)<<7)<<16 | (outWord>>1)<<24  ;
              outFloat = *((float *) &outWord);
              outWord= (int) outFloat;
              
              if (outWord>0) {
                //Serial.print(F("delay="));
                //Serial.println(outWord,DEC);
                temppause = outWord;
                
                currentID = IDPAUSE;
                currentPeriod = temppause;
                bitSet(currentPeriod, 15);
                currentTask = GETCHUNKID;
              } else {
                currentTask = GETCHUNKID;
              }     
            }
          //} 
        break;

        case ID0117:
            if(r=ReadWord(bytesRead)==2) {
              if (outWord == 300) {
                passforZero = 8;
                passforOne = 16;
                currentTask = GETCHUNKID;
              } else {
                passforZero = 2;
                passforOne =  4;              
                currentTask = GETCHUNKID;
              }     
            }           
          break;
          
        case IDCHUNKEOF:
        if(!count==0) {
#ifdef HD_SPECCYS_TZXDUINO
            // Pause for a long time at EOF
            // 32767 * 1000 * 100 = 3276.7 seconds ~= 1hr 
            currentPeriod = 32767;
            bitSet(currentPeriod, 15);
#else          
            //currentPeriod = 32767;
            currentPeriod = 10;
            bitSet(currentPeriod, 15); //bitSet(currentPeriod, 12);
#endif            
            count += -1;
          } else {
            bytesRead+=bytesToRead;
            stopFile();
          return;
          }
        break;
          
        default:
          //Serial.print(F("Skip id "));
          //Serial.print(chunkID);
          bytesRead+=bytesToRead;
          currentTask = GETCHUNKID;
        break;
            
      }
      } 
    
    if(currentTask == GETID) {
      //grab 1 byte ID
      if(ReadByte(bytesRead)==1) {
        currentID = outByte;
      } else {
        currentID = EOF;
      }
      //reset data block values
      currentBit=0;
      pass=0;
      //set current task to PROCESSID
      currentTask = PROCESSID;
      currentBlockTask = READPARAM;
    }
    if(currentTask == PROCESSID) {
      //ID Processing
      switch(currentID) {
        case ID10:
          //Process ID10 - Standard Block
          switch (currentBlockTask) {
            case READPARAM:
              if(r=ReadWord(bytesRead)==2) {
                pauseLength = outWord;
                // Log("pauseLength: %d", pauseLength);
              }
              if(r=ReadWord(bytesRead)==2) {
                bytesToRead = outWord +1;
                // Log("bytesToRead: %d", bytesToRead);
              }
              if(r=ReadByte(bytesRead)==1) {
                if(outByte == 0) {
                  pilotPulses = PILOTNUMBERL;
                } else {
                  pilotPulses = PILOTNUMBERH;
                }
                bytesRead += -1;
              }
              pilotLength = PILOTLENGTH;
              sync1Length = SYNCFIRST;
              sync2Length = SYNCSECOND;
              zeroPulse = ZEROPULSE;
              onePulse = ONEPULSE;
              currentBlockTask = PILOT;
              usedBitsInLastByte=8;
          break;
          
          default:
            StandardBlock();
          break;
          }

        break;
        
        case ID11:
          //Process ID11 - Turbo Tape Block
          switch (currentBlockTask) {
            case READPARAM:
              if(r=ReadWord(bytesRead)==2) {
                pilotLength = TickToUs(outWord);
              }
              if(r=ReadWord(bytesRead)==2) {
                sync1Length = TickToUs(outWord);
              }
              if(r=ReadWord(bytesRead)==2) {
                sync2Length = TickToUs(outWord);
              }
              if(r=ReadWord(bytesRead)==2) {
                zeroPulse = TickToUs(outWord);
              }
              if(r=ReadWord(bytesRead)==2) {
                onePulse = TickToUs(outWord);
              }              
              if(r=ReadWord(bytesRead)==2) {
                pilotPulses = outWord;
              }
              if(r=ReadByte(bytesRead)==1) {
                usedBitsInLastByte = outByte;
              }
              if(r=ReadWord(bytesRead)==2) {
                pauseLength = outWord;
              }
              if(r=ReadLong(bytesRead)==3) {
                bytesToRead = outLong +1;
              }
              currentBlockTask = PILOT;
          break;
          
          default:
            StandardBlock();
          break;
          }

        break;
        case ID12:
        //Process ID12 - Pure Tone Block
          if(currentBlockTask==READPARAM){
            if(r=ReadWord(bytesRead)==2) {
               pilotLength = TickToUs(outWord);
            }
            if(r=ReadWord(bytesRead)==2) {
              pilotPulses = outWord;
              //DebugBlock("Pilot Pulses", pilotPulses);
            }
            currentBlockTask = PILOT;
          } else {
            PureToneBlock();
          }
        break;

        case ID13:
        //Process ID13 - Sequence of Pulses          
          if(currentBlockTask==READPARAM) {  
            if(r=ReadByte(bytesRead)==1) {
              seqPulses = outByte;
            }
            currentBlockTask = DATA;
          } else {
            PulseSequenceBlock();
          }
        break;

        case ID14:
          //process ID14 - Pure Data Block
          if(currentBlockTask==READPARAM) {
            if(r=ReadWord(bytesRead)==2) {
              zeroPulse = TickToUs(outWord); 
            }
            if(r=ReadWord(bytesRead)==2) {
              onePulse = TickToUs(outWord); 
            }
            if(r=ReadByte(bytesRead)==1) {
              usedBitsInLastByte = outByte;
            }
            if(r=ReadWord(bytesRead)==2) {
              pauseLength = outWord; 
            }
            if(r=ReadLong(bytesRead)==3) {
              bytesToRead = outLong+1;
            }
            currentBlockTask=DATA;
          } else {
            PureDataBlock();
          }
        break;
        
        case ID15:
          //process ID15 - Direct Recording
          if(currentBlockTask==READPARAM) {
            if(r=ReadWord(bytesRead)==2) {     
              //Number of T-states per sample (bit of data) 79 or 158 - 22.6757uS for 44.1KHz
              TstatesperSample = TickToUs(outWord); 
            }
            if(r=ReadWord(bytesRead)==2) {      
              //Pause after this block in milliseconds
              pauseLength = outWord;  
            }
            if(r=ReadByte(bytesRead)==1) {
            //Used bits in last byte (other bits should be 0)
              usedBitsInLastByte = outByte;
            }
            if(r=ReadLong(bytesRead)==3) {
              // Length of samples' data
              bytesToRead = outLong+1;
            }
            currentBlockTask=DATA;
          } else {
            currentPeriod = TstatesperSample;
            bitSet(currentPeriod, 14);
            DirectRecording();
          }
        break;

        case ID19:
          //Process ID19 - Generalized data block
          switch (currentBlockTask) {
            case READPARAM:
        
              if(r=ReadDword(bytesRead)==4) {
                //bytesToRead = outLong;
              }
              if(r=ReadWord(bytesRead)==2) {
                //Pause after this block in milliseconds
                pauseLength = outWord;
                
              }
              bytesRead += 86 ;  // skip until DataStream filename
              //bytesToRead += -88 ;    // pauseLength + SYMDEFs
              currentBlockTask=DATA;
            break;
        
            case DATA:
              
              ZX8081DataBlock();
              
            break;
          }
        break;

        
        case ID20:
          //process ID20 - Pause Block
          if(r=ReadWord(bytesRead)==2) {
            if(outWord>0) {
              temppause = outWord;
              currentID = IDPAUSE;
            } else {
              currentTask = GETID;
            }
          }
        break;

        case ID21:
          //Process ID21 - Group Start
          if(r=ReadByte(bytesRead)==1) {
            bytesRead += outByte;
          }
          currentTask = GETID;
        break;

        case ID22:
          //Process ID22 - Group End
          currentTask = GETID;
        break;

        case ID24:
          //Process ID24 - Loop Start
          if(r=ReadWord(bytesRead)==2) {
            loopCount = outWord;
            loopStart = bytesRead;
          }
          currentTask = GETID;
        break;

        case ID25:
          //Process ID25 - Loop End
          loopCount += -1;
          if(loopCount!=0) {
            bytesRead = loopStart;
          } 
          currentTask = GETID;
        break;

        case ID2A:
          //Skip//
          bytesRead+=4;
          currentTask = GETID;
        break;

        case ID2B:
          //Skip//
          bytesRead+=5;
          currentTask = GETID;
        break;
        
        case ID30:
          //Process ID30 - Text Description
          if(r=ReadByte(bytesRead)==1) {
            //Show info on screen - removed until bigger screen used
            //byte j = outByte;
            //for(byte i=0; i<j; i++) {
            //  if(ReadByte(bytesRead)==1) {
            //    lcd.print(char(outByte));
            //  }
            //}
            bytesRead += outByte;
          }
          currentTask = GETID;
        break;

        case ID31:
          //Process ID31 - Message block
           if(r=ReadByte(bytesRead)==1) {
            // dispayTime = outByte;
          }         
          if(r=ReadByte(bytesRead)==1) {
            bytesRead += outByte;
          }
          currentTask = GETID;
        break;

        case ID32:
          //Process ID32 - Archive Info
          //Block Skipped until larger screen used
          if(ReadWord(bytesRead)==2) {
            bytesRead += outWord;
          }
          currentTask = GETID;
        break;

        case ID33:
          //Process ID32 - Archive Info
          //Block Skipped until larger screen used
          if(ReadByte(bytesRead)==1) {
            bytesRead += (long(outByte) * 3);
          }
          currentTask = GETID;
        break;       

        case ID35:
          //Process ID35 - Custom Info Block
          //Block Skipped
          bytesRead += 0x10;
          if(r=ReadDword(bytesRead)==4) {
            bytesRead += outLong;
          }
          currentTask = GETID;
        break;
        
        case ID4B:
          //Process ID4B - Kansas City Block (MSX specific implementation only)
          switch(currentBlockTask) {
            case READPARAM:
              if(r=ReadDword(bytesRead)==4) {  // Data size to read
                bytesToRead = outLong - 12;
              }
              if(r=ReadWord(bytesRead)==2) {  // Pause after block in ms
                pauseLength = outWord;
              }
              if (TSXspeedup == 0){
                  if(r=ReadWord(bytesRead)==2) {  // T-states each pilot pulse
                    pilotLength = TickToUs(outWord);
                  }
                  if(r=ReadWord(bytesRead)==2) {  // Number of pilot pulses
                    pilotPulses = outWord;
                  }
                  if(r=ReadWord(bytesRead)==2) {  // T-states 0 bit pulse
                    zeroPulse = TickToUs(outWord);
                  }
                  if(r=ReadWord(bytesRead)==2) {  // T-states 1 bit pulse
                    onePulse = TickToUs(outWord);
                  }
                  ReadWord(bytesRead);
              } else {
                  //Fixed speedup baudrate, reduced pilot duration
                  pilotPulses = 10000;
                  bytesRead += 10;
                  switch(BAUDRATE){
                    
                    case 1200:
                      pilotLength = onePulse = TickToUs(729);
                      zeroPulse = TickToUs(1458);                       
                      break;
                                          
                    case 2400:
                      pilotLength = onePulse = TickToUs(365);
                      zeroPulse = TickToUs(730);      
                      break;
                    
                    case 3600:
                      pilotLength = onePulse = TickToUs(243);
                      zeroPulse = TickToUs(486);      
                      break;
                      
                    case 3760:
                      pilotLength = onePulse = TickToUs(233);
                      zeroPulse = TickToUs(466);   
                      break;
                  }
               
              } //TSX_SPEEDUP


              currentBlockTask = PILOT;
            break;

            case PILOT:
                //Start with Pilot Pulses
                if (!pilotPulses--) {
                  currentBlockTask = DATA;
                } else {
                  currentPeriod = pilotLength;
                }
            break;
        
            case DATA:
                //Data playback
                writeData4B();
            break;
            
            case PAUSE:
                //Close block with a pause
                temppause = pauseLength;
                currentID = IDPAUSE;
            break;
          }
        break;

        case TAP:
          //Pure Tap file block
          switch(currentBlockTask) {
            case READPARAM:
              pauseLength = PAUSELENGTH;
              if(r=ReadWord(bytesRead)==2) {
                    bytesToRead = outWord+1;
              }
              if(r=ReadByte(bytesRead)==1) {
                if(outByte == 0) {
                  pilotPulses = PILOTNUMBERL + 1;
                } else {
                  pilotPulses = PILOTNUMBERH + 1;
                }
                bytesRead += -1;
              }
              pilotLength = PILOTLENGTH;
              sync1Length = SYNCFIRST;
              sync2Length = SYNCSECOND;
              zeroPulse = ZEROPULSE;
              onePulse = ONEPULSE;
              currentBlockTask = PILOT;
              usedBitsInLastByte=8;
            break;

            default:
              StandardBlock();
            break;
          }  
        break;

        case ZXP:
          switch(currentBlockTask) {
            case READPARAM:
              pauseLength = PAUSELENGTH*5;
              currentChar=0;
              currentBlockTask=PILOT;
            break;
            
            case PILOT:
              ZX81FilenameBlock();
            break;
            
            case DATA:
              ZX8081DataBlock();
            break;
          }
        break;

        case ZXO:
          switch(currentBlockTask) {
            case READPARAM:
              pauseLength = PAUSELENGTH*5;
              currentBlockTask=DATA;
            break;
            
            case DATA:
              ZX8081DataBlock();
            break; 
            
          }
        break;

        case AYO:                           //AY File - Pure AY file block - no header, must emulate it
          switch(currentBlockTask) {
            case READPARAM:
              pauseLength = PAUSELENGTH;  // Standard 1 sec pause
                                          // here we must generate the TAP header which in pure AY files is missing.
                                          // This was done with a DOS utility called FILE2TAP which does not work under recent 32bit OSs (only using DOSBOX).
                                          // TAPed AY files begin with a standard 0x13 0x00 header (0x13 bytes to follow) and contain the 
                                          // name of the AY file (max 10 bytes) which we will display as "ZXAYFile " followed by the 
                                          // length of the block (word), checksum plus 0xFF to indicate next block is DATA.
                                          // 13 00[00 03(5A 58 41 59 46 49 4C 45 2E 49)1A 0B 00 C0 00 80]21<->[1C 0B FF<AYFILE>CHK]
              //if(hdrptr==1) {
              //bytesToRead = 0x13-2; // 0x13 0x0 - TAP Header minus 2 (FLAG and CHKSUM bytes) 17 bytes total 
              //}
              if(hdrptr==HDRSTART) {
              //if (!AYPASS) {
                 pilotPulses = PILOTNUMBERL + 1;
              }
              else {
                 pilotPulses = PILOTNUMBERH + 1;
              }
              pilotLength = PILOTLENGTH;
              sync1Length = SYNCFIRST;
              sync2Length = SYNCSECOND;
              zeroPulse = ZEROPULSE;
              onePulse = ONEPULSE;
              currentBlockTask = PILOT;    // now send pilot, SYNC1, SYNC2 and DATA (writeheader() from String Vector on 1st pass then writeData() on second)
              if (hdrptr==HDRSTART) AYPASS = 1;     // Set AY TAP data read flag only if first run
              if (AYPASS == 2) {           // If we have already sent TAP header
                blkchksum = 0;  
                bytesRead = 0;
                bytesToRead = ayblklen+2;   // set length of file to be read plus data byte and CHKSUM (and 2 block LEN bytes)
                AYPASS = 5;                 // reset flag to read from file and output header 0xFF byte and end chksum
              }
              usedBitsInLastByte=8;
            break;

            default:
              StandardBlock();
            break;
          }  
        break;

        case ORIC:  
              //ReadByte(bytesRead);  
              //OricByteWrite();  
            switch(currentBlockTask) {              
              case READPARAM: // currentBit = 0 y count = 255 
              case SYNC1: 
                  if (currentBit >0) OricBitWrite();  
                  else {  
                       //if (count >0) {      
                          ReadByte(bytesRead);currentByte=outByte;currentBit = 11; bitChecksum = 0;lastByte=0;  
                          if (currentByte==0x16) count--; 
                          else {currentBit = 0; currentBlockTask=SYNC2;} //0x24 
                       //}  
                       //else currentBlockTask=SYNC2; 
                  }           
                  break;  
              case SYNC2:     
                  if(currentBit >0) OricBitWrite();               
                  else {  
                        if (count >0) {currentByte=0x16; currentBit = 11; bitChecksum = 0;lastByte=0; count--;} 
                        else {count=1; currentBlockTask=SYNCLAST;} //0x24   
                  } 
                  break;  
                    
              case SYNCLAST:    
                  if(currentBit >0) OricBitWrite();               
                  else {  
                        if (count >0) {currentByte=0x24; currentBit = 11; bitChecksum = 0;lastByte=0; count--;}   
                        else {count=9;lastByte=0;currentBlockTask=HEADER;}  
                  } 
                  break;  
                          
              case HEADER:              
                  if(currentBit >0) OricBitWrite(); 
                  else {  
                        if  (count >0) {  
                          ReadByte(bytesRead);currentByte=outByte;currentBit = 11; bitChecksum = 0;lastByte=0;                                          
                          if      (count == 5) bytesToRead = 256*outByte; 
                          else if (count == 4) bytesToRead += (outByte +1) ;  
                          else if (count == 3) bytesToRead -= (256 * outByte) ; 
                          else if (count == 2) bytesToRead -= outByte;  
                          count--;  
                        } 
                        else currentBlockTask=NAME; 
                  } 
                  break;  
                    
              case NAME:  
                  if (currentBit >0) OricBitWrite();  
                  else {  
                    ReadByte(bytesRead);currentByte=outByte;currentBit = 11; bitChecksum = 0;lastByte=0;  
                    if (currentByte==0x00) {count=1;currentBit = 0; currentBlockTask=NAMELAST;} 
                  }                 
                  break;  
                    
               case NAMELAST: 
                  if(currentBit >0) OricBitWrite();               
                  else {  
                        if (count >0) {currentByte=0x00; currentBit = 11; bitChecksum = 0;lastByte=0; count--;}   
                        else {count=100;lastByte=0;currentBlockTask=GAP;} 
                  } 
                  break;  
                                          
              case GAP: 
                  if(count>0) { 
                    currentPeriod = ORICONEPULSE; 
                    count--;  
                  } else {    
                    currentBlockTask=DATA;  
                  }               
                  break;  
                              
              case DATA:  
                  OricDataBlock();  
                  break;  
                    
              case PAUSE: 
                  //currentPeriod = 100; // 100ms pause 
                  //bitSet(currentPeriod, 15);  
                  if(!count==0) { 
                    currentPeriod = 32769;  
                    count += -1;  
                  } else {  
                    count= 255; 
                    currentBlockTask=SYNC1; 
                  } 
                  break;                  
            } 
            break;  
  
        
        case IDPAUSE:
                   
          if(temppause>0) {
            if(temppause > 8300) {
              //Serial.println(temppause, DEC);
              currentPeriod = 8300;
              temppause += -8300;         
            } else {
              currentPeriod = temppause;
              temppause = 0;
            }
            bitSet(currentPeriod, 15);
          } else {
            currentTask = GETID;
            if(EndOfFile==true) currentID=EOF;  
          } 
        break;
    
        case EOF:
          //Handle end of file
          if(!count==0) {
#ifdef HD_SPECCYS_TZXDUINO
            // Pause for a long time at EOF
            // 32767 * 1000 * 100 = 3276.7 seconds ~= 1hr 
            currentPeriod = 32767;
            bitSet(currentPeriod, 15);
#else
            currentPeriod = 32767;
            //currentPeriod = 2000;
            //bitSet(currentPeriod, 15); bitSet(currentPeriod, 12);
#endif
            count += -1;
          } else {
            stopFile();
            return;
          }
        break; 
        
        default:
          //stopFile();
          //ID Not Recognised - Fall back if non TZX file or unrecognised ID occurs
          
           #ifdef LCDSCREEN16x2
            lcd.clear();
            lcd.setCursor(0,0);
            lcd.print("ID? ");
            lcd.setCursor(4,0);
            lcd.print(String(currentID, HEX));
            lcd.setCursor(0,1);
            lcd.print(String(bytesRead,HEX) + " - L: " + String(loopCount, DEC));
          #endif

          #ifdef RGBLCD
            lcd.clear();
            lcd.setCursor(0,0);
            lcd.print("ID? ");
            lcd.setCursor(4,0);
            lcd.print(String(currentID, HEX));
            lcd.setCursor(0,1);
            lcd.print(String(bytesRead,HEX) + " - L: " + String(loopCount, DEC));
          #endif

          #ifdef OLED1306
              printtextF(PSTR("ID? "),0);
              itoa(currentID,PlayBytes,16);sendStrXY(PlayBytes,4,0);
              itoa(bytesRead,PlayBytes,16);strcat_P(PlayBytes,PSTR(" - L: "));printtext(PlayBytes,1);
              itoa(loopCount,PlayBytes,10);sendStrXY(PlayBytes,10,1);

          #endif 
          
          #ifdef P8544             
            lcd.clear();
            lcd.setCursor(0,0);
            lcd.print("ID? ");
            lcd.setCursor(4,0);
            lcd.print(String(currentID, HEX));
            lcd.setCursor(0,1);
            lcd.print(String(bytesRead,HEX) + " - L: " + String(loopCount, DEC));
          #endif  
  
          delay(5000);
          stopFile();
        break;
      }
    }
}

void StandardBlock() {
  //Standard Block Playback
  switch (currentBlockTask) {
    case PILOT:
        //Start with Pilot Pulses
        currentPeriod = pilotLength;
        pilotPulses += -1;
        if(pilotPulses == 0) {
          currentBlockTask = SYNC1;
        }
    break;
    
    case SYNC1:
      //First Sync Pulse
      currentPeriod = sync1Length;
      currentBlockTask = SYNC2;
    break;
    
    case SYNC2:
      //Second Sync Pulse
      currentPeriod = sync2Length;
      currentBlockTask = DATA;
    break;
    
    case DATA:  
      //Data Playback    
      if ((AYPASS==0)|(AYPASS==4)|(AYPASS==5)) writeData();   // Check if we are playing from file or Vector String and we need to send first 0xFF byte or checksum byte at EOF
      else {
        writeHeader();            // write TAP Header data from String Vector (AYPASS=1)
      }
    break;
    
    case PAUSE:
      //Close block with a pause
        // DEBUG
        //lcd.setCursor(0,1);
        //lcd.print(blkchksum,HEX); lcd.print("ck ptr:"); lcd.print(hdrptr);
            
 
      if((currentID!=TAP)&&(currentID!=AYO)) {                  // Check if we have !=AYO too
    temppause = pauseLength;
        currentID = IDPAUSE;
      } else {
    currentPeriod = pauseLength;
    bitSet(currentPeriod, 15);
        currentBlockTask = READPARAM;
    
      }
      if(EndOfFile==true) currentID=EOF;
    break;
  }
}


void PureToneBlock() {
  //Pure Tone Block - Long string of pulses with the same length
  currentPeriod = pilotLength;
  pilotPulses += -1;
  if(pilotPulses==0) {
    currentTask = GETID;
  }
}

void PulseSequenceBlock() {
  //Pulse Sequence Block - String of pulses each with a different length
  //Mainly used in speedload blocks
  byte r=0;
  if(r=ReadWord(bytesRead)==2) {
    currentPeriod = TickToUs(outWord);    
  }
  seqPulses += -1;
  if(seqPulses==0) {
    currentTask = GETID;
  }
}

void PureDataBlock() {
  //Pure Data Block - Data & pause only, no header, sync
  switch(currentBlockTask) {
    case DATA:
      writeData();          
    break;
    
    case PAUSE:
      temppause = pauseLength;
    currentID = IDPAUSE;
    break;
  }
}



void writeData4B() {
  //Convert byte (4B Block) from file into string of pulses.  One pulse per pass
  byte r;
  byte dataBit;
    
  //Continue with current byte
  if (currentBit>0) {

    //Start bit (0)
    if (currentBit==11) {
      currentPeriod = zeroPulse;
      pass+=1;
      if (pass==2) {
        currentBit += -1;
        pass = 0;
      }
    } else
    //Stop bits (1)
    if (currentBit<=2) {
      currentPeriod = onePulse;
      pass+=1;
      if (pass==4) {
        currentBit += -1;
        pass = 0;
      }
    } else
    //Data bits
    {
      dataBit = currentByte & 1;
      currentPeriod = dataBit==1 ? onePulse : zeroPulse;
      pass+=1;
      if ((dataBit==1 && pass==4) || (dataBit==0 && pass==2)) {
        currentByte >>= 1;
        currentBit += -1;
        pass = 0;
      }
    }
  }
  else
  if (currentBit==0 && bytesToRead!=0) {
    //Read new byte
    if (r=ReadByte(bytesRead)==1) {
      bytesToRead += -1; 
      currentByte = outByte;
      currentBit = 11;
      pass = 0;
    } else if (r==0) {
      //End of file
      currentID=EOF;
      return;
    }
  }
  
  //End of block?
  if (bytesToRead==0 && currentBit==0) {
    temppause = pauseLength;
    currentBlockTask = PAUSE;
  }
  
}

void DirectRecording() {
  //Direct Recording - Output bits based on specified sample rate (Ticks per clock) either 44.1KHz or 22.05
  switch(currentBlockTask) {
    case DATA:
      writeSampleData();          
    break;
    
    case PAUSE:
      temppause = pauseLength;
    currentID = IDPAUSE;
    break;
  }
}

void ZX81FilenameBlock() {
  //output ZX81 filename data  byte r;
  if(currentBit==0) {                         //Check for byte end/first byte
      //currentByte=ZX81Filename[currentChar];
      currentByte = pgm_read_byte(ZX81Filename+currentChar);
      currentChar+=1;
      if(currentChar==10) {
        currentBlockTask = DATA;
        return;
       }
    currentBit=9;
    pass=0;
  }
  /*currentPeriod = ZX80PULSE;
  if(pass==1) {
    currentPeriod=ZX80BITGAP;
  }
  if(pass==0) {
    if(currentByte&0x80) {                       //Set next period depending on value of bit 0
      pass=19;
    } else {
      pass=9;
    }
    currentByte <<= 1;                        //Shift along to the next bit
    currentBit += -1;
    currentPeriod=0;
  }
  pass+=-1;*/
  ZX80ByteWrite();
}

void ZX8081DataBlock() {
  byte r;
  if(currentBit==0) {                         //Check for byte end/first byte
    if(r=ReadByte(bytesRead)==1) {            //Read in a byte
      currentByte = outByte;     
        bytesToRead += -1;
         
          
    } else if(r==0) {
      //EndOfFile=true;
      //temppause = 3000;
      temppause = pauseLength;
      currentID = IDPAUSE;
      //return;
    }
    currentBit=9;
    pass=0;
  }
  
  /*currentPeriod = ZX80PULSE;
  if(pass==1) {
    currentPeriod=ZX80BITGAP;
  }
  if(pass==0) {
    if(currentByte&0x80) {                       //Set next period depending on value of bit 0
      pass=19;
    } else {
      pass=9;
    }
    currentByte <<= 1;                        //Shift along to the next bit
    currentBit += -1;
    currentPeriod=0;
  }
  pass+=-1;*/
  ZX80ByteWrite();
}


void ZX80ByteWrite() {
  if (uefTurboMode){
    currentPeriod = ZX80TURBOPULSE;
  if(pass==1) {
    currentPeriod=ZX80TURBOBITGAP;
  }
  }
  else {
  currentPeriod = ZX80PULSE;
  if(pass==1) {
    currentPeriod=ZX80BITGAP;
  }
  }
  if(pass==0) {
    if(currentByte&0x80) {                       //Set next period depending on value of bit 0
      pass=19;
    } else {
      pass=9;
    }
    currentByte <<= 1;                        //Shift along to the next bit
    currentBit += -1;
    currentPeriod=0;
  }
  pass+=-1;
    
}




void writeData() {
  //Convert byte from file into string of pulses.  One pulse per pass
  byte r;
  if(currentBit==0) {                         //Check for byte end/first byte
    if(r=ReadByte(bytesRead)==1) {            //Read in a byte
      currentByte = outByte;
      if (AYPASS==5) {
        currentByte = 0xFF;                 // Only insert first DATA byte if sending AY TAP DATA Block and don't decrement counter
        AYPASS = 4;                         // set Checksum flag to be sent when EOF reached
        bytesRead += -1;                    // rollback ptr and compensate for dummy read byte
        bytesToRead += 2;                   // add 2 bytes to read as we send 0xFF (data flag header byte) and chksum at the end of the block
      } else {
        bytesToRead += -1;  
      }
      blkchksum = blkchksum ^ currentByte;    // keep calculating checksum
      if(bytesToRead == 0) {                  //Check for end of data block
        bytesRead += -1;                      //rewind a byte if we've reached the end
        if(pauseLength==0) {                  //Search for next ID if there is no pause
          currentTask = GETID;
        } else {
          currentBlockTask = PAUSE;           //Otherwise start the pause
        }
        return;                               // exit
      }
    } else if(r==0) {                         // If we reached the EOF
        if (AYPASS!=4) {                   // Check if need to send checksum
          EndOfFile=true;
          if(pauseLength==0) {
            currentTask = GETID;
          } else {
            currentBlockTask = PAUSE;
          }
          return;                           // return here if normal TAP or TZX
        } else {
          currentByte = blkchksum;            // else send calculated chksum
          bytesToRead += 1;                   // add one byte to read
          AYPASS = 0;                         // Reset flag to end block
        }
      //return;
    }
    if(bytesToRead!=1) {                      //If we're not reading the last byte play all 8 bits
      currentBit=8;
    } else {
      currentBit=usedBitsInLastByte;          //Otherwise only play back the bits needed
    }
    pass=0;
  } 
 if(currentByte&0x80) {                       //Set next period depending on value of bit 0
    currentPeriod = onePulse;
  } else {
    currentPeriod = zeroPulse;
  }
  pass+=1;                                    //Data is played as 2 x pulses
  if(pass==2) {
    currentByte <<= 1;                        //Shift along to the next bit
    currentBit += -1;
    pass=0;  
  }    
}

void writeHeader() {
  //Convert byte from HDR Vector String into string of pulses and calculate checksum. One pulse per pass
  if(currentBit==0) {                         //Check for byte end/new byte                         
    if(hdrptr==19) {              // If we've reached end of header block send checksum byte
      currentByte = blkchksum;
      AYPASS = 2;                 // set flag to Stop playing from header in RAM 
      currentBlockTask = PAUSE;   // we've finished outputting the TAP header so now PAUSE and send DATA block normally from file
      return;
    }
    hdrptr += 1;                   // increase header string vector pointer
    if(hdrptr<20) {                     //Read a byte until we reach end of tap header
      //currentByte = TAPHdr[hdrptr];
       currentByte = pgm_read_byte(TAPHdr+hdrptr);
      if(hdrptr==13){                           // insert calculated block length minus LEN bytes
            currentByte = lowByte(ayblklen-3);
      } else if(hdrptr==14){
            currentByte = highByte(ayblklen);
      }
      blkchksum = blkchksum ^ currentByte;    // Keep track of Chksum
    //}    
    //if(hdrptr<20) {               //If we're not reading the last byte play all 8 bits
    //if(bytesToRead!=1) {                      //If we're not reading the last byte play all 8 bits
      currentBit=8;
    } else {
      currentBit=usedBitsInLastByte;          //Otherwise only play back the bits needed
    }   
    pass=0; 
 } //End if currentBit == 0
 if(currentByte&0x80) {                       //Set next period depending on value of bit 0
    currentPeriod = onePulse;
  } else {
    currentPeriod = zeroPulse;
  }
  pass+=1;                                    //Data is played as 2 x pulses
  if(pass==2) {
    currentByte <<= 1;                        //Shift along to the next bit
    currentBit += -1;
    pass=0;  
  }    
}  // End writeHeader()

void wave() {
  //ISR Output routine
  //unsigned long fudgeTime = micros();         //fudgeTime is used to reduce length of the next period by the time taken to process the ISR
  word workingPeriod = wbuffer[pos][workingBuffer];
  byte pauseFlipBit = false;
  unsigned long newTime=1;
  intError = false;

  // Log("wave: isStopped=%d pos=%d, workingBuffer=%d, workingPeriod=%d", isStopped, pos, workingBuffer, workingPeriod);

  if(isStopped==0 && workingPeriod >= 1)
  {
      if bitRead(workingPeriod, 15)          
      {
        //If bit 15 of the current period is set we're about to run a pause
        //Pauses start with a 1.5ms where the output is untouched after which the output is set LOW
        //Pause block periods are stored in milliseconds not microseconds
        isPauseBlock = true;
        bitClear(workingPeriod,15);         //Clear pause block flag
        pinState = !pinState;
        pauseFlipBit = true;
        wasPauseBlock = true;
      } else {
         if(workingPeriod >= 1 && wasPauseBlock==false) {
          pinState = !pinState;
        } else if (wasPauseBlock==true && isPauseBlock==false) {
          wasPauseBlock=false;
        }
        //if (wasPauseBlock==true && isPauseBlock==false) wasPauseBlock=false; 
      }
      
      if (ID15switch == 1){
        if (bitRead(workingPeriod, 14)== 0)
        {
          //pinState = !pinState;
          if (pinState == LOW)
          {
            LowWrite();
          }
          else
          {
            HighWrite();
          }
        }
        else
        {
          if (bitRead(workingPeriod, 13) == 0)
          {
            LowWrite();
          }
          else
          {
            HighWrite();
            bitClear(workingPeriod,13);
          }
          bitClear(workingPeriod,14);         //Clear ID15 flag
          workingPeriod = TstatesperSample;
        }
      } 
      else {
        //pinState = !pinState;
        if(pinState==LOW)
        {
          LowWrite();
        }
        else
        {
          HighWrite();
        }
      }
      
      if(pauseFlipBit==true) {
        newTime = 1500;                     //Set 1.5ms initial pause block
        //pinState = LOW;                     //Set next pinstate LOW
        if (!FlipPolarity) {
          pinState = LOW;
        } else {
          pinState = HIGH;
        }
        wbuffer[pos][workingBuffer] = workingPeriod - 1;  //reduce pause by 1ms as we've already pause for 1.5ms
        pauseFlipBit=false;
      } else {
        if(isPauseBlock==true) {
          newTime = long(workingPeriod)*1000; //Set pause length in microseconds
          isPauseBlock = false;
        } else {
          newTime = workingPeriod;          //After all that, if it's not a pause block set the pulse period 
        }
        pos += 1;
        if(pos > buffsize)                  //Swap buffer pages if we've reached the end
        {
          pos = 0;
          workingBuffer^=1;
          morebuff = HIGH;                  //Request more data to fill inactive page
        } 
     }
  } else if(workingPeriod <= 1 && isStopped==0) {
    newTime = 1000;                         //Just in case we have a 0 in the buffer
    pos += 1;
    if(pos > buffsize) {
      pos = 0;
      workingBuffer ^= 1;
      morebuff = HIGH;
    }
  } else {
    newTime = 1000000;                         //Just in case we have a 0 in the buffer
  }

#ifdef HD_SPECCYS_TZXDUINO
// TODO - call a function to get the fudge time
  unsigned long nextPeriod = newTime - 19;
  if (nextPeriod <= 0) nextPeriod = 1;
  Timer.setPeriod(nextPeriod);    //Finally set the next pulse length
#else
  //newTime += 12;
  //fudgeTime = micros() - fudgeTime;         //Compensate for stupidly long ISR
  //Timer.setPeriod(newTime - fudgeTime);    //Finally set the next pulse length
  Timer.setPeriod(newTime +4);    //Finally set the next pulse length
#endif  
}

int ReadByte(unsigned long pos) {
  //Read a byte from the file, and move file position on one if successful
  byte out[1];
  int i=0;
  if(entry.seekSet(pos)) {
    i = entry.read(out,1);
    if(i==1) bytesRead += 1;
  }
  outByte = out[0];
  //blkchksum = blkchksum ^ out[0];
  return i;
}

int ReadWord(unsigned long pos) {
  //Read 2 bytes from the file, and move file position on two if successful
  byte out[2];
  int i=0;
  if(entry.seekSet(pos)) {
    i = entry.read(out,2);
    if(i==2) bytesRead += 2;
  }
  outWord = word(out[1],out[0]);
  //blkchksum = blkchksum ^ out[0] ^ out[1];
  return i;
}

int ReadLong(unsigned long pos) {
  //Read 3 bytes from the file, and move file position on three if successful
  byte out[3];
  int i=0;
  if(entry.seekSet(pos)) {
    i = entry.read(out,3);
    if(i==3) bytesRead += 3;
  }
  outLong = ((unsigned long) word(out[2],out[1]) << 8) | out[0];
  //outLong = (word(out[2],out[1]) << 8) | out[0];
  //blkchksum = blkchksum ^ out[0] ^ out[1] ^ out[2];
  return i;
}

int ReadDword(unsigned long pos) {
  //Read 4 bytes from the file, and move file position on four if successful  
  byte out[4];
  int i=0;
  if(entry.seekSet(pos)) {
    i = entry.read(out,4);
    if(i==4) bytesRead += 4;
  }
  outLong = ((unsigned long) word(out[3],out[2]) << 16) | word(out[1],out[0]);
  //outLong = (word(out[3],out[2]) << 16) | word(out[1],out[0]);
  //blkchksum = blkchksum ^ out[0] ^ out[1] ^ out[2] ^ out[3];
  return i;
}

void ReadTZXHeader() {
  //Read and check first 10 bytes for a TZX header
  char tzxHeader[11];
  int i=0;
  
  if(entry.seekSet(0)) {
    i = entry.read(tzxHeader,10);
    i = i; // HD_SPECCYS - unused
    if(memcmp_P(tzxHeader,TZXTape,7)!=0) {
      printtextF(PSTR("Not TZXTape"),1);
      //lcd_clearline(1);
      //lcd.print(F("Not TZXTape"));     
      TZXStop();
    }
  } else {
    printtextF(PSTR("Error Reading File"),0);
    //lcd_clearline(0);
    //lcd.print(F("Error Reading File"));        
  }
  bytesRead = 10;
}

void ReadAYHeader() {
  //Read and check first 8 bytes for a TZX header
  char ayHeader[9];
  int i=0;
  
  if(entry.seekSet(0)) {
    i = entry.read(ayHeader,8);
    i = i; // HD_SPECCYS - unused
    if(memcmp_P(ayHeader,AYFile,8)!=0) {
      printtextF(PSTR("Not AY File"),1);
      //lcd_clearline(0);
      //lcd.print(F("Not AY File"));    
      TZXStop();
    }
  } else {
    printtextF(PSTR("Error Reading File"),0);
    //lcd_clearline(0);
    //lcd.print(F("Error Reading File"));    
  }
  bytesRead = 0;
}


void writeSampleData() {
  //Convert byte from file into string of pulses.  One pulse per pass
  byte r;
  ID15switch = 1;
  if(currentBit==0) {                         //Check for byte end/first byte
    if(r=ReadByte(bytesRead)==1) {            //Read in a byte
      currentByte = outByte;
      bytesToRead += -1;
      if(bytesToRead == 0) {                  //Check for end of data block
        bytesRead += -1;                      //rewind a byte if we've reached the end
        if(pauseLength==0) {                  //Search for next ID if there is no pause
          currentTask = GETID;
        } else {
          currentBlockTask = PAUSE;           //Otherwise start the pause
        }
        return;
      }
    } else if(r==0) {
      EndOfFile=true;
      if(pauseLength==0) {
        //ID15switch = 0;
        currentTask = GETID;
      } else {
        currentBlockTask = PAUSE;
      }
      return;
    }
    if(bytesToRead!=1) {                      //If we're not reading the last byte play all 8 bits
      currentBit=8;
    } else {
      currentBit=usedBitsInLastByte;          //Otherwise only play back the bits needed
    }
    pass=0;
  } 
 if bitRead(currentPeriod, 14) {
    //bitWrite(currentPeriod,13,currentByte&0x80);
    if(currentByte&0x80) bitSet(currentPeriod, 13);
    pass+=2;
  }
  else {
    if(currentByte&0x80){                       //Set next period depending on value of bit 0
        currentPeriod = onePulse;
      } else {
        currentPeriod = zeroPulse;
      }
      pass+=1;
}
if(pass==2) {
    currentByte <<= 1;                        //Shift along to the next bit
    currentBit += -1;
    pass=0;  
  }
}


#ifdef HD_SPECCYS_TZXDUINO

#pragma GCC diagnostic pop

#endif // HD_SPECCYS_TZXDUINO
