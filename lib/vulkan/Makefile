TARGET			:= libvk.a

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

SRC_DIR		:=	source
BUILD_DIR	:=  build
OBJ_DIR		:=	$(BUILD_DIR)/obj
DEPS_DIR	:=  $(BUILD_DIR)/deps
TARGET		:=	$(BUILD_DIR)/$(TARGET)

SOURCES		:=	$(shell find $(SRC_DIR) -type f -name '*.cpp')
OBJECTS		:=	$(addprefix $(OBJ_DIR)/,$(notdir $(SOURCES:%.cpp=%.o)))
DEPS		:=	$(addprefix $(DEPS_DIR)/,$(notdir $(SOURCES:.cpp=.d)))

INCLUDE		:=	-Iinclude \
				-I../vectors/include

PLATFORM	:=	$(shell uname -s)

ifeq ($(PLATFORM), Darwin)
	INCLUDE	+= isystem /opt/homebrew/include -isystem /usr/local/include
endif


all: $(TARGET)

release: CPP_FLAGS += $(RELEASE_FLAGS)
release: all

debug: CPP_FLAGS += $(DEBUG_FLAGS)
debug: all

$(OBJ_DIR) $(DEPS_DIR):
	mkdir -p $@

$(TARGET): $(OBJ_DIR) $(DEPS_DIR) $(OBJECTS)
	ar -rcs $@ $(OBJECTS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CC) $(CPP_FLAGS) $(INCLUDE) $(DEPS_FLAGS) $(DEPS_DIR)/$*.d -c $< -o $@

-include $(DEPS)

clean:
	rm -rf $(OBJ_DIR)
	rm -rf $(DEPS_DIR)

fclean:
	rm -rf $(BUILD_DIR)

re: fclean all

re-release: fclean release

re-debug: fclean debug

.PHONY: all release debug clean fclean re re-release re-debug
