.PHONY: all run clean makedirs

BUILD_BASE_DIR = build
BUILD_DIR = $(BUILD_BASE_DIR)/debug
ifdef release
	BUILD_DIR = $(BUILD_BASE_DIR)/release
endif
SOURCES_DIR = src
SOURCES_SUBDIRS = cc main
OBJECTS_DIR = $(BUILD_DIR)/obj
SYS_MODULES_DIR = $(BUILD_BASE_DIR)/sys_mods

CXX = clang++
TYPE_FLAGS = -g -DDEBUG
ifdef release
	TYPE_FLAGS = -O3 -DNDEBUG
endif
CXXFLAGS_BASIC0 = -std=c++23 -Wno-experimental-header-units
CXXFLAGS_BASIC = $(TYPE_FLAGS) $(CXXFLAGS_BASIC0)
CXXFLAGS = $(CXXFLAGS_BASIC) $(addprefix -fprebuilt-module-path=$(OBJECTS_DIR)/,cc) $(addprefix -fmodule-file=,$(SYS_MODULES))

SYS_MODULES = $(addsuffix .pcm,$(addprefix $(SYS_MODULES_DIR)/,cstdint print))

CC_SOURCES_BARE = cc.cppm
CC_SOURCES = $(addprefix $(SOURCES_DIR)/cc/,$(CC_SOURCES_BARE))
CC_OBJECTS = $(addprefix $(OBJECTS_DIR)/cc/,$(addsuffix .o,$(basename $(CC_SOURCES_BARE))))

MAIN_SOURCES_BARE = main.cpp
MAIN_SOURCES = $(addprefix $(SOURCES_DIR)/main/,$(MAIN_SOURCES_BARE))
MAIN_OBJECTS = $(addprefix $(OBJECTS_DIR)/main/,$(addsuffix .o,$(basename $(MAIN_SOURCES_BARE))))

SOURCES_BARE = $(CC_SOURCES_BARE) $(MAIN_SOURCES_BARE)
SOURCES = $(CC_SOURCES) $(MAIN_SOURCES)
MODULES_FIRST = $(filter %.cppm,$(SOURCES))
MODULES = $(MODULES_FIRST:$(SOURCES_DIR)/%.cppm=$(OBJECTS_DIR)/%.pcm)
OBJECTS = $(CC_OBJECTS) $(MAIN_OBJECTS)
TARGETS = $(addprefix $(BUILD_DIR)/,main)

all: makedirs $(TARGETS)

$(BUILD_DIR)/main: $(OBJECTS)	# link
	$(CXX) $(CXXFLAGS) $^ -o $@

$(OBJECTS_DIR)/%.pcm: $(SOURCES_DIR)/%.cppm		# precompile BMIs
	$(CXX) $(CXXFLAGS) --precompile $^ -o $@

$(OBJECTS_DIR)/%.o: $(SOURCES_DIR)/%.cppm $(MODULES) $(SYS_MODULES)		# compile module interface units
	$(CXX) $(CXXFLAGS) -c $< -o $@
$(OBJECTS_DIR)/%.o: $(SOURCES_DIR)/%.cpp $(CC_OBJECTS) $(SYS_MODULES)	# compile normal sources
	$(CXX) $(CXXFLAGS) -c $< -o $@

%.pcm:		# build system precompiled modules
	$(CXX) $(CXXFLAGS_BASIC0) -Wno-pragma-system-header-outside-header --precompile -xc++-system-header $(basename $(notdir $@)) -o $@

makedirs:	# essential directories
	mkdir -p $(BUILD_DIR) $(OBJECTS_DIR) $(addprefix $(OBJECTS_DIR)/,$(SOURCES_SUBDIRS)) $(SYS_MODULES_DIR) &>/dev/null; true

run: $(BUILD_DIR)/main
	./$(BUILD_DIR)/main

clean:
	rm -f $(TARGETS) $(OBJECTS) $(MODULES)
