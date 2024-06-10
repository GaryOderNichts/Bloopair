#-------------------------------------------------------------------------------
.SUFFIXES:
#-------------------------------------------------------------------------------

export BLOOPAIR_TOP_DIR := $(CURDIR)

# Always build debug builds onless specified otherwise
ifeq ($(DEBUG),)
	export DEBUG := 1
endif

ifeq ($(DEBUG), 1)
	export BLOOPAIR_COMMIT_HASH := $(or $(shell git rev-parse HEAD),"ffffffffffffffffffffffffffffffffffffffff")
endif

.PHONY: all clean ios_kernel ios_usb ios_pad libbloopair loader koopair

all: loader koopair
	@echo -e "\033[92mDone!\033[0m"

dist: all
	mkdir -p dist/wiiu/apps/Koopair/
	cp koopair/Koopair.rpx dist/wiiu/apps/Koopair/
	cp koopair/Koopair.wuhb dist/wiiu/apps/Koopair/
	cp loader/30_bloopair.rpx dist/

ios_kernel: ios_usb ios_pad
	@echo -e "\033[92mBuilding $@...\033[0m"
	@$(MAKE) --no-print-directory -C $(CURDIR)/ios/ios_kernel

ios_usb:
	@echo -e "\033[92mBuilding $@...\033[0m"
	@$(MAKE) --no-print-directory -C $(CURDIR)/ios/ios_usb

ios_pad:
	@echo -e "\033[92mBuilding $@...\033[0m"
	@$(MAKE) --no-print-directory -C $(CURDIR)/ios/ios_pad

libbloopair:
	@echo -e "\033[92mBuilding $@...\033[0m"
	@$(MAKE) --no-print-directory -C $(CURDIR)/libbloopair

loader: ios_kernel libbloopair
	@echo -e "\033[92mBuilding $@...\033[0m"
	@$(MAKE) --no-print-directory -C $(CURDIR)/loader

koopair: libbloopair
	@echo -e "\033[92mBuilding $@...\033[0m"
	@$(MAKE) --no-print-directory -C $(CURDIR)/koopair

clean:
	@$(MAKE) --no-print-directory -C $(CURDIR)/ios/ios_kernel clean
	@$(MAKE) --no-print-directory -C $(CURDIR)/ios/ios_usb clean
	@$(MAKE) --no-print-directory -C $(CURDIR)/ios/ios_pad clean
	@$(MAKE) --no-print-directory -C $(CURDIR)/loader clean
	@$(MAKE) --no-print-directory -C $(CURDIR)/libbloopair clean
	@$(MAKE) --no-print-directory -C $(CURDIR)/koopair clean
