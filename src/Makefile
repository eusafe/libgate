#
# $Id: Makefile 2735 2012-09-21 14:31:21Z eu $
#

CFLAGS=-Wall 
#-I$(MODULES) -I/usr/local/include -Wpacked

# -fpack-struct=1
#LDFLAGS=
#
ifeq ($(OSTYPE),FreeBSD)
 LIBS=/usr/local/lib/libevent.a
else
 LIBS=-levent
endif

SRC=gp_layer.c gp_queue.c ad_target.c gp_util.c gp_port.c ad_cmd.c gp_processor.c db_layer.c
OBJ=$(addsuffix .o,$(basename $(SRC)))
SRC_H=$(wildcard *.h)

all : libgate.a

test:
	@echo $(SRC_H)
	@echo $(OBJ)
	
libgate.a : $(OBJ)
	ar r $@ $(OBJ)

#libgate.h: $(SRC_H)


%.o : %.c $(SRC_H)
	gcc -c -o $@ $(CFLAGS)  $<


clean :
	-rm *.o  *~ libgate.a
# DO NOT DELETE
