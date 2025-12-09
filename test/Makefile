
# =========================
# User-configurable section
# =========================
# Build type: Debug or Release
BUILD ?= Debug

# Toolchain
CXX ?= g++
CC  ?= gcc

# Project
TARGET      := test
SRC_DIR     := ../src
INC_DIR     := ../include
LINK_SCRIPT := ulog_x86_64_linux_gnu.ld

# Sources
CXX_SRCS := $(SRC_DIR)/ulog_linux.cpp test.cpp
C_SRCS   := $(SRC_DIR)/ulog.c test_c.c

# Optionally list static archives here (if you have circular deps)
# e.g., STATIC_LIBS := /path/to/libulog_core.a /path/to/libulog_platform.a
STATIC_LIBS :=

# System libs
SYS_LIBS := -lpthread

# =========================
# Flags (per build type)
# =========================
# Common warnings & features
COMMON_WARN   := -Wall -Wextra -Werror -Wshadow -Wconversion -Wstrict-aliasing
COMMON_DEFS   :=
COMMON_STD_CXX:= -std=c++17
COMMON_STD_C  := -std=c11

# Dependency generation
DEPFLAGS := -MMD -MP

# Debug vs Release
ifeq ($(BUILD),Debug)
  CXX_OPT := -Og
  C_OPT   := -Og
  DBGFLAGS := -g3 -fno-omit-frame-pointer
  SAVE_TEMPS ?= -save-temps            # keep .i/.s/.o (disable by setting SAVE_TEMPS=)
  SANITIZERS ?= -fsanitize=address,undefined # disable by SANITIZERS=
  LTO ?=
else
  CXX_OPT := -O2
  C_OPT   := -O2
  DBGFLAGS :=
  SAVE_TEMPS ?=
  SANITIZERS ?=
  LTO ?= -flto
endif

# You can toggle verbose link (show full command)
VERBOSE ?= 0

# Optional: ccache
# CXX := ccache g++
# CC  := ccache gcc

# =========================
# Derived variables
# =========================
CXXFLAGS := $(COMMON_STD_CXX) $(CXX_OPT) $(DBGFLAGS) $(COMMON_WARN) $(DEPFLAGS) -I$(INC_DIR) -fdata-sections
CFLAGS   := $(COMMON_STD_C)  $(C_OPT)   $(DBGFLAGS) $(COMMON_WARN) $(DEPFLAGS) -I$(INC_DIR) -fdata-sections

# Linker flags
LDFLAGS  := $(SANITIZERS) $(LTO) -Wl,-T,$(LINK_SCRIPT)
# If you need circular deps in static libs, enable START_GROUP=1 at invocation:
# make START_GROUP=1
ifeq ($(START_GROUP),1)
  LIBGROUP_OPEN  := -Wl,--start-group
  LIBGROUP_CLOSE := -Wl,--end-group
else
  LIBGROUP_OPEN  :=
  LIBGROUP_CLOSE :=
endif

# Save temps only in compile phases; not strictly needed on link, but harmless
CXXFLAGS += $(SAVE_TEMPS)
CFLAGS   += $(SAVE_TEMPS)

# Objects & deps
CXX_OBJS := $(patsubst %.cpp,%.o,$(CXX_SRCS))
C_OBJS   := $(patsubst %.c,%.o,$(C_SRCS))
OBJS     := $(CXX_OBJS) $(C_OBJS)
DEPS     := $(OBJS:.o=.d)

# =========================
# Default target
# =========================
.PHONY: all
all: $(TARGET)

# =========================
# Link rule
# =========================
# Put objects first, then archives, then system libs. Group archives if needed.
$(TARGET): $(OBJS)
ifeq ($(VERBOSE),1)
	$(CXX) -o $@ $(OBJS) $(LIBGROUP_OPEN) $(STATIC_LIBS) $(LIBGROUP_CLOSE) $(SYS_LIBS) $(LDFLAGS)
else
	@echo "[LD] $@"
	@$(CXX) -o $@ $(OBJS) $(LIBGROUP_OPEN) $(STATIC_LIBS) $(LIBGROUP_CLOSE) $(SYS_LIBS) $(LDFLAGS)
endif

# =========================
# Compile rules (with deps)
# =========================
# C++
%.o: %.cpp
ifeq ($(VERBOSE),1)
	$(CXX) -c $(CXXFLAGS) -o $@ $<
else
	@echo "[CXX] $<"
	@$(CXX) -c $(CXXFLAGS) -o $@ $<
endif

# C
%.o: %.c
ifeq ($(VERBOSE),1)
	$(CC) -c $(CFLAGS) -o $@ $<
else
	@echo "[CC ] $<"
	@$(CC) -c $(CFLAGS) -o $@ $<
endif

# =========================
# Convenience targets
# =========================
.PHONY: run
run: $(TARGET)
	./$(TARGET)

.PHONY: clean
clean:
	@echo "[CLEAN]"
	@rm -f $(OBJS) $(DEPS) $(TARGET) \
	       $(CXX_SRCS:.cpp=.s) $(CXX_SRCS:.cpp=.i) \
	       $(C_SRCS:.c=.s) $(C_SRCS:.c=.i)

.PHONY: distclean
distclean: clean
	@echo "[DISTCLEAN]"
	@find . -name "*~" -o -name "*.tmp" -o -name "*.log" -delete

# Disassemble target (requires objdump)
.PHONY: disasm
disasm: $(TARGET)
	@echo "[OBJDUMP] $(TARGET)"
	@objdump -d -C $(TARGET) | less

# ELF info (requires readelf)
.PHONY: elfinfo
elfinfo: $(TARGET)
	@echo "[READELF] $(TARGET)"
	@readelf -S -l $(TARGET)

# =========================
# Build-type shortcuts
# =========================
.PHONY: debug release
debug:
	$(MAKE) BUILD=Debug VERBOSE=$(VERBOSE) START_GROUP=$(START_GROUP) all
release:
	$(MAKE) BUILD=Release VERBOSE=$(VERBOSE) START_GROUP=$(START_GROUP) all

# =========================
# Include auto-generated deps
# =========================
-include $(DEPS)
