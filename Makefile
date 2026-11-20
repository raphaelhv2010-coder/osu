CC = arm-none-eabi-gcc
CXX = arm-none-eabi-g++
OBJCOPY = arm-none-eabi-objcopy

APP_NAME = osu_mania_4k
BUILD_DIR = build

CFLAGS = -mcpu=cortex-m7 -mthumb -mfpu=fpv5-sp-d16 -mfloat-abi=hard -Os -Wall -fdata-sections -ffunction-sections
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti -std=c++17
LDFLAGS = -Wl,--gc-sections -Wl,-e,main -nostartfiles -Wl,--unresolved-symbols=ignore-all

OBJS = $(BUILD_DIR)/main.o

all: $(APP_NAME).nwa

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/main.o: src/main.cpp src/eadk.h | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(APP_NAME).nwa: $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) $(LDFLAGS) -o $@
	@echo "Build successful: $(APP_NAME).nwa"

clean:
	rm -rf $(BUILD_DIR) $(APP_NAME).nwa

.PHONY: all clean
