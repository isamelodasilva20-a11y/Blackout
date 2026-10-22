# Detecção do Sistema Operacional
ifeq ($(OS),Windows_NT)
    # Configurações para Windows
    PROJ_NAME = blackout.exe
    RAYLIB_PATH ?= C:/raylib
    CFLAGS = -Wall -Werror -g -I$(INCLUDE_DIR) -I$(RAYLIB_PATH)/include
    LDFLAGS = -L$(RAYLIB_PATH)/lib -lraylib -lgdi32 -lwinmm
    RM = rmdir /s /q
    FIXPATH = $(subst /,\,$1)
else
    # Configurações para macOS e Linux
    PROJ_NAME = blackout
    UNAME_S := $(shell uname -s)
    
    ifeq ($(UNAME_S),Darwin)
        # macOS
        HOMEBREW_PREFIX ?= /opt/homebrew
        CFLAGS = -Wall -Werror -g -I$(INCLUDE_DIR) -I$(HOMEBREW_PREFIX)/include
        LDFLAGS = -L$(HOMEBREW_PREFIX)/lib -lraylib -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo
    else
        # Linux
        CFLAGS = -Wall -Werror -g -I$(INCLUDE_DIR)
        LDFLAGS = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
    endif
    RM = rm -rf
    FIXPATH = $1
endif

# Diretórios
BUILD_DIR   = build
OBJ_DIR     = $(BUILD_DIR)/obj
SRC_DIR     = src
INCLUDE_DIR = include

# Arquivos
SRC_FILES = $(notdir $(wildcard $(SRC_DIR)/*.c))
OBJ_FILES = $(SRC_FILES:%.c=$(OBJ_DIR)/%.o)

# Compilação
all: $(OBJ_DIR) $(OBJ_FILES)
	@echo Creating $(BUILD_DIR)/$(PROJ_NAME)
	@$(CC) $(CFLAGS) -o $(BUILD_DIR)/$(PROJ_NAME) $(OBJ_FILES) $(LDFLAGS)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(OBJ_DIR): $(BUILD_DIR)
	mkdir -p $(OBJ_DIR)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@echo Compiling $@...
	@$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM) $(call FIXPATH,$(BUILD_DIR))

run: all
	./$(BUILD_DIR)/$(PROJ_NAME)
