#
# Makefile
#

FIRMWARE ?= 00daf85ffa373ecce7836df7543c6ebe4cf43639

BASEURL = https://github.com/RPi-Distro/firmware-nonfree/blob/$(FIRMWARE)

firmware:
	wget -q -O LICENCE.broadcom_bcm43xx $(BASEURL)/LICENCE.broadcom_bcm43xx?raw=true
	wget -q -O brcmfmac43430-sdio.bin $(BASEURL)/brcm/brcmfmac43430-sdio.bin?raw=true
	wget -q -O brcmfmac43430-sdio.txt $(BASEURL)/brcm/brcmfmac43430-sdio.txt?raw=true
	wget -q -O brcmfmac43455-sdio.bin $(BASEURL)/brcm/brcmfmac43455-sdio.bin?raw=true
	wget -q -O brcmfmac43455-sdio.txt $(BASEURL)/brcm/brcmfmac43455-sdio.txt?raw=true
	wget -q -O brcmfmac43455-sdio.clm_blob $(BASEURL)/brcm/brcmfmac43455-sdio.clm_blob?raw=true

clean:
	rm -f *.bin *.txt *.clm_blob LICENCE*
