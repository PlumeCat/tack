# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.23

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
CMAKE_COMMAND = /opt/homebrew/Cellar/cmake/3.23.0/bin/cmake

# The command to remove a file.
RM = /opt/homebrew/Cellar/cmake/3.23.0/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /Users/james/code/tack

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /Users/james/code/tack/build

# Include any dependencies generated for this target.
include CMakeFiles/pl5.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include CMakeFiles/pl5.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/pl5.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/pl5.dir/flags.make

CMakeFiles/pl5.dir/src/pl5.cpp.o: CMakeFiles/pl5.dir/flags.make
CMakeFiles/pl5.dir/src/pl5.cpp.o: ../src/pl5.cpp
CMakeFiles/pl5.dir/src/pl5.cpp.o: CMakeFiles/pl5.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/james/code/tack/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/pl5.dir/src/pl5.cpp.o"
	/Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/pl5.dir/src/pl5.cpp.o -MF CMakeFiles/pl5.dir/src/pl5.cpp.o.d -o CMakeFiles/pl5.dir/src/pl5.cpp.o -c /Users/james/code/tack/src/pl5.cpp

CMakeFiles/pl5.dir/src/pl5.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/pl5.dir/src/pl5.cpp.i"
	/Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /Users/james/code/tack/src/pl5.cpp > CMakeFiles/pl5.dir/src/pl5.cpp.i

CMakeFiles/pl5.dir/src/pl5.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/pl5.dir/src/pl5.cpp.s"
	/Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /Users/james/code/tack/src/pl5.cpp -o CMakeFiles/pl5.dir/src/pl5.cpp.s

# Object files for target pl5
pl5_OBJECTS = \
"CMakeFiles/pl5.dir/src/pl5.cpp.o"

# External object files for target pl5
pl5_EXTERNAL_OBJECTS =

pl5: CMakeFiles/pl5.dir/src/pl5.cpp.o
pl5: CMakeFiles/pl5.dir/build.make
pl5: CMakeFiles/pl5.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/Users/james/code/tack/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable pl5"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/pl5.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/pl5.dir/build: pl5
.PHONY : CMakeFiles/pl5.dir/build

CMakeFiles/pl5.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/pl5.dir/cmake_clean.cmake
.PHONY : CMakeFiles/pl5.dir/clean

CMakeFiles/pl5.dir/depend:
	cd /Users/james/code/tack/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /Users/james/code/tack /Users/james/code/tack /Users/james/code/tack/build /Users/james/code/tack/build /Users/james/code/tack/build/CMakeFiles/pl5.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/pl5.dir/depend

