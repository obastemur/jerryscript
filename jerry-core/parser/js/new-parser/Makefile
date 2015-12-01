CC = gcc
LD = gcc
MKDIR = mkdir
RMDIR = rmdir
RM = rm -rf
CFLAGS = -g -O0
CFLAGS += -Wall -Wextra -std=c99 -pedantic \
          -Wformat-nonliteral -Winit-self -Wno-stack-protector \
          -Wconversion -Wsign-conversion -Wformat-security \
          -Wmissing-declarations -Wno-attributes
LD_FLAGS =

SRC = src
DST = bin

TARGET = js-parser
OBJS = byte-code.o common.o js-lexer.o \
       js-parser.o js-parser-expr.o js-parser-mem.o \
       js-parser-scanner.o js-parser-statm.o js-parser-util.o \
       main.o

TARGET_FILE = $(DST)/$(TARGET)
OBJ_FILES = $(addprefix $(DST)/, $(OBJS))

.PHONY: all test
all: $(TARGET_FILE)

$(DST) :
	$(MKDIR) $(DST)

$(DST)/%.o : $(SRC)/%.c
	$(CC) $(CFLAGS) $^ -c -o $@

$(TARGET_FILE) : $(DST) $(OBJ_FILES)
	$(LD) $(CFLAGS) $(LD_FLAGS) $(OBJ_FILES) -o $@

test : $(TARGET_FILE)
	tests/test.py --binary $^ --testbase tests --tests js

clean:
	$(RM) $(DST)
