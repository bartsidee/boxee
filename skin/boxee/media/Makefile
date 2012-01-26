#find . -name \*.png -o -name \*.gif | sed 's/^..//' | sed 's/ /\\ /g' | sed 's/^/          /' | sed 's/$/ \\/' > images.lst

TARGET=Textures.xbt
XBMCTEX=../../../tools/TexturePacker/TexturePacker

all: $(TARGET)

.PHONY: force

$(TARGET): $(XBMCTEX) 
	$(XBMCTEX) -input . -output $(TARGET)

$(XBMCTEX): force
	$(MAKE) -C ../../../tools/TexturePacker

clean:
	rm -f $(TARGET)
distclean: clean


