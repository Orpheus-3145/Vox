LIB_NAME            := libvk.a
CC                  := c++
AR                  := ar
RM                  := rm -rf

MODE                ?= default

BASE_FLAGS          := -std=c++2b -Wall -Wextra -Werror
DEFAULT_FLAGS       :=
DEBUG_FLAGS         := -O0 -g3 -fsanitize=address,undefined -fno-omit-frame-pointer
RELEASE_FLAGS       := -O2 -DNDEBUG -march=native -flto -fno-math-errno -fno-plt -ffast-math -funroll-loops
DEPS_FLAGS          := -MMD -MP -MF

SRC_DIR             := source
GLFW_DIR            := build/glfw
BUILD_ROOT          := build/$(MODE)
OBJ_DIR             := $(BUILD_ROOT)/obj
DEPS_DIR            := $(BUILD_ROOT)/deps
TARGET              := $(BUILD_ROOT)/$(LIB_NAME)

SOURCES             := $(shell find $(SRC_DIR) -type f -name '*.cpp')
OBJECTS             := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SOURCES))
GLFW_OBJECTS         = $(shell find $(GLFW_DIR)/build/src/CMakeFiles/glfw.dir -type f -name '*.o')
DEPS                := $(patsubst $(SRC_DIR)/%.cpp,$(DEPS_DIR)/%.d,$(SOURCES))

EXTERNAL_LIBRARIES  := include/external
INCLUDE             := -Iinclude -I$(EXTERNAL_LIBRARIES) -I../vectors/include -I$(GLFW_DIR)/include
PLATFORM            := $(shell uname -s)

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

$(EXTERNAL_LIBRARIES):
	mkdir -p $(EXTERNAL_LIBRARIES)
	wget --quiet --directory-prefix=$(EXTERNAL_LIBRARIES) 'https://raw.githubusercontent.com/nothings/stb/refs/heads/master/stb_truetype.h'
	wget --quiet --directory-prefix=$(EXTERNAL_LIBRARIES) 'https://raw.githubusercontent.com/nothings/stb/refs/heads/master/stb_image.h'

$(GLFW_DIR):
	wget --quiet -O glfw-3.4.zip 'https://github.com/glfw/glfw/archive/refs/tags/3.4.zip'
	unzip -qq -a glfw-3.4.zip -d .
	$(RM) glfw-3.4.zip
	mv glfw-3.4 $(GLFW_DIR)
	mkdir -p $(GLFW_DIR)/build
	cmake --log-level=ERROR -S $(GLFW_DIR) -B $(GLFW_DIR)/build
	$(MAKE) -j4 -C $(GLFW_DIR)/build > /dev/null 2>&1

$(TARGET): $(OBJ_DIR) $(DEPS_DIR) $(GLFW_DIR) $(EXTERNAL_LIBRARIES) $(OBJECTS)
	$(AR) -rcs $@ $(OBJECTS) $(GLFW_OBJECTS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@) $(dir $(DEPS_DIR)/$*.d)
	$(CC) $(CPP_FLAGS) $(INCLUDE) $(DEPS_FLAGS) $(DEPS_DIR)/$*.d -c $< -o $@

-include $(DEPS)

clean:
	$(RM) build

fclean: clean
	$(RM) $(EXTERNAL_LIBRARIES)

re: fclean all
re-debug: fclean debug
re-release: fclean release

.PHONY: all debug release default clean fclean re re-debug re-release