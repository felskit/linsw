################################################################################
#
# gpio-counter
#
################################################################################

GPIO_COUNTER_VERSION = 1.0
GPIO_COUNTER_SITE = $(TOPDIR)/../felskit-lab2/gpio-counter
GPIO_COUNTER_SITE_METHOD = local
GPIO_COUNTER_LICENSE = MIT

define GPIO_COUNTER_BUILD_CMDS
   $(MAKE) $(TARGET_CONFIGURE_OPTS) all -C $(@D)
endef

define GPIO_COUNTER_INSTALL_TARGET_CMDS 
   $(INSTALL) -D -m 0755 $(@D)/counter $(TARGET_DIR)/usr/bin 
endef

$(eval $(generic-package))
