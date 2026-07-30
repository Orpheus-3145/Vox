LIB_NAME         := libvectors.a
CC               := c++

BASE_FLAGS       := -std=c++2b -Wall -Wextra -Werror
CPP_FLAGS        := $(BASE_FLAGS)
DEBUG_FLAGS      := -O0 -g3 -fsanitize=address,undefined -fno-omit-frame-pointer
RELEASE_FLAGS    := -O2 -DNDEBUG -march=native -flto -fno-math-errno -fno-plt -ffast-math -funroll-loops
DEPS_FLAGS       := -MMD -MP -MF

INCLUDE          := -Iinclude

SRC_DIR          := source
BUILD_DIR        := build
DEPS_DIR         := $(BUILD_DIR)/deps
OBJ_DIR          := $(BUILD_DIR)/obj
TARGET           := $(BUILD_DIR)/$(LIB_NAME)

SOURCES           = $(shell find $(SRC_DIR) -type f -name '*.cpp')
OBJECTS           = $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SOURCES))
DEPS              = $(patsubst $(SRC_DIR)/%.cpp,$(DEPS_DIR)/%.d,$(SOURCES))

LAST_MODE        := $(BUILD_DIR)/.last_build_mode
CURR_MODE        := default

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

$(TARGET): $(OBJ_DIR) $(DEPS_DIR) $(OBJECTS)
	ar -rcs $@ $(OBJECTS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CC) $(CPP_FLAGS) $(INCLUDE) $(DEPS_FLAGS) $(DEPS_DIR)/$*.d -c $< -o $@

-include $(DEPS)

clean:
	rm -rf $(BUILD_DIR)

fclean: clean

re: fclean all

re-debug: fclean debug

re-release: fclean release

.PHONY: all debug release clean fclean re re-debug re-release
