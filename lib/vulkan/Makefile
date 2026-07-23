LIB_NAME        := libvk.a
CC              := c++
AR              := ar
RM              := rm -rf

MODE            ?= default

BASE_FLAGS      := -std=c++2b -Wall -Wextra -Werror
DEFAULT_FLAGS   :=
DEBUG_FLAGS     := -O0 -g3 -fsanitize=address,undefined -fno-omit-frame-pointer
RELEASE_FLAGS   := -O2 -DNDEBUG -march=native -flto -fno-math-errno -fno-plt -ffast-math -funroll-loops
DEPS_FLAGS      := -MMD -MP -MF

SRC_DIR         := source
BUILD_ROOT      := build/$(MODE)
OBJ_DIR         := $(BUILD_ROOT)/obj
DEPS_DIR        := $(BUILD_ROOT)/deps
TARGET          := $(BUILD_ROOT)/$(LIB_NAME)

SOURCES         := $(shell find $(SRC_DIR) -type f -name '*.cpp')
OBJECTS         := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SOURCES))
DEPS            := $(patsubst $(SRC_DIR)/%.cpp,$(DEPS_DIR)/%.d,$(SOURCES))

INCLUDE         := -Iinclude -I../vectors/include
PLATFORM        := $(shell uname -s)

ifeq ($(PLATFORM),Darwin)
INCLUDE += -isystem /opt/homebrew/include -isystem /usr/local/include
endif

ifeq ($(MODE),default)
MODE_FLAGS := $(DEFAULT_FLAGS)
else ifeq ($(MODE),debug)
MODE_FLAGS := $(DEBUG_FLAGS)
else ifeq ($(MODE),release)
MODE_FLAGS := $(RELEASE_FLAGS)
else
$(error Unknown MODE='$(MODE)'. Use MODE=default|debug|release)
endif

CPP_FLAGS := $(BASE_FLAGS) $(MODE_FLAGS)

all: $(TARGET)

debug:
	$(MAKE) MODE=debug all

release:
	$(MAKE) MODE=release all

default:
	$(MAKE) MODE=default all

$(OBJ_DIR) $(DEPS_DIR):
	mkdir -p $@

$(TARGET): $(OBJ_DIR) $(DEPS_DIR) $(OBJECTS)
	$(AR) -rcs $@ $(OBJECTS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@) $(dir $(DEPS_DIR)/$*.d)
	$(CC) $(CPP_FLAGS) $(INCLUDE) $(DEPS_FLAGS) $(DEPS_DIR)/$*.d -c $< -o $@

-include $(DEPS)

clean:
	$(RM) build

fclean: clean

re: fclean all
re-debug: fclean debug
re-release: fclean release

.PHONY: all debug release default clean fclean re re-debug re-release