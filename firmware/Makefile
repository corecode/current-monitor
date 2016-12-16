PROG=	current-monitor
PROJECTDIR:=$(abspath $(dir $(lastword ${MAKEFILE_LIST})))
BOARD=${PROJECTDIR}/board

SRCS=	current-monitor.c ter-u24b.c

CFLAGS+= -Wall

include mchck-os/build/mchck.mk
