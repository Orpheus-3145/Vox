TARGET			:= libvectors.a

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
TEST_DIR	:=	$(SRC_DIR)/tests

SOURCES			:=	$(shell find $(SRC_DIR) -maxdepth 1 -type f -name '*.cpp')
OBJECTS			:=	$(addprefix $(OBJ_DIR)/,$(notdir $(SOURCES:%.cpp=%.o)))
DEPS			:=	$(addprefix $(DEPS_DIR)/,$(notdir $(SOURCES:.cpp=.d)))

TEST_EXEC		:=	testmath
TEST_SOURCES	:=	$(shell find $(TEST_DIR) -type f -name '*.cpp')
TEST_OBJECTS	:=	$(addprefix $(OBJ_DIR)/,$(notdir $(TEST_SOURCES:%.cpp=%.o)))

INCLUDE		:=	-Iinclude

PLATFORM	:=	$(shell uname -s)

ifeq ($(PLATFORM), Darwin)
	INCLUDE	+= isystem /opt/homebrew/include -isystem /usr/local/include
endif


all: $(TARGET)

test: $(TEST_EXEC)

runtest: $(TEST_EXEC)
	./$(TEST_EXEC)

release: CPP_FLAGS += $(RELEASE_FLAGS)
release: all

debug: CPP_FLAGS += $(DEBUG_FLAGS)
debug: all

$(OBJ_DIR) $(DEPS_DIR):
	mkdir -p $@

$(TEST_EXEC): $(TARGET) $(TEST_OBJECTS) $(OBJECTS)
	$(CC) $(CPP_FLAGS) $(INCLUDE) -I$(TEST_DIR) $(TEST_OBJECTS) $(OBJECTS) $(TARGET) -o $(TEST_EXEC)

$(TARGET): $(OBJ_DIR) $(DEPS_DIR) $(OBJECTS)
	ar -rcs $@ $(OBJECTS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CC) $(CPP_FLAGS) $(INCLUDE) $(DEPS_FLAGS) $(DEPS_DIR)/$*.d -c $< -o $@

$(OBJ_DIR)/%.o: $(TEST_DIR)/%.cpp
	$(CC) $(CPP_FLAGS) $(INCLUDE) -I$(TEST_DIR) $(DEPS_FLAGS) $(DEPS_DIR)/$*.d -c $< -o $@

-include $(DEPS)

clean:
	rm -rf $(OBJ_DIR)
	rm -rf $(DEPS_DIR)

fclean:
	rm -rf $(BUILD_DIR)
	rm -f $(TEST_EXEC)

re: fclean all

re-release: fclean release

re-debug: fclean debug

.PHONY: all test runtest release debug clean fclean re re-release re-debug
