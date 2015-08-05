file(GLOB_RECURSE TubesSources
	"Tubes.h"
	"Tubes.cpp"
	"TubesLibraryDefine.h"
	"TubesUtility.h"
	"TubesUtility.cpp"
	"TubesTypes.h"
	"TubesTypes.cpp"
	"TubesErrors.h"
	"Connection.h"
	"Connection.cpp"
	"ConnectionManager.h"
	"ConnectionManager.cpp"
	"Communication.h"
	"Communication.cpp"
)

add_definitions(-DTUBES_DLL_EXPORT)
add_library(Tubes SHARED ${TubesSources})
target_link_libraries(Tubes Utility)
target_link_libraries(Tubes Memory)

SET_TARGET_PROPERTIES(Tubes PROPERTIES LINKER_LANGUAGE CXX) #TODODB: Find out why we need to set this or remove it when we don't need to set it anymore (Cmake should figure it out from the source files)

install(
	TARGETS Tubes DESTINATION lib
	LIBRARY DESTINATION lib
	RUNTIME DESTINATION bin/${OperatingSystemNameLowerCase}/${TargetArchitecture})
