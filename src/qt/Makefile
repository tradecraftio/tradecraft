.PHONY: FORCE
all: FORCE
	$(MAKE) -C .. freicoin_qt test_freicoin_qt
clean: FORCE
	$(MAKE) -C .. freicoin_qt_clean test_freicoin_qt_clean
check: FORCE
	$(MAKE) -C .. test_freicoin_qt_check
freicoin-qt freicoin-qt.exe: FORCE
	 $(MAKE) -C .. freicoin_qt
apk: FORCE
	$(MAKE) -C .. freicoin_qt_apk
