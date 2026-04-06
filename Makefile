BUILD_DIR ?= build

.DEFAULT_GOAL := rpg

.PHONY: configure build feather rpg run-rpg clean

configure:
	git submodule update --init --recursive
	cmake -S . -B $(BUILD_DIR)

build: configure
	cmake --build $(BUILD_DIR)

feather: configure
	cmake --build $(BUILD_DIR) --target Feather

rpg: configure
	cmake --build $(BUILD_DIR) --target warden

run-rpg: rpg
	./$(BUILD_DIR)/Warden

clean:
	cmake -E rm -rf $(BUILD_DIR)
