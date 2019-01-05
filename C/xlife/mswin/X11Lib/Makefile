CC		= /opt/mingw32/bin/i686-w64-mingw32-gcc
CXX		= /opt/mingw32/bin/i686-w64-mingw32-g++
RESC		= /opt/mingw32/bin/i686-w64-mingw32-windres
#CC		= /opt/mingw64/bin/x86_64-w64-mingw32-gcc
#CXX		= /opt/mingw64/bin/x86_64-w64-mingw32-g++
#RESC		= /opt/mingw64/bin/x86_64-w64-mingw32-windres
#CC		= i586-mingw32msvc-g++
#CXX		= i586-mingw32msvc-g++
#RESC		= i586-mingw32msvc-windres

CFLAGS1		= -s -O5 -mwindows -I../include -posix

XPATSDIR_ROOT 	= /usr/share/xlife
CVARS		= -DLIFEDIR=\".\" -DHASHBITS=20 -DVFREQ=50 -DMICROSOFT #-DSMALLFONT
CCOPTIONS	= $(CFLAGS1) -iquote../.. $(CVARS)
LDINIOPTS	= -O5 -s -posix -static
LDOPTIONS	= -mwindows $(LDINIOPTS)
#-Wl,-e_WinMain@16

EXTRA_LIBRARIES	= -lstdc++

OBJS1 = X11WinMain.o\
	X11Lib.o\
        X11App.o\
	X11FrameWnd.o\
	X11Wnd.o\
	#X11Lib.res

OBJS2 = cell.o\
	clipboard.o\
	collect.o\
	colors.o\
	data.o\
	file.o\
	file_misc.o\
	framebuffer.o\
	generate.o\
	gentab.o\
	help.o\
	history.o\
	isave.o\
	key.o\
	tentative.o\
	topology.o\
	tile.o\
	utils.o\
	xwidget.o\
	bignumber.o\
	hashlife.o\
	main.o

PREF = ../../

all: xlife.exe lifeconv.exe gen-multirules.exe

xlife.exe: $(OBJS1) $(OBJS2)
	$(CC) -o $@ $(LDOPTIONS) $(OBJS2) $(OBJS1) $(EXTRA_LIBRARIES)

lifeconv.exe: xlife.exe
	cp $< $@

gen-multirules.exe: $(PREF)gen-multirules.cpp
	$(CC) -o $@ $(LDINIOPTS) $<

X11Lib.res: $(PREF)icon.h X11Lib.rc
#	convert xbm:$< -negate X11Lib.ico
	$(RESC) X11Lib.rc -O coff -o $@

X11App.o: X11App.cpp X11App.h X11Wnd.h
	$(CXX) $(CFLAGS1) -c -o $@ $<

X11FrameWnd.o: X11FrameWnd.cpp X11FrameWnd.h X11App.h
	$(CXX) $(CFLAGS1) -c -o $@ $<

X11Lib.o: X11Lib.cpp X11Wnd.h X11FrameWnd.h X11App.h Xlife.h
	$(CXX) $(CFLAGS1) -c -o $@ $<

Xlife.h: $(PREF)defs.h
	grep '^#define.*MAXCHANGE.*[0-9]' $< >$@

X11WinMain.o: X11WinMain.cpp
	$(CXX) $(CFLAGS1) -c -o $@ $<

X11Wnd.o: X11Wnd.cpp X11Wnd.h X11App.h
	$(CXX) $(CFLAGS1) -c -o $@ $<

COMMON1 = $(PREF)common.h $(PREF)defs.h
COMMON2 = $(COMMON1) $(PREF)tile.h
COMMON3 = $(COMMON2) $(PREF)colors.h
COMMON4 = $(COMMON3) $(PREF)file.h

cell.o: $(PREF)cell.cpp $(PREF)framebuffer.h $(COMMON3)
	$(CXX) $(CCOPTIONS) -c -o $@ $<

clipboard.o: $(PREF)clipboard.cpp $(PREF)clipboard.h $(COMMON3)
	$(CXX) $(CCOPTIONS) -c -o $@ $<

file.o: $(PREF)file.cpp $(PREF)topology.h $(COMMON4)
	$(CXX) $(CCOPTIONS) -c -o $@ $<

collect.o: $(PREF)collect.cpp $(PREF)file.h $(PREF)colors.h $(COMMON1)
	$(CXX) $(CCOPTIONS) -c -o $@ $<

colors.o: $(PREF)colors.cpp $(PREF)colors.h $(COMMON1)
	$(CXX) $(CCOPTIONS) -c -o $@ $<

data.o: $(PREF)data.cpp $(COMMON1)
	$(CXX) $(CCOPTIONS) -c -o $@ $<


file_misc.o: $(PREF)file_misc.cpp $(COMMON2)
	$(CXX) $(CCOPTIONS) -c -o $@ $<

framebuffer.o: $(PREF)framebuffer.cpp $(PREF)framebuffer.h $(PREF)colors.h $(PREF)history.h $(COMMON1)
	$(CXX) $(CCOPTIONS) -c -o $@ $<

generate.o: $(PREF)generate.cpp $(PREF)history.h $(PREF)tab.h $(PREF)topology.h $(COMMON4)
	$(CXX) $(CCOPTIONS) -c -o $@ $<

gentab.o: $(PREF)gentab.cpp $(COMMON1)
	$(CXX) $(CCOPTIONS) -c -o $@ $<

$(PREF)help.h: $(PREF)help.text
	sed <$< -e '/\\[^n]/s//\\&/g' -e '/.*/s//"&",/' >$@

help.o: $(PREF)help.cpp $(PREF)help.h $(PREF)history.h $(PREF)file.h $(COMMON2)
	$(CXX) $(CCOPTIONS) -c -o $@ $<

history.o: $(PREF)history.cpp $(PREF)history.h $(COMMON2)
	$(CXX) $(CCOPTIONS) -c -o $@ $<

isave.o: $(PREF)isave.cpp $(PREF)file.h $(COMMON2)
	$(CXX) $(CCOPTIONS) -c -o $@ $<

key.o: $(PREF)key.cpp $(COMMON3)
	$(CXX) $(CCOPTIONS) -c -o $@ $<

main.o: $(PREF)main.cpp $(PREF)framebuffer.h $(PREF)patchlevel.h $(PREF)icon.h $(PREF)cursor.h $(PREF)clipboard.h $(PREF)history.h $(PREF)topology.h $(PREF)xwidget.h $(COMMON4)
	$(CXX) $(CCOPTIONS) -c -o $@ $<

tentative.o: $(PREF)tentative.cpp $(COMMON2)
	$(CXX) $(CCOPTIONS) -c -o $@ $<

tile.o: $(PREF)tile.cpp $(PREF)framebuffer.h $(PREF)history.h $(PREF)patchlevel.h $(PREF)topology.h $(COMMON4)
	$(CXX) $(CCOPTIONS) -c -o $@ $<

topology.o: $(PREF)topology.cpp $(COMMON3)
	$(CXX) $(CCOPTIONS) -c -o $@ $<

utils.o: $(PREF)utils.cpp $(COMMON3)
	$(CXX) $(CCOPTIONS) -c -o $@ $<

xwidget.o: $(PREF)xwidget.cpp $(PREF)xwidget.h $(COMMON4)
	$(CXX) $(CCOPTIONS) -c -o $@ $<

bignumber.o: $(PREF)bignumber.cpp $(PREF)bignumber.h
	 $(CXX) $(CCOPTIONS) -c -o $@ $<

hashlife.o: $(PREF)hashlife.cpp $(PREF)hashlife.h $(PREF)bignumber.h $(PREF)framebuffer.h $(COMMON2)
	 $(CXX) $(CCOPTIONS) -c -o $@ $<

clean::
	rm -f *.o Xlife.h *.exe icon.* *.res *~

