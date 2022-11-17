//
// config.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2017-2022  R. Stange <rsta2@o2online.de>
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
#ifndef _config_h
#define _config_h

//#define USE_USB			// use USB audio instead of I2S

#define SAMPLE_RATE	48000		// overall system clock

#ifndef USE_USB
#define WRITE_FORMAT	2		// 0: 8-bit unsigned, 1: 16-bit signed, 2: 24-bit signed,
					// 3: 24-bit signed (occupies 32-bit)
#else
#define WRITE_FORMAT	1
#endif

#define WRITE_CHANNELS	2		// 1: Mono, 2: Stereo

#define QUEUE_SIZE_MSECS 1000		// size of the sound queue in milliseconds duration
#define CHUNK_SIZE	1024		// number of samples, written to sound device at once

#ifdef ENABLE_RECORDER
#define DRIVE		"SD:"
#define FILEPATTERN	"/audio-%u.wav"

#define RECORD_BUTTON	17		// GPIO number of record button (chip number)
#endif

//#define INPUT_JACK	CSoundController::JackLineIn	// or: CSoundController::JackMicrophone
//#define INPUT_VOLUME	0				// in dB, or: Info.RangeMax

#endif
