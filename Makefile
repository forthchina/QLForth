
TARGETS = QLForth

SRC_DIR = src
OBJ_DIR	= bin

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(addprefix $(OBJ_DIR)/, $(patsubst %.c, %.o, $(notdir $(SRCS))))

CC      := gcc
LIBS    := 
LDFLAGS := -lm
DEFINES :=
INCLUDE := -Iinc
CFLAGS  := -c -g -Wall -O3 -std=c99 $(DEFINES) $(INCLUDE)
RM      = rm -f -v

.PHONY: all clean
all: $(OBJ_DIR)/$(TARGETS)

$(OBJ_DIR)/QLForth : $(OBJ_DIR)/main.o $(OBJ_DIR)/QLForth.o $(OBJ_DIR)/Code.o $(OBJ_DIR)/Compiler.o $(OBJ_DIR)/Simulator.o
	$(CC) -o $(OBJ_DIR)/QLForth $(LDFLAGS) $(OBJ_DIR)/main.o $(OBJ_DIR)/QLForth.o $(OBJ_DIR)/Code.o $(OBJ_DIR)/Compiler.o $(OBJ_DIR)/Simulator.o

$(OBJ_DIR)/main.o : $(SRC_DIR)/main.c
	$(CC) $(CFLAGS) -o $(OBJ_DIR)/main.o $(SRC_DIR)/main.c 

$(OBJ_DIR)/QLForth.o : $(SRC_DIR)/QLForth.c
	$(CC) $(CFLAGS) -o $(OBJ_DIR)/QLForth.o $(SRC_DIR)/QLForth.c 

$(OBJ_DIR)/Code.o : $(SRC_DIR)/Code.c
	$(CC) $(CFLAGS) -o $(OBJ_DIR)/Code.o $(SRC_DIR)/Code.c 

$(OBJ_DIR)/Compiler.o : $(SRC_DIR)/Compiler.c
	$(CC) $(CFLAGS) -o $(OBJ_DIR)/Compiler.o $(SRC_DIR)/Compiler.c 

$(OBJ_DIR)/Simulator.o : $(SRC_DIR)/Simulator.c
	$(CC) $(CFLAGS) -o $(OBJ_DIR)/Simulator.o $(SRC_DIR)/Simulator.c 
	
clean:
	rm -f $(OBJ_DIR)/*.*
	
	