#pragma once

#include <ctime>
#include <cstdarg>
#include <cstdio>
#include <string>

#if defined(__ANDROID__) && __has_include(<android/log.h>)
#include <android/log.h>
#define FE_LOGCAT_AVAILABLE 1
#else
#define FE_LOGCAT_AVAILABLE 0
#define ANDROID_LOG_INFO 4
#define ANDROID_LOG_WARN 5
#define ANDROID_LOG_ERROR 6
#endif

namespace fe {

	// Tag used for logcat output. Games can override it at startup so their
	// messages show up under their own tag: fe::LogSetTag("MyGame").
	inline const char*& LogTag() {
		static const char* tag = "FenixEngine";
		return tag;
	}

	inline void LogSetTag(const char* tag) { LogTag() = tag; }

	inline int LogToLogcat(int level, const char* fmt, va_list args) {
#if FE_LOGCAT_AVAILABLE
		return __android_log_vprint(level, LogTag(), fmt, args);
#else
		(void)level;
		(void)fmt;
		(void)args;
		return 0;
#endif
	}

	enum class LogLevel {
		Info,
		Warning,
		Error
	};

	inline void LogV(LogLevel level, const char* fmt, va_list args) {
		const bool toStdout = level == LogLevel::Info;
		FILE* stream = toStdout ? stdout : stderr;
		va_list streamArgs;
		va_copy(streamArgs, args);
		vfprintf(stream, fmt, streamArgs);
		va_end(streamArgs);
		fprintf(stream, "\n");
		fflush(stream);

		const int androidLevel = level == LogLevel::Info ? ANDROID_LOG_INFO
			: level == LogLevel::Warning ? ANDROID_LOG_WARN : ANDROID_LOG_ERROR;
		va_list logcatArgs;
		va_copy(logcatArgs, args);
		LogToLogcat(androidLevel, fmt, logcatArgs);
		va_end(logcatArgs);
	}

	inline void Log(const char* fmt, ...) {
		va_list args;
		va_start(args, fmt);
		LogV(LogLevel::Info, fmt, args);
		va_end(args);
	}

	inline void LogError(const char* fmt, ...) {
		va_list args;
		va_start(args, fmt);
		LogV(LogLevel::Error, fmt, args);
		va_end(args);
	}

	inline void LogWarning(const char* fmt, ...) {
		va_list args;
		va_start(args, fmt);
		LogV(LogLevel::Warning, fmt, args);
		va_end(args);
	}

	// Convenience overloads so callers can pass std::string directly, like the
	// old Game::Log(std::string) did.
	inline void Log(const std::string& message) { Log("%s", message.c_str()); }
	inline void LogError(const std::string& message) { LogError("%s", message.c_str()); }
	inline void LogWarning(const std::string& message) { LogWarning("%s", message.c_str()); }

	inline std::string& LogFilePath() {
		static std::string* path = new std::string(
#ifdef _WIN32
			"C:\\Temp\\FenixEngine.log"
#else
			"/tmp/FenixEngine.log"
#endif
		);
		return *path;
	}

	inline void LogSetFilePath(const std::string& path) { LogFilePath() = path; }

	inline void LogToFile(const std::string& message) {
		Log("%s", message.c_str());
		FILE* file = fopen(LogFilePath().c_str(), "ab");
		if (!file) return;
		std::time_t now = std::time(nullptr);
		std::tm tmStruct;
#ifdef _WIN32
		localtime_s(&tmStruct, &now);
#else
		localtime_r(&now, &tmStruct);
#endif
		char timeBuf[64] = {0};
		std::strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", &tmStruct);
		fprintf(file, "[%s] %s\n", timeBuf, message.c_str());
		fclose(file);
	}

} // namespace fe