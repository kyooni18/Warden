BUILD_DIR ?= build

.DEFAULT_GOAL := rpg

.PHONY: configure build feather rpg run-rpg clean

configure:
	cmake -S . -B $(BUILD_DIR)

build: configure
	cmake --build $(BUILD_DIR)

feather: configure
	cmake --build $(BUILD_DIR) --target Feather

rpg: configure
	cmake --build $(BUILD_DIR) --target cli_text_rpg

run-rpg: rpg
	./$(BUILD_DIR)/CLITextRPG

clean:
	cmake -E rm -rf $(BUILD_DIR)
