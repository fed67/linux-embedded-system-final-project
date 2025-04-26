DESCRIPTION = "My Device Character Driver"
SECTION = "base"
LICENSE = "CLOSED"

SRC_URI = "file://onewire_dev.c \
           file://Makefile \
           "

S = "${WORKDIR}"
EXTRA_OEMAKE = "INCLUDEDIR=${includedir}"
inherit module

do_install() {
    install -d ${D}${base_libdir}/modules/${KERNEL_VERSION}/kernel/drivers/char
    install -m 0644 onewire_dev.ko ${D}${base_libdir}/modules/${KERNEL_VERSION}/kernel/drivers/char/
}

