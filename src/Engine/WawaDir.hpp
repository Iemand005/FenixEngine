#pragma once
#ifdef _WIN32
#define NOMINMAX
#include <Windows.h>
#endif


#include <cstdio>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <filesystem>

namespace fe {

	inline std::string GetExecutableDirectorye()
	{
		char exePath[MAX_PATH] = {0};
		GetModuleFileNameA(NULL, exePath, MAX_PATH);
		
		std::string fullPath(exePath);
		size_t lastSlash = fullPath.find_last_of("\\/");
		if (lastSlash != std::string::npos)
		{
			return fullPath.substr(0, lastSlash);
		}
		return "";
	}
}