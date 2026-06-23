CC = gcc
CFLAGS_COMMON = -Wall -Iinclude -pthread

CFLAGS_DEBUG = $(CFLAGS_COMMON) -g -O0
CFLAGS_RELEASE = $(CFLAGS_COMMON) -O3

BUILD_TYPE ?= debug
BIN_DIR = bin/$(BUILD_TYPE)

TARGETS = $(BIN_DIR)/smf_server $(BIN_DIR)/upf_server $(BIN_DIR)/gnb_client

all: debug

debug:
	@$(MAKE) build_project BUILD_TYPE=debug CFLAGS="$(CFLAGS_DEBUG)"

release:
	@$(MAKE) build_project BUILD_TYPE=release CFLAGS="$(CFLAGS_RELEASE)"

build_project: create_dir $(TARGETS)
	@echo "========================================================="
	@echo " Đã biên dịch THÀNH CÔNG bản [$(BUILD_TYPE)] vào thư mục: $(BIN_DIR)"
	@echo "========================================================="

create_dir:
	mkdir -p $(BIN_DIR)

$(BIN_DIR)/smf_server: src/smf_server.c
	$(CC) $(CFLAGS) src/smf_server.c -o $(BIN_DIR)/smf_server

$(BIN_DIR)/upf_server: src/upf_server.c src/threadPool.c
	$(CC) $(CFLAGS) src/upf_server.c src/threadPool.c -o $(BIN_DIR)/upf_server

$(BIN_DIR)/gnb_client: src/gnb_client.c
	$(CC) $(CFLAGS) src/gnb_client.c -o $(BIN_DIR)/gnb_client

clean:
	rm -rf bin
	@echo "Clean finished All files in bin/"