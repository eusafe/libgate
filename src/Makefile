#
# $Id: GNUmakefile 1833 2010-12-23 09:06:14Z eu $
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
SRC=gp_layer.c gp_queue.c ad_target.c gp_util.c gp_port.c
OBJ=$(addsuffix .o,$(basename $(SRC))) $(OBJ_MODULES)

all : libgate.a

test:
	@echo $(OBJ)
	
libgate.a : $(OBJ)
	ar r $@ $(OBJ)
#	gcc -o adapter  $(OBJ) $(LIBS)

adapter.o : adapter.c  gp_layer.c libgate.h
	gcc $(CFLAGS) -c $<

gp_layer.o : gp_layer.c gp_layer.h  ad_target.h libgate.h
	gcc $(CFLAGS) -c  $<


clean :
	-rm *.o  *~
