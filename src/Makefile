
# Standard compiler parameters
CFLAGS  = -O -I../libslink

# GCC specific parameters
GCC = gcc
GCCFLAGS = -O2 -Wall -I../libslink

LDFLAGS = -L../libslink
LDLIBS  = -lslink

# For SunOS/Solaris uncomment the following line
#LDLIBS = -lslink -lsocket -lnsl -lrt

BIN  = ../slarchive

OBJS = dsarchive.o archive.o slarchive.o

all: $(BIN)

$(BIN): $(OBJS)
	$(CC) $(CFLAGS) -o $(BIN) $(OBJS) $(LDFLAGS) $(LDLIBS)

static: $(OBJS)
	$(CC) $(CFLAGS) -static -o $(BIN) $(OBJS) $(LDFLAGS) $(LDLIBS)

cc:
	@$(MAKE) "CC=$(CC)" "CFLAGS=$(CFLAGS)"

gcc:
	@$(MAKE) "CC=$(GCC)" "CFLAGS=$(GCCFLAGS)"

debug:
	$(MAKE) "CC=$(CC)" "CFLAGS=-g $(CFLAGS)"

gccdebug:
	$(MAKE) "CC=$(GCC)" "CFLAGS=-g $(GCCFLAGS)"

clean:
	rm -f $(OBJS) $(BIN)

install:
	@echo
	@echo "No install method, copy the binary and man page yourself"
	@echo

