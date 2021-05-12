#pragma once
#include "Core/Common.hpp"
#include "Core/Singleton.hpp"

namespace Core
{
	enum class LoggerSeverity
	{
		Fatal,
		Error,
		Warn,
		Info,
		Debug,
		Trace
	};

	const int MaxMessageLength = 1024;

	class Logger
	{
	public:
		Logger() = default;
		~Logger() = default;

		void Log(LoggerSeverity severity, const char* message, ...);
	private:
	};
}

#define CoreLogger (::Core::Singleton<::Core::Logger>::GetInstance())

#if defined(DEBUG) && defined(LOGGER_DO_TRACE)
#define CoreLogTrace(format, ...) CoreLogger.Log(::Core::LoggerSeverity::Trace, format, ##__VA_ARGS__)
#else
#define CoreLogTrace(format, ...)
#endif

#ifdef DEBUG
#define CoreLogDebug(format, ...) CoreLogger.Log(::Core::LoggerSeverity::Debug, format, ##__VA_ARGS__)
#else
#define CoreLogDebug(format, ...)
#endif

#define CoreLogInfo(format, ...) CoreLogger.Log(::Core::LoggerSeverity::Info, format, ##__VA_ARGS__)
#define CoreLogWarn(format, ...) CoreLogger.Log(::Core::LoggerSeverity::Warn, format, ##__VA_ARGS__)
#define CoreLogError(format, ...) CoreLogger.Log(::Core::LoggerSeverity::Error, format, ##__VA_ARGS__)
#define CoreLogFatal(format, ...) CoreLogger.Log(::Core::LoggerSeverity::Fatal, format, ##__VA_ARGS__)
