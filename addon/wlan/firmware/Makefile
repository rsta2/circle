#
# Makefile
#

# From: Apr  8 2025
FIRMWARE ?= c9d3ae6584ab79d19a4f94ccf701e888f9f87a53

BASEURL = https://github.com/RPi-Distro/firmware-nonfree/raw/$(FIRMWARE)/debian/config/brcm80211

all: clean firmware firmware5

firmware:
	wget -q -O brcmfmac43430-sdio.bin $(BASEURL)/cypress/cyfmac43430-sdio.bin
	wget -q -O brcmfmac43430-sdio.txt $(BASEURL)/brcm/brcmfmac43430-sdio.txt
	wget -q -O brcmfmac43430-sdio.clm_blob $(BASEURL)/cypress/cyfmac43430-sdio.clm_blob
	wget -q -O brcmfmac43436-sdio.bin $(BASEURL)/brcm/brcmfmac43436-sdio.bin
	wget -q -O brcmfmac43436-sdio.txt $(BASEURL)/brcm/brcmfmac43436-sdio.txt
	wget -q -O brcmfmac43436-sdio.clm_blob $(BASEURL)/brcm/brcmfmac43436-sdio.clm_blob
	wget -q -O brcmfmac43436s-sdio.bin $(BASEURL)/brcm/brcmfmac43436s-sdio.bin
	wget -q -O brcmfmac43436s-sdio.txt $(BASEURL)/brcm/brcmfmac43436s-sdio.txt
	wget -q -O brcmfmac43455-sdio.bin $(BASEURL)/cypress/cyfmac43455-sdio-minimal.bin
	wget -q -O brcmfmac43455-sdio.txt $(BASEURL)/brcm/brcmfmac43455-sdio.txt
	wget -q -O brcmfmac43455-sdio.clm_blob $(BASEURL)/cypress/cyfmac43455-sdio.clm_blob
	wget -q -O brcmfmac43456-sdio.bin $(BASEURL)/brcm/brcmfmac43456-sdio.bin
	wget -q -O brcmfmac43456-sdio.txt $(BASEURL)/brcm/brcmfmac43456-sdio.txt
	wget -q -O brcmfmac43456-sdio.clm_blob $(BASEURL)/brcm/brcmfmac43456-sdio.clm_blob

firmware5:
	wget -q -O brcmfmac43455-sdio.raspberrypi,5-model-b.bin $(BASEURL)/cypress/cyfmac43455-sdio-minimal.bin
	wget -q -O brcmfmac43455-sdio.raspberrypi,5-model-b.txt $(BASEURL)/brcm/brcmfmac43455-sdio.txt
	wget -q -O brcmfmac43455-sdio.raspberrypi,5-model-b.clm_blob $(BASEURL)/cypress/cyfmac43455-sdio.clm_blob

clean:
	rm -f *.bin *.txt *.clm_blob
