# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.22

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
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /tmp/tmp.dIrX13OUnU

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /tmp/tmp.dIrX13OUnU/cmake-build-debug

# Include any dependencies generated for this target.
include CMakeFiles/db_test.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include CMakeFiles/db_test.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/db_test.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/db_test.dir/flags.make

CMakeFiles/db_test.dir/file_test.o: CMakeFiles/db_test.dir/flags.make
CMakeFiles/db_test.dir/file_test.o: ../file_test.cc
CMakeFiles/db_test.dir/file_test.o: CMakeFiles/db_test.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/tmp/tmp.dIrX13OUnU/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/db_test.dir/file_test.o"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/db_test.dir/file_test.o -MF CMakeFiles/db_test.dir/file_test.o.d -o CMakeFiles/db_test.dir/file_test.o -c /tmp/tmp.dIrX13OUnU/file_test.cc

CMakeFiles/db_test.dir/file_test.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/db_test.dir/file_test.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /tmp/tmp.dIrX13OUnU/file_test.cc > CMakeFiles/db_test.dir/file_test.i

CMakeFiles/db_test.dir/file_test.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/db_test.dir/file_test.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /tmp/tmp.dIrX13OUnU/file_test.cc -o CMakeFiles/db_test.dir/file_test.s

# Object files for target db_test
db_test_OBJECTS = \
"CMakeFiles/db_test.dir/file_test.o"

# External object files for target db_test
db_test_EXTERNAL_OBJECTS =

db_test: CMakeFiles/db_test.dir/file_test.o
db_test: CMakeFiles/db_test.dir/build.make
db_test: lib/libgtest_maind.a
db_test: lib/libgtestd.a
db_test: CMakeFiles/db_test.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/tmp/tmp.dIrX13OUnU/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable db_test"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/db_test.dir/link.txt --verbose=$(VERBOSE)
	/usr/bin/cmake -D TEST_TARGET=db_test -D TEST_EXECUTABLE=/tmp/tmp.dIrX13OUnU/cmake-build-debug/db_test -D TEST_EXECUTOR= -D TEST_WORKING_DIR=/tmp/tmp.dIrX13OUnU/cmake-build-debug -D TEST_EXTRA_ARGS= -D TEST_PROPERTIES= -D TEST_PREFIX= -D TEST_SUFFIX= -D TEST_FILTER= -D NO_PRETTY_TYPES=FALSE -D NO_PRETTY_VALUES=FALSE -D TEST_LIST=db_test_TESTS -D CTEST_FILE=/tmp/tmp.dIrX13OUnU/cmake-build-debug/db_test[1]_tests.cmake -D TEST_DISCOVERY_TIMEOUT=5 -D TEST_XML_OUTPUT_DIR= -P /usr/share/cmake-3.22/Modules/GoogleTestAddTests.cmake

# Rule to build all files generated by this target.
CMakeFiles/db_test.dir/build: db_test
.PHONY : CMakeFiles/db_test.dir/build

CMakeFiles/db_test.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/db_test.dir/cmake_clean.cmake
.PHONY : CMakeFiles/db_test.dir/clean

CMakeFiles/db_test.dir/depend:
	cd /tmp/tmp.dIrX13OUnU/cmake-build-debug && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /tmp/tmp.dIrX13OUnU /tmp/tmp.dIrX13OUnU /tmp/tmp.dIrX13OUnU/cmake-build-debug /tmp/tmp.dIrX13OUnU/cmake-build-debug /tmp/tmp.dIrX13OUnU/cmake-build-debug/CMakeFiles/db_test.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/db_test.dir/depend
