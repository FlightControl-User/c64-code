# Incremental KickC build with explicit ordered source batches.
# KickC handles .c/.o timestamps internally, but not dependency ordering between
# generated .p interfaces, so make passes the sources in dependency order.

MODE ?= normal
TARGET ?= -t=C64
VERBOSE ?= 0

# Keep each source batch ordered by interface dependency.
# APP_C_SOURCES order rule:
# - first source contains main()
# - least-dependent sources should be placed later in the list
RESOURCE_C_SOURCES ?= graphics/c64_road.c
APP_C_SOURCES ?= c64_rds.c c64_editor.c c64_machine.c c64_draw.c c64_disk.c
MAIN ?= $(firstword $(APP_C_SOURCES))

WORKSPACE_DIR := $(abspath ..)
USER_DEV := $(abspath ../../x16-code)
DIR_ARG := c64-rds
TARGET_DIR_REL := ../.target/c64-rds
TARGET_DIR_ABS := $(abspath $(TARGET_DIR_REL))

PS := powershell -NoProfile -ExecutionPolicy Bypass -Command


KICKC_DEV := $(subst \,/,$(KICKCDEV))
KICKC_JAR := $(KICKC_DEV)/target/kickc-release.jar
USER_INCLUDE := $(USER_DEV)/x16-inc
USER_LIB := $(USER_DEV)/x16-lib
USER_ASM := $(USER_DEV)/.target/x16-asm
KICKC_STD_INCLUDE := $(KICKC_DEV)/src/main/kc/include
KICKC_STD_LIB := $(KICKC_DEV)/src/main/kc/lib
KICKC_FRAGMENT_HOME := $(KICKC_DEV)/src/main/fragment
KICKC_PLATFORM_HOME := $(KICKC_DEV)/src/main/kc/target

RESOURCE_STAMP := $(TARGET_DIR_REL)/resource.batch.stamp
APP_STAMP := $(TARGET_DIR_REL)/app.batch.stamp

ALL_LOCAL_HEADERS := $(wildcard *.h graphics/*.h)

define scan_includes_ext
$(strip $(shell $(PS) "$$ErrorActionPreference='Stop'; $$root = ([System.IO.Path]::GetFullPath((Get-Location).Path)); $$rootPrefix = $$root + [System.IO.Path]::DirectorySeparatorChar; $$pending = New-Object 'System.Collections.Generic.Queue[string]'; $$seen = New-Object 'System.Collections.Generic.HashSet[string]'; $$out = New-Object 'System.Collections.Generic.List[string]'; foreach($$f in @('$(1)'.Split(' ', [System.StringSplitOptions]::RemoveEmptyEntries))) { $$pending.Enqueue($$f) }; while($$pending.Count -gt 0) { $$rel = $$pending.Dequeue(); $$full = [System.IO.Path]::GetFullPath((Join-Path $$root $$rel)); if(!(Test-Path $$full)) { continue }; if(-not $$seen.Add($$full)) { continue }; $$parent = Split-Path -Parent $$full; foreach($$line in Get-Content $$full) { $$trim = $$line.TrimStart(); if($$trim.StartsWith('#include \"')) { $$rest = $$trim.Substring(10); $$end = $$rest.IndexOf('\"'); if($$end -gt 0) { $$inc = $$rest.Substring(0, $$end).Replace('\\','/'); if($$inc.EndsWith('.$(2)') -and -not $$out.Contains($$inc)) { [void]$$out.Add($$inc) }; $$cand = [System.IO.Path]::GetFullPath((Join-Path $$parent $$inc)); if((Test-Path $$cand) -and $$cand.StartsWith($$rootPrefix)) { $$relCand = $$cand.Substring($$rootPrefix.Length).Replace('\\','/'); $$pending.Enqueue($$relCand) } } } } }; [Console]::Write(($$out -join ' '))"))
endef

RESOURCE_INCLUDE_H_FILES := $(call scan_includes_ext,$(RESOURCE_C_SOURCES),h)
APP_INCLUDE_H_FILES := $(call scan_includes_ext,$(APP_C_SOURCES),h)

RESOURCE_INCLUDE_DEPS := $(filter $(ALL_LOCAL_HEADERS),$(RESOURCE_INCLUDE_H_FILES))
APP_INCLUDE_DEPS := $(filter $(ALL_LOCAL_HEADERS),$(APP_INCLUDE_H_FILES))

COMMON_KICKC_ARGS := \
	-Xms64M \
	-Xmx512M \
	-jar "$(KICKC_JAR)" \
	"$(TARGET)" \
	-I "$(USER_INCLUDE)" \
	-I "$(KICKC_STD_INCLUDE)" \
	-I "$(USER_ASM)" \
	-I "$(TARGET_DIR_ABS)" \
	-L "$(USER_LIB)" \
	-L "$(KICKC_STD_LIB)" \
	-L "$(TARGET_DIR_ABS)" \
	-F "$(KICKC_FRAGMENT_HOME)" \
	-P "$(KICKC_PLATFORM_HOME)" \
	-a \
	-Sc \
	-Si \
	-Xassembler=-showmem \
	-Xassembler=-symbolfile \
	-odir "$(TARGET_DIR_ABS)"

ifeq ($(MODE),optimized)
MODE_KICKC_ARGS := -v -Ouplift=100 -Ocoalesce
else
MODE_KICKC_ARGS := -Onouplift
endif

define run_compile
$(PS) "$$ErrorActionPreference='Stop'; if(!(Test-Path '$(TARGET_DIR_ABS)')) { [void](New-Item -ItemType Directory -Path '$(TARGET_DIR_ABS)' -Force) }; $$command = 'java $(COMMON_KICKC_ARGS) $(MODE_KICKC_ARGS) $(foreach s,$(1),"$(WORKSPACE_DIR)/$(DIR_ARG)/$(s)" )'; if('$(VERBOSE)' -eq '1') { Write-Host $$command }; Invoke-Expression $$command; if($$LASTEXITCODE -ne 0) { exit $$LASTEXITCODE }"
endef

define run_compile_batch
$(PS) "$$ErrorActionPreference='Stop'; if(!(Test-Path '$(TARGET_DIR_ABS)')) { [void](New-Item -ItemType Directory -Path '$(TARGET_DIR_ABS)' -Force) }; $$command = 'java $(COMMON_KICKC_ARGS) $(MODE_KICKC_ARGS) $(foreach s,$(1),"$(WORKSPACE_DIR)/$(DIR_ARG)/$(s)" )'; if('$(VERBOSE)' -eq '1') { Write-Host $$command }; Invoke-Expression $$command; if($$LASTEXITCODE -ne 0) { exit $$LASTEXITCODE }; Set-Content -Path '$@' -Value ''"
endef

.PHONY: all app resources file normal optimized clean info list

all: app

resources: $(RESOURCE_STAMP)

app: $(APP_STAMP)

$(RESOURCE_STAMP): $(RESOURCE_C_SOURCES) $(filter %.h,$(RESOURCE_INCLUDE_DEPS))
	$(call run_compile_batch,$(RESOURCE_C_SOURCES))

$(APP_STAMP): $(APP_C_SOURCES) $(filter %.h,$(APP_INCLUDE_DEPS)) $(RESOURCE_STAMP)
	$(call run_compile_batch,$(APP_C_SOURCES))

file:
	$(if $(SRC),,$(error Use SRC=<file.c>))
	$(if $(wildcard $(SRC)),,$(error Source file '$(SRC)' not found in $(CURDIR)))
	$(call run_compile,$(SRC))

normal:
	$(MAKE) MODE=normal all

optimized:
	$(MAKE) MODE=optimized all

clean:
	$(PS) "if(Test-Path '$(WORKSPACE_DIR)\\.target\\c64-rds'){ Remove-Item -Recurse -Force '$(WORKSPACE_DIR)\\.target\\c64-rds' }"

list:
	@echo RESOURCE_C_SOURCES=$(RESOURCE_C_SOURCES)
	@echo APP_C_SOURCES=$(APP_C_SOURCES)
	@echo RESOURCE_INCLUDE_DEPS=$(RESOURCE_INCLUDE_DEPS)
	@echo APP_INCLUDE_DEPS=$(APP_INCLUDE_DEPS)

info:
	@echo MODE=$(MODE)
	@echo TARGET=$(TARGET)
	@echo VERBOSE=$(VERBOSE)
	@echo MAIN=$(MAIN)
	@echo RESOURCE_C_SOURCES=$(RESOURCE_C_SOURCES)
	@echo APP_C_SOURCES=$(APP_C_SOURCES)
	@echo WORKSPACE_DIR=$(WORKSPACE_DIR)
	@echo USER_DEV=$(USER_DEV)
	@echo TARGET_DIR_ABS=$(TARGET_DIR_ABS)
	@echo KICKC_DEV=$(KICKC_DEV)
	@echo KICKC_JAR=$(KICKC_JAR)
	@echo RESOURCE_INCLUDE_DEPS=$(RESOURCE_INCLUDE_DEPS)
	@echo APP_INCLUDE_DEPS=$(APP_INCLUDE_DEPS)
