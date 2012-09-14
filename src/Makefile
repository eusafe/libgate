#
# $Id: Makefile 2731 2012-09-14 14:55:56Z eu $
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


#gp_port.c 
SRC=gp_layer.c gp_queue.c ad_target.c gp_util.c gp_port.c ad_cmd.c gp_processor.c
OBJ=$(addsuffix .o,$(basename $(SRC)))
SRC_H=$(wildcast *.h)

all : libgate.a

test:
	@echo $(OBJ)
	
libgate.a : $(OBJ)
	ar r $@ $(OBJ)
#	gcc -o adapter  $(OBJ) $(LIBS)

libgate.h: $(SRC_H)
	echo $(SRC_H)

#adapter.o : adapter.c  libgate.h

%.o : %.c libgate.h
	gcc $(CFLAGS) -c $<

gp_layer.o : gp_layer.c gp_layer.h  ad_target.h libgate.h
	gcc $(CFLAGS) -c  $<


clean :
	-rm *.o  *~ libgate.a
# DO NOT DELETE
