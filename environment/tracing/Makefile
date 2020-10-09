EXE=app
SRCS=$(wildcard *.c)

ifneq ($(DEBUG),)
# definie le debug pour le tracage (make DEBUG=1)
EXTRA_CFLAGS+=-DDEBUG
endif

# Include the standard application Makefile for the CSEL1 labs
include ../appl.mk
