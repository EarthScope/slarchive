CC = gcc
CFLAGS  = -O2 -Wall -I../libslink

# For using another compiler (e.g. the Sun compiler)
#CC = cc
#CFLAGS = -O -I../libslink

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

clean:
	rm -f $(OBJS) $(BIN)

install:
	@echo
	@echo "No install method, copy the binary and man page yourself"
	@echo

