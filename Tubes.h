#pragma once
#include "TubesLibraryDefine.h"

class Tubes {
public:
	TUBES_API bool Initialize();
	TUBES_API void Shutdown();
	TUBES_API void Update();

private:
	bool m_Initialized = false;
};