// SPDX-License-Identifier: GPL-3.0-or-later
//
// play-macos.c
//
// Circle - A C++ bare metal environment for Raspberry Pi
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
#include <AudioToolbox/AudioToolbox.h>
#include <CoreFoundation/CoreFoundation.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
static FILE *file;
static unsigned pending=0;
static atomic_bool done=false;
static atomic_int error=0;
static void fill(void *context,AudioQueueRef queue,AudioQueueBufferRef buffer){
 (void)context;
 if(buffer->mAudioDataByteSize) --pending;
 size_t n=fread(buffer->mAudioData,1,buffer->mAudioDataBytesCapacity,file);
 buffer->mAudioDataByteSize=(UInt32)n;
 if(n){OSStatus e=AudioQueueEnqueueBuffer(queue,buffer,0,NULL);if(e){atomic_store(&error,e);atomic_store(&done,true);}else ++pending;}
 else if(!pending)atomic_store(&done,true);
}
int main(int argc,char **argv){
 if(argc!=2)return 2;file=fopen(argv[1],"rb");if(!file)return 2;
 unsigned char h[44];if(fread(h,1,44,file)!=44 || memcmp(h,"RIFF",4) || memcmp(h+8,"WAVEfmt ",8) || h[20]!=1 || h[22]!=2 || h[34]!=16 || memcmp(h+36,"data",4))return 2;
 unsigned rate=h[24]+(h[25]<<8)+(h[26]<<16)+(h[27]<<24);if(rate!=48000)return 2;
 AudioStreamBasicDescription f={0};f.mSampleRate=48000;f.mFormatID=kAudioFormatLinearPCM;
 f.mFormatFlags=kAudioFormatFlagIsSignedInteger|kAudioFormatFlagIsPacked;
 f.mBytesPerPacket=4;f.mFramesPerPacket=1;f.mBytesPerFrame=4;f.mChannelsPerFrame=2;f.mBitsPerChannel=16;
 AudioQueueRef q;OSStatus e=AudioQueueNewOutput(&f,fill,NULL,NULL,NULL,0,&q);if(e){fprintf(stderr,"AudioQueueNewOutput: %d\n",(int)e);return 3;}
 CFStringRef uid=CFSTR("BuiltInSpeakerDevice");
 e=AudioQueueSetProperty(q,kAudioQueueProperty_CurrentDevice,&uid,sizeof uid);if(e){fprintf(stderr,"Built-in speaker selection: %d\n",(int)e);return 3;}
 e=AudioQueueSetParameter(q,kAudioQueueParam_Volume,0.5f);if(e)return 3;
 for(unsigned i=0;i<3;i++){AudioQueueBufferRef b;if(AudioQueueAllocateBuffer(q,4096,&b))return 3;b->mAudioDataByteSize=0;fill(NULL,q,b);}
 e=AudioQueueStart(q,NULL);if(e){fprintf(stderr,"AudioQueueStart: %d\n",(int)e);return 3;}
 printf("Playing QEMU-rendered audio on BuiltInSpeakerDevice at 50%% queue volume.\n");fflush(stdout);
 for(unsigned i=0;i<200 && !atomic_load(&done);i++)usleep(100000);
 AudioQueueStop(q,true);AudioQueueDispose(q,true);fclose(file);
 if(!atomic_load(&done)||atomic_load(&error)){fprintf(stderr,"Playback failed or timed out: %d\n",atomic_load(&error));return 4;}
 puts("Playback completed without CoreAudio errors.");return 0;
}
