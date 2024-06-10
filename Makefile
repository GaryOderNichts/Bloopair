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


all: loader pair_menu
	@echo -e "\033[92mDone!\033[0m"

dist: all
	mkdir -p dist/wiiu/apps/Bloopair_pair_menu/
	cp pair_menu/Bloopair_pair_menu.rpx dist/wiiu/apps/Bloopair_pair_menu/
	cp pair_menu/Bloopair_pair_menu.wuhb dist/wiiu/apps/Bloopair_pair_menu/
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

pair_menu: libbloopair
	@echo -e "\033[92mBuilding $@...\033[0m"
	@$(MAKE) --no-print-directory -C $(CURDIR)/pair_menu

clean:
	@$(MAKE) --no-print-directory -C $(CURDIR)/ios/ios_kernel clean
	@$(MAKE) --no-print-directory -C $(CURDIR)/ios/ios_usb clean
	@$(MAKE) --no-print-directory -C $(CURDIR)/ios/ios_pad clean
	@$(MAKE) --no-print-directory -C $(CURDIR)/loader clean
	@$(MAKE) --no-print-directory -C $(CURDIR)/libbloopair clean
	@$(MAKE) --no-print-directory -C $(CURDIR)/pair_menu clean
