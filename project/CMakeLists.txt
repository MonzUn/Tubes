cmake_minimum_required(VERSION 3.0.2)
set(CMAKE_CONFIGURATION_TYPES Debug Release)

# Project name
project(Tubes CXX)

# OS Name
string(TOLOWER ${CMAKE_SYSTEM_NAME} OperatingSystemNameLowerCase)

# Root directory
set(ProjectRoot ${CMAKE_CURRENT_LIST_DIR}/../)
get_filename_component(ProjectRootAbsolute ${ProjectRoot} ABSOLUTE)

# Visual Studio settings
if(MSVC)
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP /D \"_CRT_SECURE_NO_WARNINGS\"")
endif(MSVC)

# Include directory
file(RELATIVE_PATH IncludeDirectoryRelative ${Tubes_SOURCE_DIR} ${ProjectRootAbsolute}/include)
get_filename_component(IncludeDirectoryAbsolute ${IncludeDirectoryRelative} ABSOLUTE)

# Debug library directory
file(RELATIVE_PATH DebugLibraryDirectoryRelative ${Tubes_SOURCE_DIR} ${ProjectRootAbsolute}/lib/debug/x64)
get_filename_component(DebugLibraryDirectoryAbsolute ${DebugLibraryDirectoryRelative} ABSOLUTE)

# Release library directory
file(RELATIVE_PATH ReleaseLibraryDirectoryRelative ${Tubes_SOURCE_DIR} ${ProjectRootAbsolute}/lib/release/x64)
get_filename_component(ReleaseLibraryDirectoryAbsolute ${ReleaseLibraryDirectoryRelative} ABSOLUTE)

# Find MUtility
find_path(MUtilityInclude MUtility/MUtilityByte.h PATHS ${IncludeDirectoryAbsolute})
set(MUtilityIncludes ${MUtilityInclude}/MUtility)

set(MUtilityDebugLibraryDirectoryAbsolute "${DebugLibraryDirectoryAbsolute}/MUtility")
set(MUtilityReleaseLibraryDirectoryAbsolute "${ReleaseLibraryDirectoryAbsolute}/MUtility")

# Set include directories
include_directories(
	${MUtilityIncludes}
)

# List source file directories
file(GLOB_RECURSE SOURCE_DIRECTORIES
	"../source/*.h"
	"../source/*.cpp"
)

# Create static library
ADD_LIBRARY( ${PROJECT_NAME} STATIC
   	${SOURCE_DIRECTORIES}
)

# Set output directory
set_target_properties(${PROJECT_NAME} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${ProjectRootAbsolute}/output/)

# Make sure that the linker language gets set to C++
set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)

# ---------- Dependencies ----------

if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
	# Get local(non versioned) paths to dependencies
	include("${CMAKE_CURRENT_LIST_DIR}/CMakeLocalDependenciesPaths.txt" OPTIONAL)

	# Run CMake of dependencies
	if(MUtilityCMakePath)
		add_subdirectory(${MUtilityCMakePath} "${MUtilityCMakePath}")
	endif(MUtilityCMakePath)
endif(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)

# Add prebuilds scripts that copies local dependencies to their place within the project
if(MUtilityIncludeCopyPath)
	add_custom_command(TARGET ${PROJECT_NAME}
	PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${MUtilityIncludeCopyPath}
        ${MUtilityIncludes})
endif(MUtilityIncludeCopyPath)

if(MUtilityDebugLibCopyPath)
	add_custom_command(TARGET ${PROJECT_NAME}
	PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${MUtilityDebugLibCopyPath}
        ${MUtilityDebugLibraryDirectoryAbsolute})
endif(MUtilityDebugLibCopyPath)

if(MUtilityReleaseLibCopyPath)
	add_custom_command(TARGET ${PROJECT_NAME}
	PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${MUtilityReleaseLibCopyPath}
        ${MUtilityReleaseLibraryDirectoryAbsolute})
endif(MUtilityReleaseLibCopyPath)

# --- DEBUG COPY PASTE ---
#message("|||--- TEST ---||| ${var}")