OBJECTS=cinema.o cinema_map.o servidor.o buffer.o list.o


DEFAULT_INCLUDES = -I. -I$(srcdir) -I../include -I ../include 
COMPILE = $(CC) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(CFLAGS)  
AR = ar
CC = gcc
CFLAGS ?= -g -O0 -Wall -m32 -pthread 
DEFS ?=  -DHAVE_CONFIG_H -DUSE_PTHREADS
ARFLAGS = cru
LIBSTHREAD = ../sthread_lib/libsthread.a

DEFS ?= -DHAVE_CONFIG_H

RANLIB = ranlib
INCLUDES = -I . -I ../include

all: libcinema.a show

show: show.o
	$(COMPILE)  ../sthread_lib/sthread_start.o -o $@ $< libcinema.a $(LIBSTHREAD)

libcinema.a: $(OBJECTS) 
	$(AR) $(ARFLAGS) $@ $^
	$(RANLIB) $@


.c.o:
	$(COMPILE) -c -o $@ $< 

clean: 
	rm -f *.o *~
	rm -f *.a
	rm show









# Tell versions [3.59,3.63) of GNU make to not export all variables.
# Otherwise a system limit (for SysV at least) may be exceeded.
.NOEXPORT:
