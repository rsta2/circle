/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2019        */
/* Implementation for Circle by R. Stange <rsta2@o2online.de>            */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include "ff.h"			/* Obtains integer types */
#include "diskio.h"		/* Declarations of disk functions */
#include <circle/device.h>
#include <circle/devicenameservice.h>
#include <circle/util.h>
#include <circle/types.h>
#include <assert.h>

#if FF_MIN_SS != FF_MAX_SS
	#error FF_MIN_SS != FF_MAX_SS is not supported!
#endif
#define SECTOR_SIZE		FF_MIN_SS

/*-----------------------------------------------------------------------*/
/* Static Data                                                           */
/*-----------------------------------------------------------------------*/

static const char *s_pVolumeName[FF_VOLUMES] =
{
	"emmc1",
	"umsd1",
	"umsd2",
	"umsd3",
};

static CDevice *s_pVolume[FF_VOLUMES] = {0};

static u8 *s_pBuffer = 0;
static unsigned s_nBufferSize = 0;



/*-----------------------------------------------------------------------*/
/* Callbacks                                                             */
/*-----------------------------------------------------------------------*/

static void disk_removed (
	CDevice *pDevice,	/* device removed */
	void *pContext
)
{
	*((CDevice **) pContext) = 0;
}



/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
	BYTE pdrv		/* Physical drive nmuber to identify the drive */
)
{
	if (   pdrv < FF_VOLUMES
	    && s_pVolume[pdrv] != 0)
	{
		return 0;
	}

	return STA_NOINIT;
}



/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
	BYTE pdrv				/* Physical drive nmuber to identify the drive */
)
{
	if (pdrv >= FF_VOLUMES)
	{
		return STA_NOINIT;
	}

	s_pVolume[pdrv] = CDeviceNameService::Get ()->GetDevice (s_pVolumeName[pdrv], TRUE);
	if (s_pVolume[pdrv] != 0)
	{
		s_pVolume[pdrv]->RegisterRemovedHandler (disk_removed, &s_pVolume[pdrv]);

		return 0;
	}

	return STA_NOINIT;
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
	BYTE pdrv,		/* Physical drive nmuber to identify the drive */
	BYTE *buff,		/* Data buffer to store read data */
	LBA_t sector,	/* Start sector in LBA */
	UINT count		/* Number of sectors to read */
)
{
	if (pdrv >= FF_VOLUMES)
	{
		return RES_PARERR;
	}

	CDevice *pDevice = s_pVolume[pdrv];
	if (pDevice == 0)
	{
		return RES_NOTRDY;
	}

	/* Ensure that the transfer buffer is word aligned */
	BYTE *pBuffer = buff;
	unsigned nSize = count * SECTOR_SIZE;
	if (((uintptr) pBuffer & 3) != 0)
	{
		if (s_nBufferSize < nSize)
		{
			delete [] s_pBuffer;

			s_nBufferSize = nSize;

			s_pBuffer = new u8[s_nBufferSize];
			assert (s_pBuffer != 0);
		}

		pBuffer = s_pBuffer;
	}

	QWORD offset = sector;
	offset *= SECTOR_SIZE;
	pDevice->Seek (offset);

	if (pDevice->Read (pBuffer, nSize) < 0)
	{
		return RES_ERROR;
	}

	if (pBuffer != buff)
	{
		memcpy (buff, pBuffer, nSize);
	}

	return RES_OK;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if FF_FS_READONLY == 0

DRESULT disk_write (
	BYTE pdrv,			/* Physical drive nmuber to identify the drive */
	const BYTE *buff,	/* Data to be written */
	LBA_t sector,		/* Start sector in LBA */
	UINT count			/* Number of sectors to write */
)
{
	if (pdrv >= FF_VOLUMES)
	{
		return RES_PARERR;
	}

	CDevice *pDevice = s_pVolume[pdrv];
	if (pDevice == 0)
	{
		return RES_NOTRDY;
	}

	/* Ensure that the transfer buffer is word aligned */
	const BYTE *pBuffer = buff;
	unsigned nSize = count * SECTOR_SIZE;
	if (((uintptr) pBuffer & 3) != 0)
	{
		if (s_nBufferSize < nSize)
		{
			delete [] s_pBuffer;

			s_nBufferSize = nSize;

			s_pBuffer = new u8[s_nBufferSize];
			assert (s_pBuffer != 0);
		}

		memcpy (s_pBuffer, buff, nSize);

		pBuffer = s_pBuffer;
	}

	QWORD offset = sector;
	offset *= SECTOR_SIZE;
	pDevice->Seek (offset);

	if (pDevice->Write (pBuffer, nSize) < 0)
	{
		return RES_ERROR;
	}

	return RES_OK;
}

#endif


/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl (
	BYTE pdrv,		/* Physical drive nmuber (0..) */
	BYTE cmd,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
	switch (cmd)
	{
	case GET_SECTOR_COUNT:
		{
			if (pdrv >= FF_VOLUMES)
			{
				return RES_PARERR;
			}

			CDevice *pDevice =
				CDeviceNameService::Get ()->GetDevice (s_pVolumeName[pdrv], TRUE);
			if (pDevice != 0)
			{
				u64 ullSize = pDevice->GetSize ();
				if (ullSize == (u64) -1)
				{
					return RES_PARERR;
				}

				*(LBA_t *) buff = (LBA_t) (ullSize / SECTOR_SIZE);
			}
			else
			{
				return RES_NOTRDY;
			}
		}
		return RES_OK;

	case CTRL_SYNC:
		return RES_OK;

	case GET_SECTOR_SIZE:
		assert (buff != 0);
		*(WORD *) buff = SECTOR_SIZE;
		return RES_OK;

	case CTRL_EJECT:
		if (pdrv >= FF_VOLUMES)
		{
			return RES_PARERR;
		}

		if (s_pVolume[pdrv] == 0)
		{
			s_pVolume[pdrv] = CDeviceNameService::Get ()->GetDevice (s_pVolumeName[pdrv], TRUE);
			if (s_pVolume[pdrv] == 0)
			{
				return RES_NOTRDY;
			}
		}

		if (!s_pVolume[pdrv]->RemoveDevice ())
		{
			return RES_ERROR;
		}

		s_pVolume[pdrv] = 0;

		return RES_OK;
	}

	return RES_PARERR;
}

