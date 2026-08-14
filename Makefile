TARGET          := vox
CC              := c++

BASE_FLAGS      := -std=c++2b -Wall -Wextra -Werror# NB set it to c++20
CPP_FLAGS       := $(BASE_FLAGS)
DEBUG_FLAGS     := -O0 -g3 -fsanitize=address,undefined -fno-omit-frame-pointer
RELEASE_FLAGS   := -O2 -DNDEBUG -march=native -flto -fno-math-errno -fno-plt -ffast-math -funroll-loops
DEPS_FLAGS      := -MMD -MP -MF

GLSLC            = $(shell which glslc)

SRC_DIR         := source
SHADERS_DIR     := shaders
VECTOR_DIR      := libs/vectors
VULKAN_DIR      := libs/vulkan
BUILD_DIR       := build
SHADERS_OUT_DIR := $(BUILD_DIR)/shaders
DEPS_DIR        := $(BUILD_DIR)/deps
OBJ_DIR         := $(BUILD_DIR)/obj

INCLUDE         := -Iinclude -I$(VECTOR_DIR)/include -I$(VULKAN_DIR)/include -I$(VULKAN_DIR)/include/external

SOURCES          = $(shell find $(SRC_DIR) -type f -name '*.cpp')
OBJECTS          = $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SOURCES))
DEPS             = $(patsubst $(SRC_DIR)/%.cpp,$(DEPS_DIR)/%.d,$(SOURCES))

SHADERS_SRC      = $(shell find $(SHADERS_DIR) -type f)
SHADERS_OBJ      = $(patsubst $(SHADERS_DIR)/%,$(SHADERS_OUT_DIR)/%.spv,$(SHADERS_SRC))

SYS_LIBS        := -lvulkan -lGL -lX11 -lpthread -lXrandr -lXi
VK_LIBS         := $(VULKAN_DIR)/build/libvk.a $(VECTOR_DIR)/build/libvectors.a

LAST_MODE       := $(BUILD_DIR)/.last_build_mode
CURR_MODE       := default

.DEFAULT_GOAL := all

check-mode:
	@if [ ! -f $(LAST_MODE) ] || [ "$$(cat $(LAST_MODE))" != "$(CURR_MODE)" ]; then \
		$(MAKE) clean; \
		mkdir -p $(BUILD_DIR); \
		echo "$(CURR_MODE)" > $(LAST_MODE); \
	fi

run: all
	./$(TARGET)

all: CPP_FLAGS := $(BASE_FLAGS)
all: CURR_MODE := default
all: check-mode libs $(TARGET)

release: CPP_FLAGS := $(BASE_FLAGS) $(RELEASE_FLAGS)
release: CURR_MODE := release
release: check-mode libs-release $(TARGET)

debug: CPP_FLAGS := $(BASE_FLAGS) $(DEBUG_FLAGS)
debug: CURR_MODE := debug
debug: check-mode libs-debug $(TARGET)

libs:
	$(MAKE) -C $(VECTOR_DIR)
	$(MAKE) -C $(VULKAN_DIR)

libs-release:
	$(MAKE) -C $(VECTOR_DIR) release
	$(MAKE) -C $(VULKAN_DIR) release

libs-debug:
	$(MAKE) -C $(VECTOR_DIR) debug
	$(MAKE) -C $(VULKAN_DIR) debug

$(OBJ_DIR) $(DEPS_DIR) $(SHADERS_OUT_DIR):
	mkdir -p $@

$(TARGET): $(VK_LIBS) $(OBJ_DIR) $(DEPS_DIR) $(SHADERS_OUT_DIR) $(SHADERS_OBJ) $(OBJECTS)
	$(CC) $(CPP_FLAGS) $(INCLUDE) $(OBJECTS) $(VK_LIBS) $(SYS_LIBS) -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CC) $(CPP_FLAGS) $(INCLUDE) $(DEPS_FLAGS) $(DEPS_DIR)/$*.d -c $< -o $@

$(SHADERS_OUT_DIR)/%.spv: $(SHADERS_DIR)/%
	$(GLSLC) $< -o $@

-include $(DEPS)

clean:
	rm -rf $(BUILD_DIR)
	$(MAKE) -C $(VECTOR_DIR) clean
	$(MAKE) -C $(VULKAN_DIR) clean

fclean: clean
	$(MAKE) -C $(VECTOR_DIR) fclean
	$(MAKE) -C $(VULKAN_DIR) fclean

re: fclean all

rerun: fclean run

re-debug: fclean debug

re-release: fclean release

.PHONY: all default debug release libs run run-debug run-release clean fclean re re-debug re-release
