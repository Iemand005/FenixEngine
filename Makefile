# Makefile

CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2
INCLUDES = -Iinclude
LIBS = -lglfw3 -lGL -lX11 -lpthread -ldl -lm -lstdc++

# Source files
GLAD_SRC = src/glad.c
IMGUI_SRCS = src/imgui/imgui.cpp \
             src/imgui/imgui_draw.cpp \
             src/imgui/imgui_tables.cpp \
             src/imgui/imgui_widgets.cpp \
             src/imgui/imgui_impl_glfw.cpp \
             src/imgui/imgui_impl_opengl3.cpp
ENGINE_SRC = src/engine/engine.cpp
MAIN_SRC = src/games/Annihilation/main.cpp

# Output
BUILD_DIR = build
TARGET = $(BUILD_DIR)/annihilation

# Object files
OBJECTS = $(BUILD_DIR)/glad.o \
          $(BUILD_DIR)/imgui.o \
          $(BUILD_DIR)/imgui_draw.o \
          $(BUILD_DIR)/imgui_tables.o \
          $(BUILD_DIR)/imgui_widgets.o \
          $(BUILD_DIR)/imgui_impl_glfw.o \
          $(BUILD_DIR)/imgui_impl_opengl3.o \
          $(BUILD_DIR)/engine.o \
          $(BUILD_DIR)/main.o

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $@ $(LIBS)

# Compile C files (glad.c)
$(BUILD_DIR)/glad.o: $(GLAD_SRC)
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Compile C++ files
$(BUILD_DIR)/%.o: src/%.cpp
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/%.o: src/imgui/%.cpp
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/%.o: src/engine/%.cpp
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/%.o: src/games/Annihilation/%.cpp
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run