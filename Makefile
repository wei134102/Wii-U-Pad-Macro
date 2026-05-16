#-------------------------------------------------------------------------------
.SUFFIXES:
#-------------------------------------------------------------------------------

ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>/devkitpro")
endif

TOPDIR ?= $(CURDIR)
PARENT_DIR := $(abspath $(TOPDIR)/../Wii-U-Time-Sync)
PARENT_REL := ../Wii-U-Time-Sync

WUPS_DIR := $(PARENT_DIR)/external/libwupsxx/external/WiiUPluginSystem
WUPS_ROOT := $(WUPS_DIR)

LIBNOTIFICATIONS_DIR := $(PARENT_DIR)/external/libwupsxx/external/libnotifications

include $(WUPS_DIR)/share/wups_rules

WUMS_ROOT := $(DEVKITPRO)/wums
WUT_ROOT := $(DEVKITPRO)/wut

#-------------------------------------------------------------------------------
PLUGIN_NAME    := GamePad Macro Demo
PLUGIN_VERSION := v0.1.0

TARGET   := GamePad_Macro_Demo
BUILD    := build

INCLUDES := \
	include \
	$(PARENT_REL)/external/libwupsxx/include

DATA := data

DEBUG := 0

#-------------------------------------------------------------------------------
CXX += -std=c++23

WARN_FLAGS := -Wall -Wextra -Wundef -Wpointer-arith -Wcast-align -Wno-odr

OPTFLAGS := -Os -fipa-pta -ffunction-sections -fdata-sections -flto

CFLAGS := $(WARN_FLAGS) $(OPTFLAGS) $(MACHDEP) -pthread

CXXFLAGS := $(CFLAGS)

DEFINES := '-DPLUGIN_NAME="$(PLUGIN_NAME)"'                   \
           '-DPLUGIN_VERSION="$(PLUGIN_VERSION)"'

CPPFLAGS = $(INCLUDE) -D__WIIU__ -D__WUT__ -D__WUPS__  $(DEFINES)

ASFLAGS	:= -g $(ARCH)

LDFLAGS	= -g \
          $(ARCH) \
          $(RPXSPECS) \
          $(WUPSSPECS) \
          -Wl,-Map,$(notdir $*.map) \
          $(CXXFLAGS)

LIBS := \
	-lnotifications \
	-lwups \
	-lwut

LIBDIRS	:= \
	$(LIBNOTIFICATIONS_DIR) \
	$(PORTLIBS) \
	$(WUPS_DIR) \
	$(WUPS_ROOT) \
	$(WUT_ROOT)

#-------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))
#-------------------------------------------------------------------------------

export TOPDIR	:=	$(CURDIR)
export OUTPUT	:=	$(TOPDIR)/$(TARGET)
export VPATH	:=	$(TOPDIR)
export DEPSDIR	:=	$(TOPDIR)/$(BUILD)

SOURCES_EXCLUDE := \
	$(PARENT_REL)/external/libwupsxx/src/shortcut.cpp \
	$(PARENT_REL)/external/libwupsxx/src/shortcut_item.cpp

LIBWUPSXX_CPP := $(filter-out $(SOURCES_EXCLUDE),$(wildcard $(PARENT_REL)/external/libwupsxx/src/*.cpp))

export CPPFILES := \
	source/main.cpp \
	source/cfg.cpp \
	source/macro.cpp \
	source/macro_script.cpp \
	source/macro_editor_item.cpp \
	source/macro_sd.cpp \
	source/macro_sd_item.cpp \
	source/i18n.cpp \
	source/i18n_items.cpp \
	source/vpad_hook.cpp \
	$(LIBWUPSXX_CPP)

export CFILES :=
export SFILES :=

export OFILES_BIN	:=
export OFILES_SRC	:= $(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)
export OFILES		:= $(OFILES_BIN) $(OFILES_SRC)
export HFILES_BIN	:=

export INCLUDE	:=	$(foreach dir,$(INCLUDES),-I$(TOPDIR)/$(dir)) \
			$(foreach dir,$(LIBDIRS),-I$(dir)/include)

export LIBPATHS	:=	$(foreach dir,$(LIBDIRS),-L$(dir)/lib)

export LD	:=	$(CXX)

.PHONY: $(BUILD) clean all

#-------------------------------------------------------------------------------
all: $(BUILD)

$(BUILD):
	@$(MAKE) -C $(LIBNOTIFICATIONS_DIR) TOPDIR=$(LIBNOTIFICATIONS_DIR)
	@$(MAKE) -C $(WUPS_DIR) lib/libwups.a TOPDIR=$(WUPS_DIR)
	@$(shell [ -d $@ ] || mkdir -p $@)
	mkdir -p $(addprefix build/,$(sort $(dir $(OFILES))))
	$(MAKE) -C $(BUILD) -f $(TOPDIR)/Makefile V=$(DEBUG)

#-------------------------------------------------------------------------------
clean:
	$(info clean ...)
	$(RM) -r $(BUILD) $(TARGET).wps $(TARGET).elf

#-------------------------------------------------------------------------------
else
.PHONY:	all

DEPENDS	:=	$(OFILES:.o=.d)

#-------------------------------------------------------------------------------
all	:	$(OUTPUT).wps

$(OUTPUT).wps	:	$(OUTPUT).elf
$(OUTPUT).elf	:	$(OFILES)

$(OFILES_SRC)	: $(HFILES_BIN)

-include $(DEPENDS)

#-------------------------------------------------------------------------------
endif
#-------------------------------------------------------------------------------
