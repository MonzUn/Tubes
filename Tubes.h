#pragma once
#include "TubesLibraryDefine.h"

class Tubes {
public:
	TUBES_API bool Initialize();
	TUBES_API void Shutdown();
	TUBES_API void Update();

	TUBES_API bool GetHostFlag() const;

	TUBES_API void SetHostFlag( bool isHost );

private:
	bool m_Initialized	= false;
	bool m_HostFlag		= false;
};