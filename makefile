
SRC_DIR = ./src
OBJ_DIR = ./bin
TARGET_NAME = QLforth

CC = gcc

CFLAGS =
CPPFLAGS += -MMD -MP

LDFLAGS =
LDLIBS =

SRCS = $(wildcard $(SRC_DIR)/*.c)

OBJS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))

DEPS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.d,$(SRCS))

all : $(OBJ_DIR)/$(TARGET_NAME) ;

$(OBJ_DIR)/$(TARGET_NAME) : $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)
	@echo
	@echo $(TARGET_NAME) build success!
	@echo

$(OBJ_DIR)/%.o : $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) -c $(CFLAGS) $(CPPFLAGS) -o $@ $<

-include $(DEPS)

$(OBJ_DIR) :
	@echo Creating obj_dir ...
	@mkdir $(OBJ_DIR)
	@echo obj_dir created!

clean :
	@echo "cleanning..."
	del $(OBJ_DIR)
	@echo "clean done!"

.PHONY: all clean 
