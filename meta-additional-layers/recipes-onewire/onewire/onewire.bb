DESCRIPTION = "My Device Character Driver"
SECTION = "base"
LICENSE = "CLOSED"

SRC_URI = "file://onewire_dev.c \
           file://Makefile \
           "

 
inherit module

S = "${WORKDIR}"
UNPACKDIR = "${S}"


