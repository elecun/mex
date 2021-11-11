
TARGET_DRIVER:=advserptl
LVER:=$(shell uname -r)
MOD_DIR:= /lib/modules/$(LVER)/kernel/drivers/char/

all: module

install:
	@mkdir -p /lib/modules/$(LVER)/kernel/drivers/char/
	$(shell [ -e $(MOD_DIR)/$(TARGET_DRIVER).ko.gz ] && mv $(MOD_DIR)/$(TARGET_DRIVER).ko.gz $(MOD_DIR)/$(TARGET_DRIVER).ko.gz.bk)
	@cp -f ./$(TARGET_DRIVER).ko /lib/modules/$(LVER)/kernel/drivers/char/
	@mkdir -p /lib/modules/$(LVER)/misc/
	@cp -f ./$(TARGET_DRIVER).ko /lib/modules/$(LVER)/misc/
	@depmod -a
	@modprobe $(TARGET_DRIVER)
	@echo " "
	@echo "**********************************************************************"
	@echo " Installed $(TARGET_DRIVER).ko successfully."
	@echo "**********************************************************************"

ifneq ($(KERNELRELEASE),)
obj-m := advserptl.o
advserptl-objs := seradvptl_pci.o seradvptl.o seradv_func.o
else
KDIR	:= /lib/modules/$(LVER)/build
PWD	:= $(shell pwd)

module: 
	@$(MAKE) -w -C $(KDIR) M=$(PWD) modules 2>> build.log \
	&& cp -f ./build.log ../build.log \
	&& exit 0 \
	|| echo "Building Error! Please check build.log for further information." \
	&& cp -f ./build.log ../build.log \
	&& exit 1

clean: clean_local

endif

uninstall: clean_lib

clean_local:
	@rm -f *.o *.log
	@rm -f *.ko
	@rm -f *.mod.c modules.order  Module.symvers Module.markers
	@rm -f .*.cmd
	@rm -rf .tmp_versions
	@rm -rf *~

clean_lib:
	$(shell if grep $(TARGET_DRIVER) /proc/modules > /dev/null ; then \
	 rmmod $(TARGET_DRIVER) ; fi)
	@rm -f /lib/modules/$(LVER)/kernel/drivers/char/$(TARGET_DRIVER).ko
	@rm -f /lib/modules/$(LVER)/misc/$(TARGET_DRIVER).ko
	@depmod -a
	@echo "**********************************************************************"
	@echo " Removed $(TARGET_DRIVER).ko successfully."
	@echo "**********************************************************************"
