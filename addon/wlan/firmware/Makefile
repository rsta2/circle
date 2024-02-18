#
# Makefile
#

# From: Jan 25 2021
FIRMWARE ?= f713a6054746bc61ece1c8696dce91a7b7e22dd9

# From: Jan 17 2024
FIRMWARE5 ?= 3db4164cfd89e6d9afb7ebc87607b792651512df

BASEURL = https://github.com/RPi-Distro/firmware-nonfree/raw/$(FIRMWARE)
BASEURL5 = https://github.com/RPi-Distro/firmware-nonfree/raw/$(FIRMWARE5)

all: clean firmware firmware5

firmware:
	wget -q -O LICENCE.broadcom_bcm43xx $(BASEURL)/LICENCE.broadcom_bcm43xx
	wget -q -O brcmfmac43430-sdio.bin $(BASEURL)/brcm/brcmfmac43430-sdio.bin
	wget -q -O brcmfmac43430-sdio.txt $(BASEURL)/brcm/brcmfmac43430-sdio.txt
	wget -q -O brcmfmac43436-sdio.bin $(BASEURL)/brcm/brcmfmac43436-sdio.bin
	wget -q -O brcmfmac43436-sdio.txt $(BASEURL)/brcm/brcmfmac43436-sdio.txt
	wget -q -O brcmfmac43436-sdio.clm_blob $(BASEURL)/brcm/brcmfmac43436-sdio.clm_blob
	wget -q -O brcmfmac43455-sdio.bin $(BASEURL)/brcm/brcmfmac43455-sdio.bin
	wget -q -O brcmfmac43455-sdio.txt $(BASEURL)/brcm/brcmfmac43455-sdio.txt
	wget -q -O brcmfmac43455-sdio.clm_blob $(BASEURL)/brcm/brcmfmac43455-sdio.clm_blob
	wget -q -O brcmfmac43456-sdio.bin $(BASEURL)/brcm/brcmfmac43456-sdio.bin
	wget -q -O brcmfmac43456-sdio.txt $(BASEURL)/brcm/brcmfmac43456-sdio.txt
	wget -q -O brcmfmac43456-sdio.clm_blob $(BASEURL)/brcm/brcmfmac43456-sdio.clm_blob

firmware5:
	wget -q -O brcmfmac43455-sdio.raspberrypi,5-model-b.bin $(BASEURL5)/debian/config/brcm80211/cypress/cyfmac43455-sdio-standard.bin
	wget -q -O brcmfmac43455-sdio.raspberrypi,5-model-b.txt $(BASEURL5)/debian/config/brcm80211/brcm/brcmfmac43455-sdio.txt
	wget -q -O brcmfmac43455-sdio.raspberrypi,5-model-b.clm_blob $(BASEURL5)/debian/config/brcm80211/cypress/cyfmac43455-sdio.clm_blob

clean:
	rm -f *.bin *.txt *.clm_blob LICENCE*
