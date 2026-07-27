TARGET          := ft_vox
CC              := c++
RM              := rm -rf

MODE            ?= default

BASE_FLAGS      := -std=c++2b -Wall -Wextra -Werror
DEFAULT_FLAGS   :=
DEBUG_FLAGS     := -O0 -g3 -fsanitize=address,undefined -fno-omit-frame-pointer
RELEASE_FLAGS   := -O2 -DNDEBUG -march=native -flto -fno-math-errno -fno-plt -ffast-math -funroll-loops
# -flto				--> apply optimizations between different .o files
# -fno-math-errno	--> do not update errno variable if cmath functions fail
# -fno-plt 			--> optimize calls to linked libs functions
# -ffast-math		-->	approximation math for floating points
# -funroll-loops	-->	unpack loops
DEPS_FLAGS      := -MMD -MP -MF

GLSLC           := $(shell which glslc)

SRC_DIR         := source
SHADERS_DIR     := shaders
VECTOR_DIR      := lib/vectors
VULKAN_DIR      := lib/vulkan

BUILD_ROOT      := build/$(MODE)
OBJ_DIR         := $(BUILD_ROOT)/obj
DEPS_DIR        := $(BUILD_ROOT)/deps
SHADERS_OUT_DIR := build
TARGET_PATH     := $(BUILD_ROOT)/$(TARGET)

SOURCES         := $(shell find $(SRC_DIR) -type f -name '*.cpp')
OBJECTS         := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SOURCES))
DEPS            := $(patsubst $(SRC_DIR)/%.cpp,$(DEPS_DIR)/%.d,$(SOURCES))

SHADERS_SRC     := $(shell find $(SHADERS_DIR) -type f)
SHADERS_OBJ     := $(patsubst $(SHADERS_DIR)/%,$(SHADERS_OUT_DIR)/%.spv,$(SHADERS_SRC))

INCLUDE         := -Iinclude -I$(VECTOR_DIR)/include -I$(VULKAN_DIR)/include -I$(VULKAN_DIR)/include/external

LIBS            :=	$(VULKAN_DIR)/build/$(MODE)/libvk.a \
					$(VECTOR_DIR)/build/$(MODE)/libvectors.a

SYS_LIBS        := -lvulkan
PLATFORM        := $(shell uname -s)

ifeq ($(PLATFORM),Linux)
	SYS_LIBS += -lGL -lX11 -lpthread -lXrandr -lXi 
else ifeq ($(PLATFORM),Darwin)
INCLUDE  += -isystem /opt/homebrew/include -isystem /usr/local/include
SYS_LIBS += -L/opt/homebrew/lib -Wl,-rpath,/usr/local/lib -framework Cocoa -framework IOKit -framework OpenGL -lglfw3
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

all: libs $(TARGET_PATH)

default:
	$(MAKE) MODE=default all

debug:
	$(MAKE) MODE=debug all

release:
	$(MAKE) MODE=release all

libs:
	$(MAKE) -C $(VECTOR_DIR) MODE=$(MODE)
	$(MAKE) -C $(VULKAN_DIR) MODE=$(MODE)

run: all
	./$(TARGET_PATH)

run-debug:
	$(MAKE) MODE=debug run

run-release:
	$(MAKE) MODE=release run

$(OBJ_DIR) $(DEPS_DIR) $(SHADERS_OUT_DIR):
	mkdir -p $@

$(TARGET_PATH): $(LIBS) $(OBJ_DIR) $(DEPS_DIR) $(SHADERS_OUT_DIR) $(SHADERS_OBJ) $(OBJECTS)
	$(CC) $(CPP_FLAGS) $(INCLUDE) $(OBJECTS) $(LIBS) $(SYS_LIBS) -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@) $(dir $(DEPS_DIR)/$*.d)
	$(CC) $(CPP_FLAGS) $(INCLUDE) $(DEPS_FLAGS) $(DEPS_DIR)/$*.d -c $< -o $@

$(SHADERS_OUT_DIR)/%.spv: $(SHADERS_DIR)/%
	@mkdir -p $(dir $@)
	$(GLSLC) $< -o $@

-include $(DEPS)

clean:
	$(RM) build
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