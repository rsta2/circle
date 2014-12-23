//
// fatfsdef.h
//
#ifndef _circle_fs_fat_fatfsdef_h
#define _circle_fs_fat_fatfsdef_h

#include <circle/types.h>
#include <circle/macros.h>

#define FAT_SECTOR_SIZE		512

#define FAT_BUFFERS		100
#define FAT_FILES		40

#define FAT_MAX_FILESIZE	0xFFFFFFFF

struct TFATBPBStruct
{
	u8	Jump[3];		// ignored
	u8	OEMName[8];		// ignored
	u16	nBytesPerSector;	// usually 512, can be: 512, 1024, 2048 or 4096
	u8	nSectorsPerCluster;	// 1, 2, 4, 8, 16, 32, 64 or 128
	u16	nReservedSectors;	// must not be 0
	u8	nNumberOfFATs;		// should be 2
	u16	nRootEntries;		// 0 for FAT32
	u16	nTotalSectors16;	// if 0 nTotalSectors32 must be set
	u8	nMedia;			// FAT starts with this byte
	u16	nFATSize16;		// 0 for FAT32
	u16	nSectorsPerTrack;	// ignored
	u16	nNumberOfHeads;		// ignored
	u32	nHiddenSectors;		// ignored
	u32	nTotalSectors32;	// if 0 nTotalSectors16 must be set
}
PACKED;

struct TFAT1xStruct
{
	u8	nDriveNumber;		// ignored
	u8	nReserved;		// ignored
	u8	nBootSignature;		// must be 0x29
	u32	nVolumeSerial;		// ignored
	u8	VolumeLabel[11];	// ignored
	u8	FileSystemType[8];	// ignored
}
PACKED;

struct TFAT32Struct
{
	u32	nFATSize32;
	u16	nActiveFAT	: 4,	// zero based, only if FAT mirroring is off
		nReserved1	: 3,	// ignored
		nMirroringOff	: 1,	// 1: only one active FAT
		nReserved2	: 8;	// ignored
	u16	nFSVersion;		// must be 0
	u32	nRootCluster;		// 1st cluster of root directory, usually 2
	u16	nFSInfoSector;		// usually 1
	u16	nBackupBootSector;	// ignored
	u8	nReserved3[12];		// ignored
	u8	nDriveNumber;		// ignored
	u8	nReserved4;		// ignored
	u8	nBootSignature;		// must be 0x29
	u32	nVolumeSerial;		// ignored
	u8	VolumeLabel[11];	// ignored
	u8	FileSystemType[8];	// ignored
}
PACKED;

struct TFATBootSector
{
	TFATBPBStruct	BPB;

	union
	{
		TFAT1xStruct	FAT1x;
		TFAT32Struct	FAT32;
	}
	Struct;

	u8	BootCode[420];
	u16	nBootSignature;
	#define BOOT_SIGNATURE		0xAA55
}
PACKED;

struct TFAT32FSInfoSector
{
	u32	nLeadingSignature;
	#define LEADING_SIGNATURE	0x41615252
	u8	Reserved1[480];		// ignored
	u32	nStructSignature;
	#define STRUCT_SIGNATURE	0x61417272
	u32	nFreeCount;		// <= volume cluster count or unknown
	#define FREE_COUNT_UNKNOWN	0xFFFFFFFF
	u32	nNextFreeCluster;	// hint to look for free cluster or unknown
	#define NEXT_FREE_CLUSTER_UNKNOWN 0xFFFFFFFF
	u8	Reserved2[12];		// ignored
	u32	nTrailingSignature;
	#define TRAILING_SIGNATURE	0xAA550000
}
PACKED;

struct TFATDirectoryEntry
{
	#define FAT_DIR_NAME_LENGTH	11
	u8	Name[FAT_DIR_NAME_LENGTH];	// name and extension padded by ' '
	#define FAT_DIR_NAME0_LAST	0x00
	#define FAT_DIR_NAME0_FREE	0xE5
	#define FAT_DIR_NAME0_KANJI_E5	0x05
	u8	nAttributes;
	#define FAT_DIR_ATTR_READ_ONLY	0x01
	#define FAT_DIR_ATTR_HIDDEN	0x02
	#define FAT_DIR_ATTR_SYSTEM	0x04
	#define FAT_DIR_ATTR_VOLUME_ID	0x08
	#define FAT_DIR_ATTR_DIRECTORY	0x10
	#define FAT_DIR_ATTR_ARCHIVE	0x20
	#define FAT_DIR_ATTR_LONG_NAME	0x0F
	#define FAT_DIR_ATTR_LONG_NAME_MASK 0x3F
	u8	nNTReserved;		// set to 0 on file create
	u8	nCreatedTimeTenth;	// 0..199 = 0..1.99
	u16	nCreatedTime;		// HHHHHMMMMMMSSSSS (0..23, 0..59, 0..29 = 0..58)
	u16	nCreatedDate;		// YYYYYYYMMMMDDDDD (0..127 = 1980..2107, 1..12, 1..31)
	u16	nLastAccessDate;	// YYYYYYYMMMMDDDDD (0..127 = 1980..2107, 1..12, 1..31)
	u16	nFirstClusterHigh;
	u16	nWriteTime;		// HHHHHMMMMMMSSSSS (0..23, 0..59, 0..29 = 0..58)
	u16	nWriteDate;		// YYYYYYYMMMMDDDDD (0..127 = 1980..2107, 1..12, 1..31)
	u16	nFirstClusterLow;
	u32	nFileSize;
}
PACKED;

#define FAT_DIR_ENTRY_SIZE		32
#define FAT_DIR_ENTRIES_PER_SECTOR	(FAT_SECTOR_SIZE / FAT_DIR_ENTRY_SIZE)

#endif
