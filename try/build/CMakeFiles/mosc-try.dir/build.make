# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.27

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Disable VCS-based implicit rules.
% : %,v

# Disable VCS-based implicit rules.
% : RCS/%

# Disable VCS-based implicit rules.
% : RCS/%,v

# Disable VCS-based implicit rules.
% : SCCS/s.%

# Disable VCS-based implicit rules.
% : s.%

.SUFFIXES: .hpux_make_needs_suffix_list

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/local/Cellar/cmake/3.27.1/bin/cmake

# The command to remove a file.
RM = /usr/local/Cellar/cmake/3.27.1/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /Users/doumbia0804/CLionProjects/mosc/try

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /Users/doumbia0804/CLionProjects/mosc/try/build

# Include any dependencies generated for this target.
include CMakeFiles/mosc-try.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include CMakeFiles/mosc-try.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/mosc-try.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/mosc-try.dir/flags.make

CMakeFiles/mosc-try.dir/main.try.c.o: CMakeFiles/mosc-try.dir/flags.make
CMakeFiles/mosc-try.dir/main.try.c.o: /Users/doumbia0804/CLionProjects/mosc/try/main.try.c
CMakeFiles/mosc-try.dir/main.try.c.o: CMakeFiles/mosc-try.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --progress-dir=/Users/doumbia0804/CLionProjects/mosc/try/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object CMakeFiles/mosc-try.dir/main.try.c.o"
	/Users/doumbia0804/Documents/2023/Labs/emsdk/upstream/emscripten/emcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT CMakeFiles/mosc-try.dir/main.try.c.o -MF CMakeFiles/mosc-try.dir/main.try.c.o.d -o CMakeFiles/mosc-try.dir/main.try.c.o -c /Users/doumbia0804/CLionProjects/mosc/try/main.try.c

CMakeFiles/mosc-try.dir/main.try.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Preprocessing C source to CMakeFiles/mosc-try.dir/main.try.c.i"
	/Users/doumbia0804/Documents/2023/Labs/emsdk/upstream/emscripten/emcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /Users/doumbia0804/CLionProjects/mosc/try/main.try.c > CMakeFiles/mosc-try.dir/main.try.c.i

CMakeFiles/mosc-try.dir/main.try.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Compiling C source to assembly CMakeFiles/mosc-try.dir/main.try.c.s"
	/Users/doumbia0804/Documents/2023/Labs/emsdk/upstream/emscripten/emcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /Users/doumbia0804/CLionProjects/mosc/try/main.try.c -o CMakeFiles/mosc-try.dir/main.try.c.s

# Object files for target mosc-try
mosc__try_OBJECTS = \
"CMakeFiles/mosc-try.dir/main.try.c.o"

# External object files for target mosc-try
mosc__try_EXTERNAL_OBJECTS =

mosc-try.html: CMakeFiles/mosc-try.dir/main.try.c.o
mosc-try.html: CMakeFiles/mosc-try.dir/build.make
mosc-try.html: libmosc.a
mosc-try.html: CMakeFiles/mosc-try.dir/linkLibs.rsp
mosc-try.html: CMakeFiles/mosc-try.dir/objects1.rsp
mosc-try.html: CMakeFiles/mosc-try.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --bold --progress-dir=/Users/doumbia0804/CLionProjects/mosc/try/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking C executable mosc-try.html"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/mosc-try.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/mosc-try.dir/build: mosc-try.html
.PHONY : CMakeFiles/mosc-try.dir/build

CMakeFiles/mosc-try.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/mosc-try.dir/cmake_clean.cmake
.PHONY : CMakeFiles/mosc-try.dir/clean

CMakeFiles/mosc-try.dir/depend:
	cd /Users/doumbia0804/CLionProjects/mosc/try/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /Users/doumbia0804/CLionProjects/mosc/try /Users/doumbia0804/CLionProjects/mosc/try /Users/doumbia0804/CLionProjects/mosc/try/build /Users/doumbia0804/CLionProjects/mosc/try/build /Users/doumbia0804/CLionProjects/mosc/try/build/CMakeFiles/mosc-try.dir/DependInfo.cmake "--color=$(COLOR)"
.PHONY : CMakeFiles/mosc-try.dir/depend

