CC=gcc
CFLAGS=-g -Wall -fPIC
LDFLAGS=
INCLUDES=

COMPILE = $(CC) $(INCLUDES) $(CFLAGS)
LINK = $(CC) $(CFLAGS) $(LDFLAGS)

INSTALL = /usr/bin/install -c
INSTALL_DATA = $(INSTALL) -m 644
INSTALL_PROGRAM = $(INSTALL) -m 755

usplash-theme-boxee.so: throbber_back.png.c.o throbber_fore.png.c.o frame01.png.c.o frame02.png.c.o frame03.png.c.o frame04.png.c.o frame05.png.c.o frame06.png.c.o frame07.png.c.o frame08.png.c.o frame09.png.c.o frame10.png.c.o frame11.png.c.o frame12.png.c.o frame13.png.c.o frame14.png.c.o frame15.png.c.o frame16.png.c.o frame17.png.c.o frame18.png.c.o frame19.png.c.o frame20.png.c.o frame21.png.c.o frame22.png.c.o frame23.png.c.o frame24.png.c.o frame25.png.c.o frame26.png.c.o frame27.png.c.o frame28.png.c.o frame29.png.c.o frame30.png.c.o frame31.png.c.o frame32.png.c.o frame33.png.c.o frame34.png.c.o frame35.png.c.o frame36.png.c.o frame37.png.c.o frame38.png.c.o frame39.png.c.o frame40.png.c.o usplash-splash-boxee.c.o
	$(COMPILE) -shared -o $@ $^

%.png.c: %.png
	pngtousplash $< > $@

%.bdf.c: %.bdf
	bdftousplash $< > $@

%.c.o: %.c
	$(COMPILE) -o $@ -c $<

install:
	$(INSTALL) -d $(DESTDIR)/usr/lib/usplash
	$(INSTALL_PROGRAM) usplash-theme-boxee.so $(DESTDIR)/usr/lib/usplash/usplash-theme-boxee.so
	update-alternatives --install /usr/lib/usplash/usplash-artwork.so usplash-artwork.so /usr/lib/usplash/usplash-theme-boxee.so 56
	update-alternatives --set usplash-artwork.so /usr/lib/usplash/usplash-theme-boxee.so

kernel:
	dpkg-reconfigure linux-image-`uname -r`

clean:
	rm -f *.png.c *.bdf.c *.c.o

distclean:
	rm -f *.so
