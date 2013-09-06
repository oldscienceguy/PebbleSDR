TOPDIR  := $(shell cd ..; pwd)
include $(TOPDIR)/Rules.make

APP = write-test

all: $(APP)

$(APP): main.c	
	$(CC) main.c -o $(APP) $(CFLAGS)	
	
clean:
	rm -f *.o ; rm $(APP)
