CC = gcc
CFLAGS = -Wall -O2

SRC = src/ant_simulator.c
OBJ = build/ant.o
APP = build/ant_simulator
BUILD_DIR = build

all: prepare $(APP)

prepare:
	mkdir -p $(BUILD_DIR)

$(APP): $(OBJ)
	$(CC) -o $@ $? -lant -lraylib

$(OBJ): $(SRC)
	$(CC) $(CFLAGS) -c $? -o $@