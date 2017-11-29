#include <linux/raspberrypi-firmware.h>
#include <linux/errno.h>
#include <circle/bcmpropertytags.h>
#include <circle/util.h>
#include <circle/types.h>

struct TPropertyTagGeneric
{
	TPropertyTag	Tag;
	u32		Data[0];
};

int rpi_firmware_property (struct rpi_firmware *fw, u32 tag, void *data, size_t len)
{
	if (len > 2048)
	{
		return -ENOMEM;
	}

	u8 Buffer[sizeof (TPropertyTagGeneric) + len];
	TPropertyTagGeneric *pTag = reinterpret_cast<TPropertyTagGeneric *> (Buffer);

	memcpy (pTag->Data, data, len);

	CBcmPropertyTags Tags;
	if (!Tags.GetTag (tag, pTag, sizeof (TPropertyTagGeneric) + len))
	{
		return -ENXIO;
	}

	memcpy (data, pTag->Data, len);

	return 0;
}
