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
