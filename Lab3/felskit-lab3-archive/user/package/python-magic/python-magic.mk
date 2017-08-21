################################################################################
#
# python-magic
#
################################################################################

PYTHON_MAGIC_VERSION = 0.4.13
PYTHON_MAGIC_SITE = https://github.com/ahupp/python-magic.git
PYTHON_MAGIC_SITE_METHOD = git
PYTHON_MAGIC_LICENSE = MIT
PYTHON_MAGIC_SETUP_TYPE = setuptools

$(eval $(python-package))
