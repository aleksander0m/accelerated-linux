#
# Makefile -- Build instructions for Digi/AnywhereUSB8
#

ROMFSIMG  = $(IMAGEDIR)/rootfs.bin
ROMFSSIG  = $(IMAGEDIR)/rootfs.bin.sig
ROMFSSIZE = $(IMAGEDIR)/rootfs.bin.size
KERNEL    = $(IMAGEDIR)/kernel.bin
ZKERNEL   = $(IMAGEDIR)/kernel.gz
ITBIMG    = $(IMAGEDIR)/anywhereusb8.itb
ITBSIG    = $(IMAGEDIR)/anywhereusb8.itb.sig
IMAGE     = $(IMAGEDIR)/image.bin

PRODUCTION_BUILD=0
ifeq ($(BUILD_TYPE),candidate)
PRODUCTION_BUILD=1
endif

ifneq ($(PRODUCTION_BUILD),1)
$(info Developer Build $(BUILD_TYPE))
FIRMWARE_CA_PEM = $(ROOTDIR)/prop/sign_image/devkeys/awusb/firmware_ca.pem
SIGN_IMAGE_SCRIPT = $(ROOTDIR)/prop/sign_image/sign_image.py
SIGNER_CERT_PEM = $(ROOTDIR)/prop/sign_image/devkeys/awusb/development.pem
SIGNER_OPTS =
else
$(info PRODUCTION BUILD)
FIRMWARE_CA_PEM = $(ROOTDIR)/prop/sign_image/production/awusb/firmware_ca.pem
SIGN_IMAGE_SCRIPT = $(ROOTDIR)/prop/sign_image/sign_awusb.py
SIGNER_CERT_PEM = $(ROOTDIR)/prop/sign_image/production/awusb/signer_cert.pem
SIGNER_OPTS = -p
endif

VENDOR_ROMFS_DIR = $(ROOTDIR)/vendors/AcceleratedConcepts
ROMFS_DIRS = $(DEFAULT_ROMFS_DIRS)
ROMFS_DIRS += etc etc/cli etc/config etc/runt.d etc/ssl etc/ssl/firmware home proc sys tmp usr/sbin usr/var var opt bin

DEVICES += $(DEVICE_PTY_64) \
	serial/port1,c,4,65

FLASH_DEVICES = \
	boot,c,90,2 \
	bootenv,c,90,6 \
	opt,b,179,5

all:

clean: image.clean
	-rm version.info

romfs: romfs_dev romfs.dirs romfs.default romfs.rc romfs.version romfs.cryptokey
	$(ROMFSINST) /etc/default/start
	$(ROMFSINST) -d -p 555 /etc/action.d/led
	$(ROMFSINST) /etc/fw_env.config
	[ ! -d $(ROOTDIR)/prop ] || $(ROMFSINST) -p 400 $(FIRMWARE_CA_PEM) /etc/ssl/firmware/firmware_ca.pem
	$(ROMFSINST) -s /var/tmp/log /dev/log
	$(ROMFSINST) -s /var/run /run
	$(ROMFSINST) -s /var/run/syslog.conf -e CONFIG_USER_SYSKLOGD /etc/syslog.conf
	$(ROMFSINST) -d /etc/inittab.d/console
	$(ROMFSINST) -p 555 /usr/sbin/digi_check_firmware_update
	$(ROMFSINST) -p 555 /usr/sbin/digi_update_firmware
	$(ROMFSINST) -p 555 digi_update_firmware /bin/netflash
	echo "Digi AnywhereUSB8" > $(ROMFSDIR)/etc/issue
	echo "NETWORK_PRODUCT=awusb">version.info
	echo "PRODUCT_VERSION="$$(sed -e 's?^[^ ]* [^ ]* ??' -e 's?-.*$$??' <$(ROMFSDIR)/etc/version)>> version.info
	echo "PRODUCT_BUILDSTRING="$$(sed -e 's?^[^ ]* [^ ]* ??' -e 's?-.*$$??' <$(ROMFSDIR)/etc/version) awusb -- $(BUILD_START_STRING) >> version.info
	echo "SUPPORTED_PRODUCT_IDS=0xd003,0xd007" >>version.info
	echo "OLDEST_SUPPORTED_VERSION=20.2.0.0" >>version.info
	echo "MIN_RUNNING_RELEASE=" >>version.info
	$(ROMFSINST) -p 444 /etc/version.info

romfs.post:: romfs.cleanup

uimage.bin:
	cp $(ROOTDIR)/$(LINUXDIR)/arch/arm64/boot/dts/freescale/anywhereusb8.dtb $(IMAGEDIR)/
	cp $(ROOTDIR)/$(LINUXDIR)/arch/arm64/boot/Image $(KERNEL)
	gzip -c $(KERNEL) > $(ZKERNEL)
	cp anywhereusb8.its $(IMAGEDIR)
	( cd $(IMAGEDIR) ; mkimage -f anywhereusb8.its $(ITBIMG) )
	cp anywhereusb8-initrd.its $(IMAGEDIR)
	( cd $(IMAGEDIR) ; mkimage -f anywhereusb8-initrd.its anywhereusb8-initrd.itb )
	rm -f $(IMAGEDIR)/anywhereusb8*.its

image: image.configs image.dir image.squashfs uimage.bin image.tag image.copy
	if [ -x $(SIGN_IMAGE_SCRIPT) ] ; then \
		$(SIGN_IMAGE_SCRIPT) $(ITBIMG) $(ROOTDIR)/romfs awusb || exit 1; \
		$(SIGN_IMAGE_SCRIPT) $(ROMFSIMG) $(ROOTDIR)/romfs awusb || exit 1; \
		stat --printf="%s\n" $(ROMFSIMG) > $(ROMFSSIZE) ; \
		./mk_firmware_image.sh \
			$(ROMFSDIR)/etc/version.info \
			$(SIGNER_CERT_PEM) \
			$(ITBIMG) \
			$(ROMFSIMG) \
			$(IMAGE) \
			$(IMAGEDIR) \
			-f $(SIGNER_OPTS) || exit 1; \
		[ "$(NO_BUILD_INTO_TFTPBOOT)" ] || cp $(ITBSIG) /tftpboot/kernel.sig ; \
		[ "$(NO_BUILD_INTO_TFTPBOOT)" ] || cp $(ROMFSSIG) /tftpboot/rootfs.sig ; \
		[ "$(NO_BUILD_INTO_TFTPBOOT)" ] || cp $(ROMFSSIZE) /tftpboot/rootfs.size ; \
		[ "$(NO_BUILD_INTO_TFTPBOOT)" ] || cp $(IMAGE) /tftpboot/anywhereusb8.bin; \
	fi
	[ "$(NO_BUILD_INTO_TFTPBOOT)" ] || cp $(ITBIMG) /tftpboot/kernel
	[ "$(NO_BUILD_INTO_TFTPBOOT)" ] || cp $(ROMFSIMG) /tftpboot/rootfs

include $(ROOTDIR)/vendors/config/config.dev
include $(ROOTDIR)/vendors/AcceleratedConcepts/vendor.mak
