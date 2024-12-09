/*
 * cflashy.c
 *
 * Circle - A C++ bare metal environment for Raspberry Pi
 * Copyright (C) 2024  R. Stange <rsta2@o2online.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
/*
 * This flash tool adapts the most important features
 * of the Flashy tool (version 1), which was developed
 * by Brad Robinson <contact@toptensoftware.com>.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <assert.h>

/* Parameters */

static struct
{
	const char	*pSerialPort;
	const char	*pHexFile;
	unsigned	 nFlashBaud;
	unsigned	 nUserBaud;
	const char	*pRebootMagic;
	unsigned	 nRebootDelay;
	unsigned	 nGoDelay;
	unsigned	 nPacketSize;
	int		 bMonitor;
}
ToolParam = {NULL, NULL, 115200, 115200, NULL, 1000, 0, 0, 0};

/* Usage */

static const char Usage[] =
{
	"Usage: cflashy serialport [ hexfile ] [ options ]\n"
	"A flash tool, written in C\n"
	"\n"
	"serialport\t  Serial port to access (e.g. /dev/ttyUSB0, /dev/ttyS3, COM3)\n"
	"hexfile\t\t  The .hex file to be uploaded\n"
	"--flashbaud:BAUD  Baud rate for flashing (default=115200)\n"
	"--userbaud:BAUD\t  Baud rate for monitor and reboot magic (default=115200)\n"
	"--godelay:MS\t  Sets a delay period for the go command\n"
	"--reboot:MAGIC\t  Sends a magic reboot string at user baud before flashing\n"
	"--rebootdelay:MS  Delay after sending reboot magic (default=1000 ms)\n"
	"--packetsize:BYTES Data is uploaded in chunks of this size\n"
	"--monitor\t  Monitor serial port input to stderr\n"
};

/* Serial interface routines */

#ifndef WIN32

/* Linux support */

#include <termios.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>

typedef int TSerialPort;

#define IS_VALID(port)		((port) >= 0)

/* Returns B0, if baud rate is not supported */
static speed_t Baud2Speed (unsigned nBaud)
{
	static const struct
	{
		unsigned nBaud;
		speed_t  Speed;
	}
	BaudRates[] =
	{
		{9600,		B9600},
		{19200,		B19200},
		{38400,		B38400},
		{57600,		B57600},
		{115200,	B115200},
		{230400,	B230400},
#ifndef __APPLE__
		{460800,	B460800},
		{500000,	B500000},
		{576000,	B576000},
		{921600,	B921600},
		{1000000,	B1000000},
		{1152000,	B1152000},
		{1500000,	B1500000},
		{2000000,	B2000000},
		{2500000,	B2500000},
		{3000000,	B3000000},
		{3500000,	B3500000},
		{4000000,	B4000000},
#endif
		{0,		B0}
	};

	for (unsigned i = 0; BaudRates[i].nBaud; i++)
	{
		if (BaudRates[i].nBaud == nBaud)
		{
			return BaudRates[i].Speed;
		}
	}

	return B0;
}

static TSerialPort SerialOpen (unsigned nBaud)
{
	speed_t Speed = Baud2Speed (nBaud);
	if (Speed == B0)
	{
		printf ("Baud rate is not supported: %u\n", nBaud);
		return -1;
	}

	assert (ToolParam.pSerialPort);
	int nFile = open (ToolParam.pSerialPort, O_RDWR | O_NOCTTY | O_SYNC);
	if (nFile < 0)
	{
		printf ("Cannot open: %s: %s\n", ToolParam.pSerialPort, strerror (errno));
		return -1;
	}

	struct termios tty;
	if (tcgetattr (nFile, &tty) < 0)
	{
		printf ("tcgetattr failed: %s\n", strerror (errno));
		return -1;
	}

	cfsetospeed (&tty, Speed);
	cfsetispeed (&tty, Speed);

	tty.c_cflag |= (CLOCAL | CREAD);	/* ignore modem controls */
	tty.c_cflag &= ~CSIZE;
	tty.c_cflag |= CS8;			/* 8-bit characters */
	tty.c_cflag &= ~PARENB;			/* no parity bit */
	tty.c_cflag &= ~CSTOPB;			/* 1 stop bit */
	tty.c_cflag &= ~CRTSCTS;		/* no hardware flowcontrol */

	/* setup for non-canonical mode */
	tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
	tty.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
	tty.c_oflag &= ~OPOST;

	/* fetch bytes as they become available */
	tty.c_cc[VMIN] = 0;
	tty.c_cc[VTIME] = 0;

	if (tcsetattr (nFile, TCSANOW, &tty) != 0)
	{
		printf ("tcsetattr failed: %s\n", strerror (errno));
		return -1;
	}

	return nFile;
}

static void SerialClose (TSerialPort nFile)
{
	close (nFile);
}

static int SerialWrite (TSerialPort nFile, const void *pBuffer, size_t ulLength)
{
	int nResult = write (nFile, pBuffer, ulLength);
	if (nResult < 0)
	{
		printf ("Write failed: %s\n", strerror (errno));
	}

	return nResult;
}

static int SerialRead (TSerialPort nFile, void *pBuffer, size_t ulMaxLength)
{
	int nResult = read (nFile, pBuffer, ulMaxLength);
	if (nResult < 0)
	{
		printf ("Read failed: %s\n", strerror (errno));
		return -1;
	}

	if (   ToolParam.bMonitor
	    && nResult > 0)
	{
		write (2, pBuffer, nResult);
	}

	return nResult;
}

static void SerialFlush (TSerialPort nFile)
{
	tcdrain (nFile);
}

static void msDelay (unsigned nMillis)
{
	if (nMillis)
	{
		struct timespec Duration;
		Duration.tv_sec = nMillis / 1000;
		Duration.tv_nsec = (nMillis % 1000) * 1000000;

		nanosleep (&Duration, NULL);
	}
}

#else	// WIN32

/* Windows support */

#include <windows.h>

typedef HANDLE TSerialPort;

#define IS_VALID(port)		((port) != INVALID_HANDLE_VALUE)

/* Returns pointer to 0-terminated static buffer */
static const char *GetLastErrorMessage (void)
{
	static char Buffer[200];

	FormatMessage (FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		       NULL, GetLastError (), MAKELANGID (LANG_ENGLISH, SUBLANG_DEFAULT),
		       Buffer, sizeof Buffer-1, NULL);

	Buffer[sizeof Buffer-1] = '\0';

	return Buffer;
}

static TSerialPort SerialOpen (unsigned nBaud)
{
	char SerialPort[50];

	/* Convert "/dev/ttyS" to "COM" */
	static const char Prefix[] = "/dev/ttyS";
	assert (ToolParam.pSerialPort);
	if (strncmp (ToolParam.pSerialPort, Prefix, sizeof Prefix-1) == 0)
	{
		snprintf (SerialPort, sizeof SerialPort, "COM%s",
			  ToolParam.pSerialPort + sizeof Prefix-1);
	}
	else
	{
		strncpy (SerialPort, ToolParam.pSerialPort, sizeof SerialPort);
	}
	SerialPort[sizeof SerialPort-1] = '\0';

	HANDLE hFile = CreateFile (SerialPort,
				   GENERIC_READ | GENERIC_WRITE,
				   0, 0, OPEN_EXISTING, 0, 0);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		printf ("Cannot open: %s: %s\n", SerialPort, GetLastErrorMessage ());
		return hFile;
	}

	/* Modify parameters */
	DCB dcb = {0};
	dcb.DCBlength = sizeof (DCB);
	if (!GetCommState (hFile, &dcb))
	{
		printf ("GetCommState failed: %s\n", GetLastErrorMessage ());
		return INVALID_HANDLE_VALUE;
	}

	dcb.BaudRate = nBaud;
	dcb.ByteSize = 8;
	dcb.Parity   = NOPARITY;
	dcb.StopBits = ONESTOPBIT;

	if (!SetCommState (hFile, &dcb))
	{
		printf ("SetCommState failed: %s\n", GetLastErrorMessage());
		return INVALID_HANDLE_VALUE;
	}

	/* Set timeouts */
	COMMTIMEOUTS timeouts;
	timeouts.ReadIntervalTimeout = MAXDWORD;
	timeouts.ReadTotalTimeoutMultiplier = 0;
	timeouts.ReadTotalTimeoutConstant = 0;
	timeouts.WriteTotalTimeoutMultiplier = 0;
	timeouts.WriteTotalTimeoutConstant = 0;

	if (!SetCommTimeouts (hFile, &timeouts))
	{
		printf ("SetCommTimeouts failed: %s\n", GetLastErrorMessage ());
		return INVALID_HANDLE_VALUE;
	}

	return hFile;
}

static void SerialClose (TSerialPort hFile)
{
	CloseHandle (hFile);
}

static int SerialWrite (TSerialPort hFile, const void *pBuffer, size_t ulLength)
{
	DWORD dwBytesWritten;
	if (   !WriteFile (hFile, pBuffer, ulLength, &dwBytesWritten, NULL)
	    || dwBytesWritten != ulLength)
	{
		printf ("Write failed: %s\n", GetLastErrorMessage ());
		return -1;
	}

	return dwBytesWritten;
}

static int SerialRead (TSerialPort hFile, void *pBuffer, size_t ulMaxLength)
{
	DWORD dwBytesRead;
	if (!ReadFile (hFile, pBuffer, ulMaxLength, &dwBytesRead, NULL))
	{
		printf ("Read failed: %s\n", GetLastErrorMessage ());
		return -1;
	}

	if (   ToolParam.bMonitor
	    && dwBytesRead > 0)
	{
		write (2, pBuffer, dwBytesRead);
	}

	return dwBytesRead;
}

static void SerialFlush (TSerialPort hFile)
{
	FlushFileBuffers (hFile);
}

static void msDelay (unsigned nMillis)
{
	if (nMillis)
	{
		Sleep (nMillis);
	}
}

#endif	// WIN32

/*
 * Read from serial and match against pattern
 *
 * Returns pointer to 0-terminated static buffer,
 * Terminates the program on read error
 */
static char *SerialReadAndMatch (TSerialPort Serial, const char *pPattern)
{
	static char Buffer[100];

	int nResult = SerialRead (Serial, Buffer, sizeof Buffer-1);
	if (nResult > 0)
	{
		Buffer[nResult] = '\0';

		assert (pPattern);
		char *p = strstr (Buffer, pPattern);
		if (p)
		{
			return p;
		}
	}
	else if (nResult < 0)
	{
		exit (1);
	}

	return NULL;
}

/* Hexfile scanner and converter to fast-mode format */

static int GetNextByte (FILE *pFile, size_t *pulOffset)
{
	assert (pFile);
	assert (pulOffset);

	int nChar;

	do
	{
		nChar = fgetc (pFile);
		if (nChar == EOF)
		{
			return EOF;
		}

		(*pulOffset)++;
	}
	while (isspace (nChar));

	if (nChar == ':')
	{
		return '=';
	}

	int nByte = nChar < 'A' ? nChar - '0' : nChar - 'A' + 10;
	nByte <<= 4;

	nChar = fgetc (pFile);
	if (nChar == EOF)
	{
		return EOF;
	}

	(*pulOffset)++;

	nByte |= nChar < 'A' ? nChar - '0' : nChar - 'A' + 10;

	return nByte;
}

/* Main */

int main (int nArgC, char **ppArgV)
{
	assert (nArgC > 0);
	const char *pArg0 = *ppArgV++;
	nArgC--;

	if (nArgC == 0)
	{
		printf (Usage);
		return 1;
	}

	/* Parse command line */

	ToolParam.pSerialPort = *ppArgV++;
	nArgC--;

	if (*ToolParam.pSerialPort == '-')
	{
		printf ("%s: Serial port expected: %s\n", pArg0, ToolParam.pSerialPort);
		return 1;
	}

	while (nArgC > 0)
	{
		const char *pOption = *ppArgV++;
		nArgC--;

		if (pOption[0] == '-' && pOption[1] == '-')
		{
			pOption += 2;

			char *pParam = strchr (pOption, ':');
			char *pEnd = NULL;
			unsigned long ulValue = 0;
			if (pParam)
			{
				*pParam++ = '\0';

				ulValue = strtoul (pParam, &pEnd, 10);
			}

			if (strcasecmp (pOption, "flashbaud") == 0)
			{
				if (   (pEnd && *pEnd != '\0')
				    || *pParam == '\0'
				    || ulValue > 4000000)
				{
					printf ("%s: Invalid baud rate: %s\n", pArg0, pParam);
					return 1;
				}

				ToolParam.nFlashBaud = (unsigned) ulValue;
			}
			else if (strcasecmp (pOption, "userbaud") == 0)
			{
				if (   (pEnd && *pEnd != '\0')
				    || *pParam == '\0'
				    || ulValue > 4000000)
				{
					printf ("%s: Invalid baud rate: %s\n", pArg0, pParam);
					return 1;
				}

				ToolParam.nUserBaud = (unsigned) ulValue;
			}
			else if (strcasecmp (pOption, "godelay") == 0)
			{
				if (   (pEnd && *pEnd != '\0')
				    || *pParam == '\0'
				    || ulValue > 10000)
				{
					printf ("%s: Invalid delay: %s\n", pArg0, pParam);
					return 1;
				}

				ToolParam.nGoDelay = (unsigned) ulValue;
			}
			else if (strcasecmp (pOption, "reboot") == 0)
			{
				if (!pParam)
				{
					printf ("%s: Reboot magic expected\n", pArg0);
					return 1;
				}

				if (*pParam)
				{
					ToolParam.pRebootMagic = pParam;
				}
			}
			else if (strcasecmp (pOption, "rebootdelay") == 0)
			{
				if (   (pEnd && *pEnd != '\0')
				    || *pParam == '\0'
				    || ulValue > 20000)
				{
					printf ("%s: Invalid delay: %s\n", pArg0, pParam);
					return 1;
				}

				ToolParam.nRebootDelay = (unsigned) ulValue;
			}
			else if (strcasecmp (pOption, "packetsize") == 0)
			{
				if (   (pEnd && *pEnd != '\0')
				    || *pParam == '\0'
				    || ulValue > 1000000)
				{
					printf ("%s: Invalid size: %s\n", pArg0, pParam);
					return 1;
				}

				ToolParam.nPacketSize = (unsigned) ulValue;
			}
			else if (strcasecmp (pOption, "monitor") == 0)
			{
				ToolParam.bMonitor = 1;
			}
			else
			{
				printf ("%s: Invalid option: --%s\n", pArg0, pOption);
				return 1;
			}
		}
		else
		{
			if (ToolParam.pHexFile)
			{
				printf ("%s: Too many parameters: %s\n", pArg0, pOption);
				return 1;
			}

			if (pOption[0] == '\0')
			{
				printf ("%s: Hexfile expected\n", pArg0);
				return 1;
			}

			ToolParam.pHexFile = pOption;
		}
	}

	/* Write magic reboot string */

	if (ToolParam.pRebootMagic)
	{
		TSerialPort Serial = SerialOpen (ToolParam.nUserBaud);
		if (!IS_VALID (Serial))
		{
			return 1;
		}

		if (SerialWrite (Serial, ToolParam.pRebootMagic,
				 strlen (ToolParam.pRebootMagic)) < 0)
		{
			SerialClose (Serial);
			return 1;
		}

		SerialFlush (Serial);

		SerialClose (Serial);

		msDelay (ToolParam.nRebootDelay);
	}

	/* Write reset command */

	TSerialPort Serial = SerialOpen (ToolParam.nFlashBaud);
	if (!IS_VALID (Serial))
	{
		return 1;
	}

	unsigned nTries = 0;
	do
	{
		/*
		 * Reset signal consists of 256 x 0x80 chars, followed
		 * by a reset 'R' command.  The idea here is the 0x80s will
		 * flush a previously canceled flash out of a binary
		 * record state.  0x80 is used in case the bootloader is
		 * cancelled at the start of a binary record and we don't
		 * want to write a lo-memory address that will trash the
		 * bootloader itself.
		*/
		char Buffer[257];
		memset (Buffer, '\x80', sizeof Buffer-1);
		Buffer[sizeof Buffer-1] = 'R';
		if (SerialWrite (Serial, Buffer, sizeof Buffer) < 0)
		{
			return 1;
		}

		SerialFlush (Serial);

		msDelay (100);

		if (++nTries == 10)
		{
			printf ("Waiting for device ... ");
			fflush (stdout);
		}
		else if (nTries == 150 && !ToolParam.bMonitor)
		{
			printf ("timeout\n");
			SerialClose (Serial);
			return 1;
		}
	}
	while (!SerialReadAndMatch (Serial, "IHEX-F"));

	if (nTries >= 10)
	{
		printf ("\r                      \r");
		fflush (stdout);
	}

	/* Upload hexfile */

	if (ToolParam.pHexFile)
	{
		FILE *pFile = fopen (ToolParam.pHexFile, "rt");
		if (!pFile)
		{
			printf ("%s: Cannot open: %s\n", pArg0, ToolParam.pHexFile);
			SerialClose (Serial);
			return 1;
		}

		struct stat StatBuf;
		if (fstat (fileno (pFile), &StatBuf) < 0)
		{
			printf ("%s: Cannot stat: %s\n", pArg0, ToolParam.pHexFile);
			fclose (pFile);
			SerialClose (Serial);
			return 1;
		}

		size_t ulFileSize = StatBuf.st_size;

		size_t ulPacketSize = ToolParam.nPacketSize;
		if (!ulPacketSize)
		{
			/* 1 packet takes 2 seconds by default, 10 bits per byte */
			ulPacketSize = ToolParam.nFlashBaud * 2 / 10;
		}

		char *pBuffer = malloc (ulPacketSize);
		if (!pBuffer)
		{
			printf ("%s: Not enough memory: %s\n", pArg0, strerror (errno));
			fclose (pFile);
			SerialClose (Serial);
			return 1;
		}

		size_t ulOffset = 0;
		for (int bContinue = 1; bContinue;)
		{
			char *pErr = SerialReadAndMatch (Serial, "#ERR:");
			if (pErr)
			{
				pErr += 5;

				char *pErrEnd = strchr (pErr, '\r');
				if (pErrEnd)
				{
					*pErrEnd = '\0';
				}

				printf ("%s: Transfer error: %s\n", pArg0, pErr);
				free (pBuffer);
				fclose (pFile);
				SerialClose (Serial);
				return 1;
			}

			unsigned nPercent = ulOffset * 100 / ulFileSize;
			printf ("\rUploading ... %u%%", nPercent);
			fflush (stdout);

			size_t ulRemain = 0;
			for (unsigned i = 0; i < ulPacketSize; i++)
			{
				int nByte = GetNextByte (pFile, &ulOffset);
				if (nByte == EOF)
				{
					bContinue = 0;

					break;
				}

				pBuffer[i] = (char) nByte;
				ulRemain++;
			}

			if (!ulRemain)
			{
				break;
			}

			if (SerialWrite (Serial, pBuffer, ulRemain) != (int) ulRemain)
			{
				free (pBuffer);
				fclose (pFile);
				SerialClose (Serial);
				return 1;
			}

			SerialFlush (Serial);
		}

		free (pBuffer);

		fclose (pFile);

		printf ("\r                  \r");
		fflush (stdout);

		msDelay (100);

		if (!SerialReadAndMatch (Serial, "#EOF:ok"))
		{
			printf ("%s: Upload failed\n", pArg0);
			SerialClose (Serial);
			return 1;
		}
	}

	/* Set go delay */

	if (ToolParam.nGoDelay)
	{
		char Buffer[20];
		assert (ToolParam.nGoDelay <= 10000);
		sprintf (Buffer, "s%X\n", ToolParam.nGoDelay * 1000);

		if (SerialWrite (Serial, Buffer, strlen (Buffer)) < 0)
		{
			SerialClose (Serial);
			return 1;
		}
	}

	/* Send GO */

	if (SerialWrite (Serial, "g", 1) < 0)
	{
		SerialClose (Serial);
		return 1;
	}

	msDelay (100);

	if (!SerialReadAndMatch (Serial, "--"))
	{
		printf ("%s: Go failed\n", pArg0);
	}
	else
	{
		printf ("%s successfully started\n",
			ToolParam.pHexFile ? ToolParam.pHexFile : "Device");
	}

	SerialClose (Serial);

	return 0;
}
