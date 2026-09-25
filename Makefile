# Top-level helper Makefile for CCS-managed F28P65x builds and UniFlash.

PROJECT_NAME ?= f28p65x-soes
#CONFIG ?= FLASH
CONFIG ?= LAUNCHXL_FLASH

HOST_OS ?= $(shell uname -s)
ifeq ($(HOST_OS),Linux)
include Makefile.linux_local
else ifeq ($(HOST_OS),Darwin)
include Makefile.darwin_local
else
$(error Unsupported HOST_OS '$(HOST_OS)': expected Linux or Darwin)
endif

CCS_WORKSPACE ?= /tmp/$(USER)-$(PROJECT_NAME)-ccs-workspace

TARGET_CONFIG_FILE ?= $(CURDIR)/TMS320F28P650DK9.ccxml
CORE ?= 0

OUT_FILE := $(CURDIR)/$(CONFIG)/$(PROJECT_NAME).out

CCS_BUILD = "$(CCS_CLI)" \
	-workspace "$(CCS_WORKSPACE)" \
	-application projectBuild \
	-ccs.locations "$(CURDIR)" \
	-ccs.configuration "$(CONFIG)" \
	-ccs.autoImport \
	-ccs.autoOpen \
	-ccs.listProblems

DSLITE_FLASH = "$(DSLITE)" \
	--config="$(TARGET_CONFIG_FILE)" \
	--core=$(CORE) \
	--flash \
	--verify

.PHONY: help build clean rebuild flash flash-only flash-run list-cores list-ops

help:
	@echo "Usage: make <target> [VAR=value]"
	@echo "Host platform: $(HOST_OS) (defaults in Makefile.*_local)"
	@echo ""
	@echo "Targets:"
	@echo "  build       Build the CCS $(CONFIG) configuration"
	@echo "  clean       Clean the CCS $(CONFIG) configuration"
	@echo "  rebuild     Clean, then perform a full build"
	@echo "  flash       Build, program, and verify $(OUT_FILE)"
	@echo "  flash-only Program and verify the existing .out without building"
	@echo "  flash-run   Build, program, verify, and run CPU core $(CORE)"
	@echo "  list-cores  List target cores and their UniFlash indexes"
	@echo "  list-ops    List target-specific UniFlash operations"
	@echo ""
	@echo "Common overrides:"
	@echo "  CONFIG=FLASH"
	@echo "  CCS_INSTALL_DIR=/path/to/ccs-install"
	@echo "  CCS_WORKSPACE=/path/to/headless-workspace"
	@echo "  DSLITE=/path/to/dslite.sh"
	@echo "  TARGET_CONFIG_FILE=/path/to/device.ccxml"
	@echo "  CORE=0"

build:
	@test -x "$(CCS_CLI)" || { echo "CCS CLI not found: $(CCS_CLI)"; exit 1; }
	@$(CCS_BUILD) -ccs.buildType incremental

clean:
	@test -x "$(CCS_CLI)" || { echo "CCS CLI not found: $(CCS_CLI)"; exit 1; }
	@$(CCS_BUILD) -ccs.buildType clean

rebuild: clean
	@$(CCS_BUILD) -ccs.buildType full

flash: build
	@test -x "$(DSLITE)" || { echo "UniFlash CLI not found: $(DSLITE)"; exit 1; }
	@test -f "$(TARGET_CONFIG_FILE)" || { echo "Target configuration not found: $(TARGET_CONFIG_FILE)"; exit 1; }
	@test -f "$(OUT_FILE)" || { echo "Build output not found: $(OUT_FILE)"; exit 1; }
	@$(DSLITE_FLASH) "$(OUT_FILE)"
	@echo "Programmed and verified: $(OUT_FILE)"

flash-only:
	@test -x "$(DSLITE)" || { echo "UniFlash CLI not found: $(DSLITE)"; exit 1; }
	@test -f "$(TARGET_CONFIG_FILE)" || { echo "Target configuration not found: $(TARGET_CONFIG_FILE)"; exit 1; }
	@test -f "$(OUT_FILE)" || { echo "Build output not found: $(OUT_FILE)"; exit 1; }
	@$(DSLITE_FLASH) "$(OUT_FILE)"
	@echo "Programmed and verified: $(OUT_FILE)"

flash-run: build
	@test -x "$(DSLITE)" || { echo "UniFlash CLI not found: $(DSLITE)"; exit 1; }
	@test -f "$(TARGET_CONFIG_FILE)" || { echo "Target configuration not found: $(TARGET_CONFIG_FILE)"; exit 1; }
	@test -f "$(OUT_FILE)" || { echo "Build output not found: $(OUT_FILE)"; exit 1; }
	@$(DSLITE_FLASH) --run "$(OUT_FILE)"
	@echo "Programmed, verified, and started: $(OUT_FILE)"

list-cores:
	@test -x "$(DSLITE)" || { echo "UniFlash CLI not found: $(DSLITE)"; exit 1; }
	@"$(DSLITE)" --config="$(TARGET_CONFIG_FILE)" --list-cores

list-ops:
	@test -x "$(DSLITE)" || { echo "UniFlash CLI not found: $(DSLITE)"; exit 1; }
	@"$(DSLITE)" --config="$(TARGET_CONFIG_FILE)" --list-ops
