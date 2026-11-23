# Tracker Enhanced (ENH) Centralized Version Configuration

EDITION_NAME := ENH
VER_ROOT_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
BUILD_TAG    ?= $(shell git -C "$(VER_ROOT_DIR)" rev-parse --short=7 HEAD 2>/dev/null || date +"%Y%m%d")

# *_VER_STAGE: alpha, beta or rc. *_VER_PRE: its number, or 255 for a full release.
# A pre-release adds e.g. "a3", "b1" or "rc1" to the .ptf name.
pre_suffix = $(if $(filter 255,$(2)),,$(if $(filter alpha,$(1)),a,$(if $(filter rc,$(1)),rc,b))$(2))

# Original Tracker (MK66)
ORIGINAL_VER_MAJOR := 0
ORIGINAL_VER_MINOR := 1
ORIGINAL_VER_PATCH := 0
ORIGINAL_VER_STAGE := rc
ORIGINAL_VER_PRE   := 1

# Tracker+ (IMX)
PLUS_VER_MAJOR     := 0
PLUS_VER_MINOR     := 1
PLUS_VER_PATCH     := 0
PLUS_VER_STAGE     := rc
PLUS_VER_PRE       := 1

# Tracker Mini (IMX)
MINI_VER_MAJOR     := 0
MINI_VER_MINOR     := 1
MINI_VER_PATCH     := 0
MINI_VER_STAGE     := rc
MINI_VER_PRE       := 1

$(foreach t,ORIGINAL PLUS MINI,$(if $(filter alpha beta rc,$($(t)_VER_STAGE)),,\
  $(error $(t)_VER_STAGE must be alpha, beta or rc, not "$($(t)_VER_STAGE)")))

ifeq ($(PRODUCT), MINI)
  FW_VER_1    := $(MINI_VER_MAJOR)
  FW_VER_2    := $(MINI_VER_MINOR)
  FW_VER_3    := $(MINI_VER_PATCH)
  FW_STAGE    := $(MINI_VER_STAGE)
  FW_PRE      := $(MINI_VER_PRE)
else ifeq ($(PRODUCT), PLUS)
  FW_VER_1    := $(PLUS_VER_MAJOR)
  FW_VER_2    := $(PLUS_VER_MINOR)
  FW_VER_3    := $(PLUS_VER_PATCH)
  FW_STAGE    := $(PLUS_VER_STAGE)
  FW_PRE      := $(PLUS_VER_PRE)
else
  FW_VER_1    := $(ORIGINAL_VER_MAJOR)
  FW_VER_2    := $(ORIGINAL_VER_MINOR)
  FW_VER_3    := $(ORIGINAL_VER_PATCH)
  FW_STAGE    := $(ORIGINAL_VER_STAGE)
  FW_PRE      := $(ORIGINAL_VER_PRE)
endif

FW_VERSION_DEFINES := -DFW_EDITION_NAME=\"$(EDITION_NAME)\" \
                      -DFW_VER_MAJOR=$(FW_VER_1) \
                      -DFW_VER_MINOR=$(FW_VER_2) \
                      -DFW_VER_PATCH=$(FW_VER_3) \
                      -DFW_VER_BETA=$(FW_PRE) \
                      $(if $(filter alpha,$(FW_STAGE)),-DFW_VER_ALPHA) \
                      $(if $(filter rc,$(FW_STAGE)),-DFW_VER_RC)

BUILD_ID_STR           := $(BUILD_TAG)
DEFAULT_TARGET_POSTFIX := $(EDITION_NAME)_$(FW_VER_1).$(FW_VER_2).$(FW_VER_3)$(call pre_suffix,$(FW_STAGE),$(FW_PRE))
