CC = gcc
CFLAGS = -Wall -Wfatal-errors ${def}

BIN_DIR = ./bin
LIB = ${BIN_DIR}/libgrem.so
TEST = ${BIN_DIR}/t_grem.exe

SRC_DIR = ./src
SRC_TEST_DIR = ./test
OBJ_DIR = ./obj

SRC_FILES = $(wildcard $(SRC_DIR)/*.c)
SRC_TEST_FILES = $(wildcard $(SRC_TEST_DIR)/*.c)
OBJ_FILES = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRC_FILES))
OBJ_TEST_FILES = $(patsubst $(SRC_TEST_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRC_TEST_FILES))

debug: CFLAGS += -fPIC -g
debug: $(LIB)
release: CFLAGS += -fPIC -O2
release: $(LIB)
test: CFLAGS += -g
test: $(TEST)

$(TEST): $(OBJ_TEST_FILES)
	$(CC) -o $@ $^ -L./bin -lgrem

$(LIB): $(OBJ_FILES)
	$(CC) -o $@ $^ -shared -lm

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) -o $@ -c $< $(CFLAGS)

$(OBJ_DIR)/%.o: $(SRC_TEST_DIR)/%.c
	$(CC) -o $@ -c $< $(CFLAGS) -I./src

$(SRC_DIR)/%.c: $(SRC_DIR)/%.h
	@touch $@

.PHONY: clean cclean

clean:
	rm -f $(OBJ_DIR)/*.o

cclean: clean
	rm -f $(LIB) $(TEST)
