#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------

ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>/devkitpro")
endif

TOPDIR ?= $(CURDIR)

#---------------------------------------------------------------------------------
# iosu_rules
#---------------------------------------------------------------------------------
ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>/devkitARM")
endif

include $(DEVKITARM)/base_rules
export OBJDUMP := $(PREFIX)objdump

MACHDEP = -DSTARBUCK -mbig-endian -mcpu=arm926ej-s -msoft-float -mfloat-abi=soft

%.elf:
	@echo linking ... $(notdir $@)
	$(SILENTCMD)$(LD) $(LDFLAGS) $(OFILES) $(LIBPATHS) $(LIBS) -o $@
#---------------------------------------------------------------------------------

#---------------------------------------------------------------------------------
# TARGET is the name of the output
# SOURCES is a list of directories containing source code
# DATA is a list of directories containing data files
# INCLUDES is a list of directories containing header files
#---------------------------------------------------------------------------------
TARGET		:= $(notdir $(CURDIR))
BUILD		:= build
SOURCES		:= source
DATA		:= data
INCLUDES	:= source

#---------------------------------------------------------------------------------
# options for code generation
#---------------------------------------------------------------------------------
CFLAGS	:= -Wall -std=gnu11 -Os -flto $(MACHDEP) $(INCLUDE)

ASFLAGS	:= $(MACHDEP)

LDFLAGS := -nostartfiles -nodefaultlibs -mbig-endian -flto \
	-Wl,-Map,$(notdir $*.map),-T $(TOPDIR)/link.ld

LIBS	:= -lgcc

#---------------------------------------------------------------------------------
# list of directories containing libraries, this must be the top level
# containing include and lib
#---------------------------------------------------------------------------------
LIBDIRS	:= 

#---------------------------------------------------------------------------------
# no real need to edit anything past this point unless you need to add additional
# rules for different file extensions
#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))
#---------------------------------------------------------------------------------

export TARGETNAME := $(TARGET)

export OUTPUT	:= $(CURDIR)/$(TARGET)
export TOPDIR	:= $(CURDIR)

export VPATH	:= $(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
	$(foreach dir,$(DATA),$(CURDIR)/$(dir))

export DEPSDIR	:= $(CURDIR)/$(BUILD)

CFILES	:= $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
SFILES	:= $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
BINFILES	:= $(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.*)))

export LD	:= $(CC)

export OFILES_BIN	:= $(addsuffix .o,$(BINFILES))
export OFILES_SRC	:= $(SFILES:.s=.o) $(CFILES:.c=.o)
export OFILES	:= $(OFILES_BIN) $(OFILES_SRC)
export HFILES_BIN	:= $(addsuffix .h,$(subst .,_,$(BINFILES)))

export INCLUDE	:= $(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
	$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
	-I$(CURDIR)/$(BUILD)

export LIBPATHS	:= $(foreach dir,$(LIBDIRS),-L$(dir)/lib)

.PHONY: $(BUILD) clean all
#---------------------------------------------------------------------------------
all: $(BUILD)

$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

#---------------------------------------------------------------------------------
clean:
	@echo clean ...
	@rm -fr $(BUILD) $(TARGET).elf $(TARGET).bin $(TARGET).bin.h

#---------------------------------------------------------------------------------
else

DEPENDS := $(OFILES:.o=.d)

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
all	:	$(OUTPUT).bin.h

$(OUTPUT).elf	:	$(OFILES)

$(OUTPUT).bin: $(OUTPUT).elf
	@echo "built ... $(notdir $@)"
	@$(OBJCOPY) -j .text -j .rodata -j .data -O binary $(OUTPUT).elf $@

$(OUTPUT).bin.h: $(OUTPUT).bin
	@raw2c $<
	@cp $(TARGETNAME).c $@

$(OFILES_SRC) : $(HFILES_BIN)

#-------------------------------------------------------------------------------
# you need a rule like this for each extension you use as binary data
#-------------------------------------------------------------------------------
%.bin.o	%_bin.h :	%.bin
#-------------------------------------------------------------------------------
	@echo $(notdir $<)
	@$(bin2o)

-include $(DEPENDS)

#---------------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------------
