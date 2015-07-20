#pragma once

#ifdef _WIN32
#ifdef TUBES_DLL_EXPORT
#define TUBES_API __declspec(dllexport)
#else
#define TUBES_API __declspec(dllimport)
#endif
#else
#define TUBES_API
#endif