#
# Cross Platform Makefile
# Compatible with MSYS2/MINGW, Ubuntu 14.04.1 and Mac OS X
#
# You will need GLFW (http://www.glfw.org):
# Linux:
#   apt-get install libglfw-dev
# Mac OS X:
#   brew install glfw
# MSYS2:
#   pacman -S --noconfirm --needed mingw-w64-x86_64-toolchain mingw-w64-x86_64-glfw

#CXX = g++
#CXX = clang++

EXE = ./programa_foda.out
IMGUI_DIR = ./imgui
GUI_DIR = ./src/gui
CORE_DIR = ./src/core
WINDOW_DIR = ./src/window
GRAPHICS_DIR = ./src/graphics
BUILD_DIR = ./build/obj

SOURCES = $(wildcard ./src/*.cpp)
SOURCES += $(wildcard $(GRAPHICS_DIR)/*.cpp)
SOURCES += $(wildcard $(WINDOW_DIR)/*.cpp)
SOURCES += $(wildcard $(CORE_DIR)/*.cpp)
SOURCES += $(wildcard $(GUI_DIR)/*.cpp)

SOURCES += $(IMGUI_DIR)/imgui.cpp $(IMGUI_DIR)/imgui_demo.cpp $(IMGUI_DIR)/imgui_draw.cpp $(IMGUI_DIR)/imgui_tables.cpp $(IMGUI_DIR)/imgui_widgets.cpp
SOURCES += $(IMGUI_DIR)/backends/imgui_impl_glfw.cpp $(IMGUI_DIR)/backends/imgui_impl_opengl3.cpp

OBJS = $(addprefix $(BUILD_DIR)/, $(addsuffix .o, $(basename $(notdir $(SOURCES)))))

UNAME_S := $(shell uname -s)
LINUX_GL_LIBS = -lGL

CXXFLAGS = -std=c++20 -I$(GRAPHICS_DIR) -I$(WINDOW_DIR) -I$(CORE_DIR) -I$(GUI_DIR) -I$(IMGUI_DIR) -I$(IMGUI_DIR)/backends # Define DRAW_SHAPE_NAME makes so that the name is added to the Shape class and showed on the viewport
CXXFLAGS += -g -Wall -Wformat
LIBS =

##---------------------------------------------------------------------
## OPENGL ES
##---------------------------------------------------------------------

## This assumes a GL ES library available in the system, e.g. libGLESv2.so
# CXXFLAGS += -DIMGUI_IMPL_OPENGL_ES2
# LINUX_GL_LIBS = -lGLESv2

##---------------------------------------------------------------------
## BUILD FLAGS PER PLATFORM
##---------------------------------------------------------------------

ifeq ($(UNAME_S), Linux) #LINUX
	ECHO_MESSAGE = "Linux"
	LIBS += $(LINUX_GL_LIBS) `pkg-config --static --libs glfw3`

	CXXFLAGS += `pkg-config --cflags glfw3`
	CFLAGS = $(CXXFLAGS)
endif

ifeq ($(UNAME_S), Darwin) #APPLE
	ECHO_MESSAGE = "Mac OS X"
	LIBS += -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo
	LIBS += -L/usr/local/lib -L/opt/local/lib -L/opt/homebrew/lib
	#LIBS += -lglfw3
	LIBS += -lglfw

	CXXFLAGS += -I/usr/local/include -I/opt/local/include -I/opt/homebrew/include
	CFLAGS = $(CXXFLAGS)
endif

ifeq ($(OS), Windows_NT)
	ECHO_MESSAGE = "MinGW"
	LIBS += -lglfw3 -lgdi32 -lopengl32 -limm32

	CXXFLAGS += `pkg-config --cflags glfw3`
	CFLAGS = $(CXXFLAGS)n nu
endif

##---------------------------------------------------------------------
## BUILD RULES
##---------------------------------------------------------------------

vpath %.cpp ./src $(GRAPHICS_DIR) $(WINDOW_DIR) $(CORE_DIR) $(GUI_DIR) $(IMGUI_DIR) $(IMGUI_DIR)/backends

$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

all: $(EXE)
	@echo Build complete for $(ECHO_MESSAGE)

fast: CXXFLAGS := -std=c++20 -O3 -I$(GRAPHICS_DIR) -I$(WINDOW_DIR) -I$(CORE_DIR) -I$(GUI_DIR) -I$(IMGUI_DIR) -I$(IMGUI_DIR)/backends -g -Wall -Wformat

fast: $(EXE)
	@echo Fast build complete for $(ECHO_MESSAGE)

$(EXE): $(OBJS)
	@mkdir -p $(dir $@)
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LIBS)

clean:
	rm -rf build
	rm $(EXE)
