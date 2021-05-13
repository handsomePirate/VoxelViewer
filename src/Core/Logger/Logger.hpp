#pragma once
#include "Core/Common.hpp"
#include "Core/Singleton.hpp"
#include "ImGuiLogger.hpp"

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

	enum class LoggerType
	{
		Console = 1,
		ImGui = 2,
		Both = 3
	};

	class Logger
	{
	public:
		Logger() = default;
		~Logger() = default;

		void SetTypes(LoggerType type);
		void AddTypes(LoggerType type);
		void RemoveTypes(LoggerType type);
		LoggerType GetTypes() const;

		void Log(LoggerSeverity severity, const char* message, ...);

		void DrawImGuiLogger(const char* title, bool* p_open = NULL);

	private:
		LoggerType type_ = LoggerType::Both;
		ImGuiLogger imGuiLogger_;
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
