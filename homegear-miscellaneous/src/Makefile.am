AUTOMAKE_OPTIONS = subdir-objects

AM_CPPFLAGS = -Wall -std=c++17 -D_FORTIFY_SOURCE=2 -DGCRYPT_NO_DEPRECATED
AM_LDFLAGS = -Wl,-rpath=/lib/homegear -Wl,-rpath=/usr/lib/homegear -Wl,-rpath=/usr/local/lib/homegear

libdir = $(localstatedir)/lib/homegear/modules
lib_LTLIBRARIES = mod_miscellaneous.la
mod_miscellaneous_la_SOURCES = Makefile.am MiscPeer.h Miscellaneous.h Miscellaneous.cpp Factory.cpp Gd.h MiscPeer.cpp DeviceTypes.h Gd.cpp Factory.h MiscCentral.cpp MiscCentral.h
mod_miscellaneous_la_LDFLAGS =-module -avoid-version -shared
install-exec-hook:
	rm -f $(DESTDIR)$(libdir)/mod_miscellaneous.la
