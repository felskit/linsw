################################################################################
#
# python-welcome
#
################################################################################

PYTHON_WELCOME_VERSION = 1.0
PYTHON_WELCOME_SITE = $(TOPDIR)/../felskit-lab1/python-welcome
PYTHON_WELCOME_SITE_METHOD = local
PYTHON_WELCOME_DEPENDENCIES = python
PYTHON_WELCOME_LICENSE = MIT

define PYTHON_WELCOME_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/hello-world.py $(TARGET_DIR)/usr/bin
endef

$(eval $(generic-package))
