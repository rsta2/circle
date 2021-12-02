#
# Makefile
#

# From: Jan 25 2021
FIRMWARE ?= f713a6054746bc61ece1c8696dce91a7b7e22dd9

BASEURL = https://github.com/RPi-Distro/firmware-nonfree/blob/$(FIRMWARE)

firmware: clean
	wget -q -O LICENCE.broadcom_bcm43xx $(BASEURL)/LICENCE.broadcom_bcm43xx?raw=true
	wget -q -O brcmfmac43430-sdio.bin $(BASEURL)/brcm/brcmfmac43430-sdio.bin?raw=true
	wget -q -O brcmfmac43430-sdio.txt $(BASEURL)/brcm/brcmfmac43430-sdio.txt?raw=true
	wget -q -O brcmfmac43436-sdio.bin $(BASEURL)/brcm/brcmfmac43436-sdio.bin?raw=true
	wget -q -O brcmfmac43436-sdio.txt $(BASEURL)/brcm/brcmfmac43436-sdio.txt?raw=true
	wget -q -O brcmfmac43436-sdio.clm_blob $(BASEURL)/brcm/brcmfmac43436-sdio.clm_blob?raw=true
	wget -q -O brcmfmac43455-sdio.bin $(BASEURL)/brcm/brcmfmac43455-sdio.bin?raw=true
	wget -q -O brcmfmac43455-sdio.txt $(BASEURL)/brcm/brcmfmac43455-sdio.txt?raw=true
	wget -q -O brcmfmac43455-sdio.clm_blob $(BASEURL)/brcm/brcmfmac43455-sdio.clm_blob?raw=true
	wget -q -O brcmfmac43456-sdio.bin $(BASEURL)/brcm/brcmfmac43456-sdio.bin?raw=true
	wget -q -O brcmfmac43456-sdio.txt $(BASEURL)/brcm/brcmfmac43456-sdio.txt?raw=true
	wget -q -O brcmfmac43456-sdio.clm_blob $(BASEURL)/brcm/brcmfmac43456-sdio.clm_blob?raw=true

clean:
	rm -f *.bin *.txt *.clm_blob LICENCE*
