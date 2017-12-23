cmake_minimum_required(VERSION 3.0.2)
set(CMAKE_CONFIGURATION_TYPES Debug Release)

# Project name
project(Tubes CXX)

# OS Name
string(TOLOWER ${CMAKE_SYSTEM_NAME} OperatingSystemNameLowerCase)

# Root directory
set(ProjectRoot ${Tubes_SOURCE_DIR}/../)
get_filename_component(ProjectRootAbsolute ${ProjectRoot} ABSOLUTE)

# Visual Studio settings
if(MSVC)
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP /D \"_CRT_SECURE_NO_WARNINGS\"")
endif(MSVC)

# Include directory
file(RELATIVE_PATH IncludeDirectoryRelative ${Tubes_SOURCE_DIR} ${ProjectRootAbsolute}/include)
get_filename_component(IncludeDirectoryAbsolute ${IncludeDirectoryRelative} ABSOLUTE)

# Library directory
file(RELATIVE_PATH LibraryDirectoryRelative ${Tubes_SOURCE_DIR} ${ProjectRootAbsolute}/lib/x64)
get_filename_component(LibraryDirectoryAbsolute ${LibraryDirectoryRelative} ABSOLUTE)

# Set output directory
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${ProjectRoot}/output/)

# Ensure build type set
if (NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Debug)
endif()

# Find MUtility
find_path(MUtilityInclude MUtility/MUtilityByte.h PATHS ${IncludeDirectoryAbsolute})
set(MUtilityIncludes ${MUtilityInclude}/MUtility)

set(MUtilityLibraryDirectoryAbsolute "${LibraryDirectoryAbsolute}/MUtility")
find_library(MUtilityLibrary
	NAMES MUtility
	PATHS ${MUtilityLibraryDirectoryAbsolute})
set(MUtilityLibraries ${MUtilityLibrary})

# Set lib and include directories
link_directories(
	${MUtilityLibraries}
)

include_directories(
	${MUtilityIncludes}
)

# List source file directories
file(GLOB_RECURSE SOURCE_DIRECTORIES
	"../source/*.h"
	"../source/*.cpp"
)

# Create static library
ADD_LIBRARY( ${CMAKE_PROJECT_NAME} STATIC
   	${SOURCE_DIRECTORIES}
)

# Make sure that the linker language gets set to C++
set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)

# Get local(non versioned) paths to dependencies
include("CMakeLocalDependenciesPaths.txt" OPTIONAL)

# Add prebuilds scripts that copies local dependencies to their place within the project
if(MUtilityIncludeCopyPath)
	add_custom_command(TARGET ${CMAKE_PROJECT_NAME}
	PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${MUtilityIncludeCopyPath}
        ${MUtilityIncludes})
endif(MUtilityIncludeCopyPath)

if(MUtilityLibCopyPath)
	add_custom_command(TARGET ${CMAKE_PROJECT_NAME}
	PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${MUtilityLibCopyPath}
        ${MUtilityLibraryDirectoryAbsolute})
endif(MUtilityLibCopyPath)

# --- DEBUG COPY PASTE ---
#message("|||--- TEST ---||| ${var}")