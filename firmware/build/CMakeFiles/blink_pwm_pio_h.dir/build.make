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
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/dmytro/Documents/projects/TOF_scanner/firmware

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/dmytro/Documents/projects/TOF_scanner/firmware/build

# Utility rule file for blink_pwm_pio_h.

# Include any custom commands dependencies for this target.
include CMakeFiles/blink_pwm_pio_h.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/blink_pwm_pio_h.dir/progress.make

CMakeFiles/blink_pwm_pio_h: pwm.pio.h

pwm.pio.h: /home/dmytro/Documents/projects/TOF_scanner/firmware/pwm.pio
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --blue --bold --progress-dir=/home/dmytro/Documents/projects/TOF_scanner/firmware/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Generating pwm.pio.h"
	pioasm/pioasm -o c-sdk /home/dmytro/Documents/projects/TOF_scanner/firmware/pwm.pio /home/dmytro/Documents/projects/TOF_scanner/firmware/build/pwm.pio.h

blink_pwm_pio_h: CMakeFiles/blink_pwm_pio_h
blink_pwm_pio_h: pwm.pio.h
blink_pwm_pio_h: CMakeFiles/blink_pwm_pio_h.dir/build.make
.PHONY : blink_pwm_pio_h

# Rule to build all files generated by this target.
CMakeFiles/blink_pwm_pio_h.dir/build: blink_pwm_pio_h
.PHONY : CMakeFiles/blink_pwm_pio_h.dir/build

CMakeFiles/blink_pwm_pio_h.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/blink_pwm_pio_h.dir/cmake_clean.cmake
.PHONY : CMakeFiles/blink_pwm_pio_h.dir/clean

CMakeFiles/blink_pwm_pio_h.dir/depend:
	cd /home/dmytro/Documents/projects/TOF_scanner/firmware/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/dmytro/Documents/projects/TOF_scanner/firmware /home/dmytro/Documents/projects/TOF_scanner/firmware /home/dmytro/Documents/projects/TOF_scanner/firmware/build /home/dmytro/Documents/projects/TOF_scanner/firmware/build /home/dmytro/Documents/projects/TOF_scanner/firmware/build/CMakeFiles/blink_pwm_pio_h.dir/DependInfo.cmake "--color=$(COLOR)"
.PHONY : CMakeFiles/blink_pwm_pio_h.dir/depend

