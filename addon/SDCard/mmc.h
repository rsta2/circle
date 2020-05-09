#ifndef _SDCard_mmc_h
#define _SDCard_mmc_h

#include <circle/types.h>

struct sg_mapping_iter
{
#define SG_MITER_ATOMIC		(1 << 0)
#define SG_MITER_TO_SG		(1 << 1)
#define SG_MITER_FROM_SG	(1 << 2)
	void		*addr;
	size_t		length;
	size_t		consumed;
};

struct mmc_ios
{
	unsigned	clock;
	unsigned char	bus_width;
#define MMC_BUS_WIDTH_4		(1 << 0)
	unsigned char	power_mode;
	unsigned char	timing;
	unsigned char	signal_voltage;
	unsigned char	drv_type;
};

struct mmc_host
{
	u32		caps;
#define MMC_CAP_4_BIT_DATA	(1 << 0)
#define MMC_CAP_SD_HIGHSPEED	(1 << 1)
#define MMC_CAP_MMC_HIGHSPEED	(1 << 2)
#define MMC_CAP_NEEDS_POLL	(1 << 3)
#define MMC_CAP_HW_RESET	(1 << 4)
#define MMC_CAP_ERASE		(1 << 5)
#define MMC_CAP_CMD23		(1 << 6)
	unsigned	f_min;
	unsigned	f_max;
	unsigned	actual_clock;
	unsigned	max_busy_timeout;
	unsigned short	max_segs;
	unsigned	max_seg_size;
	unsigned	max_req_size;
	unsigned	max_blk_size;
	unsigned	max_blk_count;
	u32		ocr_avail;
#define MMC_VDD_32_33		(1 << 20)
#define MMC_VDD_33_34		(1 << 21)
	mmc_ios		ios;
};

struct mmc_command;
struct mmc_request;

struct mmc_data
{
	u32		flags;
#define MMC_DATA_READ		(1 << 0)
#define MMC_DATA_WRITE		(1 << 1)
	unsigned	blksz;
	unsigned	blocks;
	void		*sg;
	unsigned	sg_len;
	unsigned	bytes_xfered;
	int		error;
	mmc_command	*stop;
	mmc_request	*mrq;
};

struct mmc_command
{
	u32		opcode;
#define MMC_SEND_OP_COND	1
#define SD_IO_SEND_OP_COND	5
#define MMC_STOP_TRANSMISSION   12
#define MMC_SEND_STATUS		13
#define MMC_READ_MULTIPLE_BLOCK 18
#define MMC_WRITE_MULTIPLE_BLOCK 25
#define SD_APP_OP_COND		41
	u32		arg;
	u32		flags;
#define MMC_RSP_PRESENT		(1 << 0)
#define MMC_RSP_136		(1 << 1)
#define MMC_RSP_CRC		(1 << 2)
#define MMC_RSP_BUSY		(1 << 3)
	u32		resp[4];
	unsigned	retries;
	int		error;
	mmc_data	*data;
};

struct mmc_request
{
	mmc_command	*sbc;
	mmc_command	*cmd;
	mmc_data	*data;
	mmc_command	*stop;
	volatile int	done;
};

void mmc_request_done (mmc_host *mmc, mmc_request *mrq);

#endif
