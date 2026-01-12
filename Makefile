# Ultra TFTP Server - Makefile

CC = gcc
CFLAGS = -Wall -Wextra -O3 -march=native -flto -Iinclude
LDFLAGS = -flto
DEBUG_CFLAGS = -Wall -Wextra -g -O0 -DDEBUG -fsanitize=address,undefined -Iinclude
DEBUG_LDFLAGS = -fsanitize=address,undefined

# Directories
SRCDIR = src
INCDIR = include
OBJDIR = obj

# Target
TARGET = utftp

# Source files
SRCS = $(SRCDIR)/main.c \
       $(SRCDIR)/server.c \
       $(SRCDIR)/session.c \
       $(SRCDIR)/transfer.c \
       $(SRCDIR)/packet.c \
       $(SRCDIR)/log.c \
       $(SRCDIR)/util.c

# Object files
OBJS = $(SRCS:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

# Header files
HDRS = $(wildcard $(INCDIR)/*.h)

.PHONY: all clean debug static install test

all: $(TARGET)

# Create object directory
$(OBJDIR):
	mkdir -p $(OBJDIR)

# Compile source files
$(OBJDIR)/%.o: $(SRCDIR)/%.c $(HDRS) | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Link
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)

# Debug build
debug: CFLAGS = $(DEBUG_CFLAGS)
debug: LDFLAGS = $(DEBUG_LDFLAGS)
debug: clean $(TARGET)
	mv $(TARGET) $(TARGET)-debug

# Static build
static: LDFLAGS += -static
static: clean $(TARGET)
	mv $(TARGET) $(TARGET)-static

# Install
install: $(TARGET)
	install -m 755 $(TARGET) /usr/local/bin/

# Clean
clean:
	rm -rf $(OBJDIR) $(TARGET) $(TARGET)-debug $(TARGET)-static

# Quick test
test: $(TARGET)
	./$(TARGET) -p 6969 -r ./test_files -d
