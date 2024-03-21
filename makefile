PACKAGER = arduino
ARCH = mbed
BOARD = nano33ble
FQBN = $(PACKAGER):$(ARCH):$(BOARD)

LIBRARIES = AsyncDelay mcp_can
#LIBRARIES += VT100

INCLUDE_FLAGS = $(addprefix -I$${HOME}/Arduino/libraries/, $(LIBRARIES))

BUILD_FLAGS = -DDEBUG_MODE

PORT = /dev/ttyACM0


.PHONY: all
all: upload

.PHONY: upload
upload: compile
	arduino-cli upload -v --fqbn $(FQBN) --port $(PORT)

.PHONY: compile
compile:
	arduino-cli compile -v --fqbn $(FQBN) --build-properties build.extra_flags=$(BUILD_FLAGS)

.PHONY: clean
clean:
	$(RM) tags
	arduino-cli cache clean
	arduino-cli compile --clean --fqbn $(FQBN)

.PHONY: update
update:
	arduino-cli core update-index
	arduino-cli core install $(PACKAGER):$(ARCH)
	arduino-cli lib update-index
	arduino-cli lib upgrade
ifdef LIBRARIES 
	arduino-cli lib install $(LIBRARIES)
endif

.PHONY: terminal
terminal:
	picocom --baud 8000000 $(PORT)

.PHONY: format
format:
	set -x; clang-format -i --style='{BasedOnStyle: LLVM, IndentWidth: 4, MaxEmptyLinesToKeep: 2, ColumnLimit: 130, IndentCaseLabels: true}' $$(find . -iname '*.ino' -or -iname '*.cpp' -or -iname '*.h')

.PHONY: check
check:
	cppcheck --check-config --enable=all --suppress=missingInclude -I. $(INCLUDE_FLAGS) $$(find . -iname '*.ino' -or -iname '*.cpp' -or -iname '*.h')

.PHONY: tags
tags:
	ctags -R . ~/Arduino/libraries/

