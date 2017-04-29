SOC+=	kinetis/MKL27Z4
TARGET=	MKL27Z64VFM4

SRCS-board=	board.c
SRCS-board.dir= ${_libdir}/../board
SRCS.libs+=	board
