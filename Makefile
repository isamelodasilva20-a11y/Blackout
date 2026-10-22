# Diretórios
BUILD_DIR   = build
OBJ_DIR     = $(BUILD_DIR)/obj
SRC_DIR     = src
INCLUDE_DIR = include

# Flags comuns (-MMD -MP gera dependências dos .h automaticamente)
CFLAGS = -Wall -g -std=c99 -MMD -MP -I$(INCLUDE_DIR)

# Detecção do Sistema Operacional
ifeq ($(OS),Windows_NT)
    # Configurações para Windows (w64devkit, MinGW ou instalador do raylib)
    PROJ_NAME = blackout.exe

    # No MinGW o "cc" padrão pode não existir
    ifeq ($(origin CC),default)
        CC = gcc
    endif

    # Pasta do raylib. Aceita o .zip oficial (C:/raylib/include e C:/raylib/lib)
    # e o instalador (C:/raylib/raylib/src). Para outro lugar:
    # make RAYLIB_PATH=D:/minhapasta/raylib
    RAYLIB_PATH ?= C:/raylib
    CFLAGS  += -I$(RAYLIB_PATH)/include -I$(RAYLIB_PATH)/raylib/src
    LDFLAGS  = -L$(RAYLIB_PATH)/lib -L$(RAYLIB_PATH)/raylib/src -lraylib -lopengl32 -lgdi32 -lwinmm

    # O make do Windows pode rodar os comandos no cmd.exe ou num shell
    # tipo Unix (w64devkit, Git Bash, MSYS2). No cmd, o echo mantém as aspas.
    ifeq ($(shell echo "x"),"x")
        MKDIR = if not exist $(subst /,\,$1) mkdir $(subst /,\,$1)
        RM    = if exist $(subst /,\,$1) rmdir /s /q $(subst /,\,$1)
        EXEC  = $(subst /,\,$1)
    else
        MKDIR = mkdir -p $1
        RM    = rm -rf $1
        EXEC  = ./$1
    endif
else
    # Configurações para macOS e Linux
    PROJ_NAME = blackout
    UNAME_S := $(shell uname -s)

    ifeq ($(UNAME_S),Darwin)
        # macOS: /opt/homebrew (Apple Silicon) ou /usr/local (Intel)
        HOMEBREW_PREFIX ?= $(shell brew --prefix 2>/dev/null || echo /opt/homebrew)
        CFLAGS  += -I$(HOMEBREW_PREFIX)/include
        LDFLAGS  = -L$(HOMEBREW_PREFIX)/lib -lraylib -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo
    else
        # Linux
        LDFLAGS = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
    endif

    MKDIR = mkdir -p $1
    RM    = rm -rf $1
    EXEC  = ./$1
endif

# Arquivos
SRC_FILES = $(notdir $(wildcard $(SRC_DIR)/*.c))
OBJ_FILES = $(SRC_FILES:%.c=$(OBJ_DIR)/%.o)
TARGET    = $(BUILD_DIR)/$(PROJ_NAME)

.PHONY: all clean run

# Compilação
all: $(TARGET)

$(TARGET): $(OBJ_FILES)
	@echo Creating $@
	@$(CC) -o $@ $(OBJ_FILES) $(LDFLAGS)

$(OBJ_DIR):
	@$(call MKDIR,$(OBJ_DIR))

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	@echo Compiling $@...
	@$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(call RM,$(BUILD_DIR))

run: all
	$(call EXEC,$(TARGET))

-include $(OBJ_FILES:.o=.d)
