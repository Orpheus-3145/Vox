LIB_NAME            := libvk.a
CC                  := c++
GLFW                := glfw-3.4

BASE_FLAGS          := -std=c++2b -Wall -Wextra -Werror
CPP_FLAGS           := $(BASE_FLAGS)
DEBUG_FLAGS         := -O0 -g3 -fsanitize=address,undefined -fno-omit-frame-pointer
RELEASE_FLAGS       := -O2 -DNDEBUG -march=native -flto -fno-math-errno -fno-plt -ffast-math -funroll-loops
DEPS_FLAGS          := -MMD -MP -MF


SRC_DIR             := source
BUILD_DIR           := build
DEPS_DIR            := $(BUILD_DIR)/deps
OBJ_DIR             := $(BUILD_DIR)/obj
GLFW_DIR            := $(BUILD_DIR)/$(GLFW)
GLFW_ZIP            := $(BUILD_DIR)/$(GLFW).zip
GLFW_BUILD_DIR      := $(BUILD_DIR)/$(GLFW)/build
EXTERNAL_HEADER_DIR := include/external

INCLUDE             := -Iinclude -I$(EXTERNAL_HEADER_DIR) -I../vectors/include -I$(GLFW_DIR)/include

TARGET              := $(BUILD_DIR)/$(LIB_NAME)

SOURCES              = $(shell find $(SRC_DIR) -type f -name '*.cpp')
OBJECTS              = $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SOURCES))
DEPS                 = $(patsubst $(SRC_DIR)/%.cpp,$(DEPS_DIR)/%.d,$(SOURCES))
GLFW_OBJECTS         = $(shell find $(GLFW_DIR)/build/src/CMakeFiles/glfw.dir -type f -name '*.o')

LAST_MODE           := $(BUILD_DIR)/.last_build_mode
CURR_MODE            = default

.DEFAULT_GOAL := all

check-mode:
	@if [ ! -f $(LAST_MODE) ] || [ "$$(cat $(LAST_MODE))" != "$(CURR_MODE)" ]; then \
		$(MAKE) clean; \
		mkdir -p $(BUILD_DIR); \
		echo "$(CURR_MODE)" > $(LAST_MODE); \
	fi

all: CPP_FLAGS := $(BASE_FLAGS)
all: CURR_MODE := default
all: check-mode $(TARGET)

release: CPP_FLAGS := $(BASE_FLAGS) $(RELEASE_FLAGS)
release: CURR_MODE := release
release: check-mode $(TARGET)

debug: CPP_FLAGS := $(BASE_FLAGS) $(DEBUG_FLAGS)
debug: CURR_MODE := debug
debug: check-mode $(TARGET)

$(OBJ_DIR) $(DEPS_DIR):
	mkdir -p $@

$(EXTERNAL_HEADER_DIR):
	mkdir -p $(EXTERNAL_HEADER_DIR)
	wget --quiet --directory-prefix=$(EXTERNAL_HEADER_DIR) 'https://raw.githubusercontent.com/nothings/stb/refs/heads/master/stb_truetype.h'
	wget --quiet --directory-prefix=$(EXTERNAL_HEADER_DIR) 'https://raw.githubusercontent.com/nothings/stb/refs/heads/master/stb_image.h'

$(GLFW_DIR):
	wget --quiet -O $(GLFW_ZIP) 'https://github.com/glfw/glfw/archive/refs/tags/3.4.zip'
	unzip -qq -a $(GLFW_ZIP) -d $(BUILD_DIR)
	$(RM) $(GLFW_ZIP)
	mkdir -p $(GLFW_BUILD_DIR)
	cmake -S $(GLFW_DIR) -B $(GLFW_BUILD_DIR)
	$(MAKE) -j4 -C $(GLFW_BUILD_DIR)

$(TARGET): $(OBJ_DIR) $(DEPS_DIR) $(GLFW_DIR) $(EXTERNAL_HEADER_DIR) $(OBJECTS)
	ar -rcs $@ $(OBJECTS) $(GLFW_OBJECTS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CC) $(CPP_FLAGS) $(INCLUDE) $(DEPS_FLAGS) $(DEPS_DIR)/$*.d -c $< -o $@

-include $(DEPS)

clean:
	rm -rf $(OBJ_DIR) $(DEPS_DIR)

fclean: clean
	rm -rf $(BUILD_DIR) $(EXTERNAL_HEADER_DIR)

re: fclean all

re-debug: fclean debug

re-release: fclean release

.PHONY: all debug release default clean fclean re re-debug re-release
