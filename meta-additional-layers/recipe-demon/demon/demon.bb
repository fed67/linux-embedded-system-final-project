SUMMARY = "bitbake-layers recipe"
DESCRIPTION = "Recipe created by bitbake-layers"
LICENSE = "MIT"

SRC_URI += "file://main.cpp"

S = "${WORKDIR}"

EXTRA_OEMAKE = "PREFIX=${prefix} CC='${CC}' CFLAGS='${CFLAGS}' DESTDIR=${D} LIBDIR=${libdir} INCLUDEDIR=${includedir} BUILD_STATIC=no"

#do_install() {
#        oe_runmake install
#}

do_install() {
    install -d ${D}${bindir}
    install -m 0755 hello_world ${D}${bindir}
}
