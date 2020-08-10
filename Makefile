#Makefile at top of application tree
TOP = .
include $(TOP)/configure/CONFIG

DIRS += configure

DIRS += src
src_DEPEND_DIRS = configure

ifdef EPICS_PVA_MAJOR_VERSION
DIRS += testing
testing_DEPEND_DIRS = src
endif

include $(TOP)/configure/RULES_TOP

UNINSTALL_DIRS += $(wildcard $(INSTALL_LOCATION)/python*)

# jump to a sub-directory where CONFIG_PY has been included
# can't include CONFIG_PY here as it may not exist yet
sphinx sh ipython: all
	$(MAKE) -C src/O.$(EPICS_HOST_ARCH) $@ PYTHON=$(PYTHON)
nose.%: all
	$(MAKE) -C src/O.$(EPICS_HOST_ARCH) $@ PYTHON=$(PYTHON)
nose.%: all
	$(MAKE) -C src/O.$(EPICS_HOST_ARCH) $@ PYTHON=$(PYTHON)

nose: nose.p4p

nose: nose.p4p

sphinx-clean:
	$(MAKE) -C documentation clean PYTHON=$(PYTHON)

sphinx-commit: sphinx
	touch documentation/_build/html/.nojekyll
	./commit-gh.sh documentation/_build/html

.PHONY: nose nose.% sphinx sh ipython sphinx-commit sphinx-clean
