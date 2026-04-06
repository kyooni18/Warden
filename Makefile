BUILD_DIR ?= build

.DEFAULT_GOAL := rpg

.PHONY: configure build feather rpg run-rpg test clean

configure:
	cmake -S . -B $(BUILD_DIR)

build: configure
	cmake --build $(BUILD_DIR)

feather: configure
	cmake --build $(BUILD_DIR) --target Feather

rpg: configure
	cmake --build $(BUILD_DIR) --target warden

run-rpg: rpg
	./$(BUILD_DIR)/Warden

test: configure
	cmake --build $(BUILD_DIR) --target test_utils
	ctest --test-dir $(BUILD_DIR) --output-on-failure

clean:
	cmake -E rm -rf $(BUILD_DIR)
