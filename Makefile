SUBDIRS = 
LIBS = rpn bmf
COMMON = io
include rules.mk

install:
	$(MAKE)
	python setup.py build
	python setup.py install --prefix=$(DESTDIR)/usr/local
	./cp_libs.sh
	./cp_bins.sh
