cmake_minimum_required(VERSION 3.8.2)
set(CMAKE_CONFIGURATION_TYPES Debug Release)

# Project name
project(Tubes CXX)

# OS Name
string(TOLOWER ${CMAKE_SYSTEM_NAME} OperatingSystemNameLowerCase)

# Root directory
set(ProjectRoot ${CMAKE_CURRENT_LIST_DIR}/../)
get_filename_component(ProjectRootAbsolute ${ProjectRoot} ABSOLUTE)

# Get local(non versioned) paths to dependencies (Note: These values will be inherited to subdirectories if the subdirectories do not specify their own paths)
include("${CMAKE_CURRENT_LIST_DIR}/CMakeLocalDependenciesPaths.txt" OPTIONAL)

# Visual Studio settings
if(MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP /D _CRT_SECURE_NO_WARNINGS")
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
set(MUtilityLocalIncludePath ${IncludeDirectoryAbsolute}/MUtility)
set(MUtilityLocalDebugLibPath "${DebugLibraryDirectoryAbsolute}/MUtility")
set(MUtilityLocalReleaseLibPath "${ReleaseLibraryDirectoryAbsolute}/MUtility")

if(MUtilityRootPath)
	message("[${PROJECT_NAME}] - Found MUtility project at path ${MUtilityRootPath}")
	set(MUtilityCMakePath "${MUtilityRootPath}/project")
	set(MUtilityIncludePath "${MUtilityRootPath}/source/interface")
	set(MUtilityDebugLibPath "${MUtilityRootPath}/output/Debug")
	set(MUtilityReleaseLibPath "${MUtilityRootPath}/output/Release")

	set(MUtilityIncludes ${MUtilityIncludePath})
	set(MUtilityDebugLibs ${MUtilityDebugLibPath})
	set(MUtilityReleaseLibs ${MUtilityReleaseLibPath})
else(MUtilityRootPath)
	set(MUtilityIncludes ${MUtilityLocalIncludePath})
	set(MUtilityDebugLibs ${MUtilityLocalDebugLibPath})
	set(MUtilityReleaseLibs ${MUtilityLocalReleaseLibPath})
endif(MUtilityRootPath)

# List include directories
set(IncludeDirectoryList
	${MUtilityIncludes}
)
message("[${PROJECT_NAME}] - Include directories are:")
foreach(IncludeDir ${IncludeDirectoryList})
	message("	${IncludeDir}")
endforeach(IncludeDir)

# List source file directories
file(GLOB_RECURSE SOURCE_DIRECTORIES
	"../source/*.h"
	"../source/*.cpp"
)

# Create static library
ADD_LIBRARY( ${PROJECT_NAME} STATIC
   	${SOURCE_DIRECTORIES}
)

# Set include directories
set_property(TARGET ${PROJECT_NAME} PROPERTY INCLUDE_DIRECTORIES ${IncludeDirectoryList})

# Set output directory
set_target_properties(${PROJECT_NAME} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${ProjectRootAbsolute}/output/)

if(MUtilityRootPath)
	add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${MUtilityIncludePath} ${MUtilityLocalIncludePath})
	add_custom_command(TARGET ${PROJECT_NAME} PRE_LINK COMMAND ${CMAKE_COMMAND} -E copy_directory ${MUtilityDebugLibPath} ${MUtilityLocalDebugLibPath})
	add_custom_command(TARGET ${PROJECT_NAME} PRE_LINK COMMAND ${CMAKE_COMMAND} -E copy_directory ${MUtilityReleaseLibPath} ${MUtilityLocalReleaseLibPath})
	if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
		add_subdirectory(${MUtilityCMakePath} ${MUtilityCMakePath})
	endif(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
endif(MUtilityRootPath)

# --- DEBUG COPY PASTE ---
#message("|||--- TEST ---||| ${var}")