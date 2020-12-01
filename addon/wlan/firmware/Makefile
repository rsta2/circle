#
# Makefile
#

# From: Nov 27 2020
FIRMWARE ?= 1f85a05b7ef063291e7a3dd1d735df4cbc441a74

BASEURL = https://github.com/RPi-Distro/firmware-nonfree/blob/$(FIRMWARE)

firmware: clean
	wget -q -O LICENCE.broadcom_bcm43xx $(BASEURL)/LICENCE.broadcom_bcm43xx?raw=true
	wget -q -O brcmfmac43430-sdio.bin $(BASEURL)/brcm/brcmfmac43430-sdio.bin?raw=true
	wget -q -O brcmfmac43430-sdio.txt $(BASEURL)/brcm/brcmfmac43430-sdio.txt?raw=true
	wget -q -O brcmfmac43455-sdio.bin $(BASEURL)/brcm/brcmfmac43455-sdio.bin?raw=true
	wget -q -O brcmfmac43455-sdio.txt $(BASEURL)/brcm/brcmfmac43455-sdio.txt?raw=true
	wget -q -O brcmfmac43455-sdio.clm_blob $(BASEURL)/brcm/brcmfmac43455-sdio.clm_blob?raw=true
	wget -q -O brcmfmac43456-sdio.bin $(BASEURL)/brcm/brcmfmac43456-sdio.bin?raw=true
	wget -q -O brcmfmac43456-sdio.txt $(BASEURL)/brcm/brcmfmac43456-sdio.txt?raw=true
	wget -q -O brcmfmac43456-sdio.clm_blob $(BASEURL)/brcm/brcmfmac43456-sdio.clm_blob?raw=true

clean:
	rm -f *.bin *.txt *.clm_blob LICENCE*
