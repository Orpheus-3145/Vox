TARGET			:=	ft_vox

CC				:=	c++
CPP_FLAGS		:=	-std=c++2b -Wall -Wextra -Werror
DEBUG_FLAGS		:=	-O0 -g3 -fsanitize=address,undefined -fno-omit-frame-pointer
RELEASE_FLAGS	:=	-O3 -DNDEBUG -march=native -flto -fno-math-errno -fno-plt -fno-rtti -ffast-math -funroll-loops
# -flto				--> apply optimizations between different .o files
# -fno-math-errno	--> do not update errno variable if cmath functions fail
# -fno-plt 			--> optimize calls to linked libs functions
# -fno-rtti			-->	use this only if dynamic_casts are not used
# -ffast-math		-->	approximation math for floating points
# -funroll-loops	-->	unpack loops
DEPS_FLAGS		:=	-MMD -MP -MF
GLSLC			:=	$(shell which glslc)

SRC_DIR		:=	source
BUILD_DIR	:=	build
OBJ_DIR		:=	$(BUILD_DIR)/obj
DEPS_DIR	:=	$(BUILD_DIR)/deps
SHADERS_DIR	:=	shaders
VECTOR_DIR	:=	lib/vectors
VULKAN_DIR	:=	lib/vulkan

SOURCES		:=	$(shell find $(SRC_DIR) -type f -name '*.cpp')
OBJECTS		:=	$(addprefix $(OBJ_DIR)/,$(notdir $(SOURCES:%.cpp=%.o)))
DEPS		:=	$(patsubst $(SRC_DIR)%,$(DEPS_DIR)%,$(SOURCES:.cpp=.d))

SHADERS_SRC	:=	$(shell ls $(SHADERS_DIR))
SHADERS_OBJ	:=	$(addprefix $(BUILD_DIR)/,$(addsuffix .spv,$(SHADERS_SRC)))

INCLUDE 	:=	-Iinclude \
				-I$(VECTOR_DIR)/include \
				-I$(VULKAN_DIR)/include

LIBS		:=	$(VECTOR_DIR)/build/libvectors.a $(VULKAN_DIR)/build/libvk.a
SYS_LIBS	:=	-lvulkan

PLATFORM	:=	$(shell uname -s)

ifeq ($(PLATFORM), Linux)
	SYS_LIBS	+= -lGL -lX11 -lpthread -lXrandr -lXi $(shell pkg-config --static --libs glfw3)

else ifeq ($(PLATFORM), Darwin)
	INCLUDE		+= -isystem /opt/homebrew/include -isystem /usr/local/include
	SYS_LIBS	+= -L/opt/homebrew/lib -Wl,-rpath,$(/usr/local/lib) -framework Cocoa -framework IOKit -framework OpenGL

endif

# source /opt/vulkan/current/setup-env.sh


all: libs $(TARGET)

libs:
	$(MAKE) -C $(VECTOR_DIR)
	$(MAKE) -C $(VULKAN_DIR)

run: all
	./$(TARGET)

rerun: re run

release: CPP_FLAGS += $(RELEASE_FLAGS)
release: libs-release $(TARGET)

libs-release: 
	$(MAKE) -C $(VECTOR_DIR) release
	$(MAKE) -C $(VULKAN_DIR) release

run-release: release
	./$(TARGET)

rerun-release: re run-release

debug: CPP_FLAGS += $(DEBUG_FLAGS)
debug: libs-debug $(TARGET)

libs-debug: 
	$(MAKE) -C $(VECTOR_DIR) debug
	$(MAKE) -C $(VULKAN_DIR) debug

run-debug: debug
	./$(TARGET)

rerun-debug: re run-debug

$(BUILD_DIR) $(OBJ_DIR) $(DEPS_DIR):
	mkdir -p $@

$(TARGET): $(LIBS) $(OBJ_DIR) $(DEPS_DIR) $(SHADERS_OBJ) $(OBJECTS)
	$(CC) $(CPP_FLAGS) $(SYS_LIBS) $(INCLUDE) $(OBJECTS) $(LIBS) $(LIBS) -o $(TARGET)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CC) $(CPP_FLAGS) $(INCLUDE) $(DEPS_FLAGS) $(DEPS_DIR)/$*.d -c $< -o $@

$(BUILD_DIR)/%.spv: $(SHADERS_DIR)/%
	$(GLSLC) $< -o $@

-include $(DEPS)

clean:
	rm -rf $(BUILD_DIR)
	$(MAKE) -C $(VECTOR_DIR) clean
	$(MAKE) -C $(VULKAN_DIR) clean

fclean:
	rm -rf $(BUILD_DIR)
	rm -rf $(TARGET)
	$(MAKE) -C $(VECTOR_DIR) fclean
	$(MAKE) -C $(VULKAN_DIR) fclean

re: fclean all

re-release: fclean release

re-debug: fclean debug

.PHONY: all libs run rerun release libs-release run-release rerun-release debug libs-debug run-debug rerun-debug clean fclean re re-release re-debug
