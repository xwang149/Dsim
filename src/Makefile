ifndef CODESBASE
CODESBASE=/home/xin/workspace/codes-base/install
endif
ifndef CODESNET
CODESNET=/home/xin/workspace/codes-net/install
endif
ifndef ROSS
ROSS=/home/xin/workspace/ROSS/install
endif

# ross conflates CFLAGS with CPPFLAGS, so use just this one
override CPPFLAGS += $(shell $(ROSS)/bin/ross-config --cflags) -I$(CODESBASE)/include -I$(CODESNET)/include/codes -I/usr/local/include/glib-2.0 -I/usr/local/lib/glib-2.0/include -I/usr/local/opt/gettext/include
CC = $(shell $(ROSS)/bin/ross-config --cc)
LDFLAGS = $(shell $(ROSS)/bin/ross-config --ldflags) -L$(CODESBASE)/lib -L$(CODESNET)/lib
LDLIBS = $(shell $(ROSS)/bin/ross-config --libs) -lcodes-net -lcodes-base -L/usr/local/lib -lglib-2.0 -L/usr/local/opt/gettext/lib -std=c99

SOURCES=datsim.c lp_source_host.c lp_source_router.c lp_dest_host.c util.c
#OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=datsim

datsim: $(SOURCES)

clean:   
	rm -f $(EXECUTABLE)
	

