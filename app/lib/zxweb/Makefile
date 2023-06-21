#
# Makefile
#

CIRCLEHOME = ../../..

OBJS	= zxweb.o

libzxweb.a: $(OBJS)
	@echo "  AR    $@"
	@rm -f $@
	@$(AR) cr $@ $(OBJS)

CONTENT	= index.h style.h ledoff.h ledon.h favicon.h

EXTRACLEAN = $(CONTENT)          


include $(CIRCLEHOME)/Rules.mk

# Webserver content

%.h: %.html
	@echo "  GEN   $@"
	@$(CIRCLEHOME)/tools/converttool $< > $@

%.h: %.css
	@echo "  GEN   $@"
	@$(CIRCLEHOME)/tools/converttool $< > $@

%.h: %.png
	@echo "  GEN   $@"
	@$(CIRCLEHOME)/tools/converttool -b $< > $@

%.h: %.ico
	@echo "  GEN   $@"
	@$(CIRCLEHOME)/tools/converttool -b $< > $@

zxweb.o: $(CONTENT)

# End Webserver content

-include $(DEPS)
