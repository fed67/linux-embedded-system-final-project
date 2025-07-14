SUMMARY = "bitbake-layers recipe"
DESCRIPTION = "Recipe created by bitbake-layers"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI += "file://main.cpp \
            file://logger.cpp \
            file://logger.h \
            file://constants.h \
            file://Makefile \
            "

S = "${WORKDIR}"

EXTRA_OEMAKE = "PREFIX=${prefix} CXX='${CXX}' CFLAGS='${CFLAGS}' DESTDIR=${D} LIBDIR=${libdir} INCLUDEDIR=${includedir} BUILD_STATIC=no"


#inherit update-alternatives

do_compile() {
    oe_runmake
}

do_install() {
    oe_runmake install DESTDIR=${D}

}
