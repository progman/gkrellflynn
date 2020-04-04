#
# Makefile for gkrellflynn
#
# updated for gkrellm2.0

CC = gcc
CFLAGS = -c -O -Wall -fPIC `${EXTRACONF}`

LD = $(CC)
LDFLAGS = -shared -o
INCPATH  = -I.
INCPATH += ${GKRELL_INC}

TARGET = gkrellflynn.so

OBJS = gkrellflynn.o

all:
	@echo "type"
	@echo "     make gkrellm"
	@echo "or"
	@echo "     make gkrellm2"

gkrellm:
	(make GKRELL_INC=-DGKRELLM EXTRACONF="gtk-config --cflags;imlib-config --cflags-gdk" ${TARGET})

gkrellm2:
	(make GKRELL_INC=-DGKRELLM2 EXTRACONF="pkg-config --cflags gtk+-2.0 gthread-2.0" ${TARGET})

${TARGET}:	${OBJS}
		${LD} ${DEBUG} ${LDFLAGS} ${TARGET} ${OBJS}

clean:
	rm -f ${OBJS}
	rm -f ${TARGET}

install:
	@echo "type"
	@echo "     make installgkrellm"
	@echo "or  "
	@echo "     make installgkrellm2"

installgkrellm:
	./install-sub.sh	${TARGET} gkrellm

installgkrellm2: 
	./install-sub.sh	${TARGET} gkrellm2

%.o :	%.c
	${CC} ${CFLAGS} $(INCPATH) -o $@ $<

