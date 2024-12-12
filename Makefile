TARGET_EXEC := luaupi

BUILD_DIR := ./build
SRC_DIR := ./src

SRCS := $(shell find $(SRC_DIR) -name '*.cpp') \
	$(shell find ./luau/VM -name '*.cpp') \
	$(shell find ./luau/Ast -name '*.cpp') \
	$(shell find ./luau/Compiler -name '*.cpp') \
	$(shell find ./luau/CodeGen -name '*.cpp')

OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)

DEPS := $(OBJS:.o=.d)

INC_FLAGS := \
	-I./src \
	-I./luau/VM/include \
	-I./luau/VM/src \
	-I./luau/Ast/include \
	-I./luau/Compiler/include \
	-I./luau/CodeGen/include \
	-I./luau/Common/include

CPPFLAGS := $(INC_FLAGS) -MMD -MP -std=c++17 -Wall
LDFLAGS := -lwiringPi

$(BUILD_DIR)/$(TARGET_EXEC): $(OBJS)
	$(CXX) $(OBJS) -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.cpp.o: %.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -r $(BUILD_DIR)

-include $(DEPS)
