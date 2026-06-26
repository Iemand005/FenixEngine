#pragma once
#ifdef _WIN32
#define NOMINMAX
#include <Windows.h>
#else
#include <unistd.h>
#include <limits.h>
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
		#ifdef _WIN32
		char exePath[MAX_PATH] = {0};
		GetModuleFileNameA(NULL, exePath, MAX_PATH);

		std::string fullPath(exePath);
		size_t lastSlash = fullPath.find_last_of("\\/");
		if (lastSlash != std::string::npos)
		{
			return fullPath.substr(0, lastSlash);
		}
		return "";
		#else
		// Linux: use /proc/self/exe
		char exePath[PATH_MAX] = {0};
		ssize_t len = readlink("/proc/self/exe", exePath, PATH_MAX - 1);

		if (len != -1)
		{
			exePath[len] = '\0';
			std::string fullPath(exePath);
			size_t lastSlash = fullPath.find_last_of("/");
			if (lastSlash != std::string::npos)
			{
				return fullPath.substr(0, lastSlash);
			}
		}
		return "";
		#endif
	}
}
