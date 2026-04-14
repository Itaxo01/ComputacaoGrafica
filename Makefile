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

#CXX = clang++

CXX = g++
EXE = ./programa_foda.out
IMGUI_DIR = ./imgui
GUI_DIR = ./src/gui
CONTROLLER_DIR = ./src/controller
CORE_DIR = ./src/core
WINDOW_DIR = ./src/window
GRAPHICS_DIR = ./src/graphics
BUILD_DIR = ./build/obj

SOURCES = $(wildcard ./src/*.cpp)
SOURCES += $(wildcard $(GRAPHICS_DIR)/*.cpp)
SOURCES += $(wildcard $(WINDOW_DIR)/*.cpp)
SOURCES += $(wildcard $(CORE_DIR)/*.cpp)
SOURCES += $(wildcard $(GUI_DIR)/*.cpp)
SOURCES += $(wildcard $(CONTROLLER_DIR)/*.cpp)

SOURCES += $(IMGUI_DIR)/imgui.cpp $(IMGUI_DIR)/imgui_demo.cpp $(IMGUI_DIR)/imgui_draw.cpp $(IMGUI_DIR)/imgui_tables.cpp $(IMGUI_DIR)/imgui_widgets.cpp
SOURCES += $(IMGUI_DIR)/backends/imgui_impl_glfw.cpp $(IMGUI_DIR)/backends/imgui_impl_opengl3.cpp

OBJS = $(addprefix $(BUILD_DIR)/, $(addsuffix .o, $(basename $(notdir $(SOURCES)))))

UNAME_S := $(shell uname -s)
LINUX_GL_LIBS = -lGL

CXXFLAGS = -std=c++20 -I$(GRAPHICS_DIR) -I$(WINDOW_DIR) -I$(CORE_DIR) -I$(GUI_DIR) -I$(CONTROLLER_DIR) -I$(IMGUI_DIR) -I$(IMGUI_DIR)/backends # Define DONT_DRAW_SHAPE_NAME makes so that the name is added to the Shape class and showed on the viewport
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
	CFLAGS = $(CXXFLAGS)
endif

# Testa se o projeto compila com o #include <execution>
TBB_CHECK := $(shell printf '#include <execution>\nint main(){}\n' | $(CXX) -std=c++17 -xc++ - -o /dev/null 2>/dev/null -ltbb && echo "YES" || echo "NO")

ifeq ($(TBB_CHECK), YES)
	ECHO_MESSAGE += "(with TBB parallel execution)"
	CXXFLAGS += -DUSE_TBB_EXECUTION
	LIBS += -ltbb
else
	ECHO_MESSAGE += "(with Native fallback execution)"
endif

##---------------------------------------------------------------------
## BUILD RULES
##---------------------------------------------------------------------

vpath %.cpp ./src $(GRAPHICS_DIR) $(WINDOW_DIR) $(CORE_DIR) $(GUI_DIR) $(CONTROLLER_DIR) $(IMGUI_DIR) $(IMGUI_DIR)/backends

$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

all: $(EXE)
	@echo Build complete for $(ECHO_MESSAGE)

fast: CXXFLAGS += -O3 -DDONT_DRAW_SHAPE_NAME -DUSE_PARALLEL_DRAWLIST

fast: $(EXE)
	@echo Fast build complete for $(ECHO_MESSAGE)

$(EXE): $(OBJS)
	@mkdir -p $(dir $@)
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LIBS)

# --- CROSS-COMPILATION FOR WINDOWS FROM LINUX (STANDALONE - NO TBB) ---
windows: CXX = x86_64-w64-mingw32-g++
windows: EXE = ./build/windows/programa_foda.exe
windows: ECHO_MESSAGE = "Windows Safe (Standalone, no TBB)"
windows: CXXFLAGS += -D_WIN32 -UUSE_TBB_EXECUTION -I./libs/windows/include
windows: LIBS = -L./libs/windows -lglfw3 -lgdi32 -lopengl32 -limm32
windows: LDFLAGS = -static

windows: $(OBJS)
	@mkdir -p $(dir $(EXE))
	$(CXX) -o $(EXE) $^ $(CXXFLAGS) $(LDFLAGS) $(LIBS)
	@cd $(dir $(EXE)) && zip programa_foda_safe.zip programa_foda.exe
	@echo Build complete for $(ECHO_MESSAGE)
	@echo The standalone Windows build was saved to $(dir $(EXE))

# --- CROSS-COMPILATION FOR WINDOWS FROM LINUX (FAST + TBB) ---
windows_fast: CXX = x86_64-w64-mingw32-g++
windows_fast: EXE = ./build/windows/programa_foda.exe
windows_fast: ECHO_MESSAGE = "Windows Fast (with TBB, requires DLLs)"
windows_fast: CXXFLAGS += -D_WIN32 -DUSE_TBB_EXECUTION -I./libs/windows/include
windows_fast: LIBS = ./libs/windows/libtbb12.dll.a -L./libs/windows -lglfw3 -lgdi32 -lopengl32 -limm32
windows_fast: LDFLAGS = -static

# A dll do libtbb12 teve que ser recompilada a partir de https://github.com/oneapi-src/oneTBB.git. Retirei o repositório do git após recompilar a dll. 

# Aqui as dll's estão sendo puxadas direto do mingw32. Isso é necessário para a aplicação ser standalone e não depender das biblitecas instaladas no computador do usuário.
windows_fast: $(OBJS)
	@mkdir -p $(dir $(EXE))
	$(CXX) -o $(EXE) $^ $(CXXFLAGS) $(LDFLAGS) $(LIBS)
	@if [ -f "./libs/windows/libtbb12.dll" ]; then \
		cp ./libs/windows/libtbb12.dll $(dir $(EXE)); \
	else  \
		echo "Error: libtbb12.dll not found! Cleaning up and aborting."; \
		$(MAKE) clean; \
		exit 1; \
	fi
	@if [ -f "/usr/lib/gcc/x86_64-w64-mingw32/13-posix/libgcc_s_seh-1.dll" ]; then \
		cp /usr/lib/gcc/x86_64-w64-mingw32/13-posix/libgcc_s_seh-1.dll $(dir $(EXE)); \
	else  \
		echo "Error: libgcc_s_seh-1.dll not found! Cleaning up and aborting."; \
		$(MAKE) clean; \
		exit 1; \
	fi
	@if [ -f "/usr/lib/gcc/x86_64-w64-mingw32/13-posix/libstdc++-6.dll" ]; then \
		cp /usr/lib/gcc/x86_64-w64-mingw32/13-posix/libstdc++-6.dll $(dir $(EXE)); \
	else  \
		echo "Error: libstdc++-6.dll not found! Cleaning up and aborting."; \
		$(MAKE) clean; \
		exit 1; \
	fi
	@if [ -f "/usr/x86_64-w64-mingw32/lib/libwinpthread-1.dll" ]; then \
		cp /usr/x86_64-w64-mingw32/lib/libwinpthread-1.dll $(dir $(EXE)); \
	else  \
		echo "Error: libwinpthread-1.dll not found! Cleaning up and aborting."; \
		$(MAKE) clean; \
		exit 1; \
	fi
	@cd $(dir $(EXE)) && zip programa_compilado_windows64.zip programa_foda.exe libtbb12.dll libwinpthread-1.dll libgcc_s_seh-1.dll libstdc++-6.dll
	@echo Build complete for $(ECHO_MESSAGE)
	@echo The fast Windows build was saved to $(dir $(EXE))
	@echo "** IMPORTANT: Keep libtbb12.dll in the same folder as your .exe! **"

clean:
	rm -rf build
	rm -f $(EXE)