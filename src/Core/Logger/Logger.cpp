#include "Logger.hpp"
#include "Core/Platform/Platform.hpp"

#define FORBID_TRACE

void Core::Logger::SetTypes(LoggerType type)
{
	type_ = type;
}

void Core::Logger::AddTypes(LoggerType type)
{
	type_ = (LoggerType)((int)type_ | ((int)type));
}

void Core::Logger::RemoveTypes(LoggerType type)
{
	type_ = (LoggerType)((int)type_ & (~(int)type));
}

Core::LoggerType Core::Logger::GetTypes() const
{
	return type_;
}

void Core::Logger::Log(Core::LoggerSeverity severity, const char* message, ...)
{
	va_list args;
	va_start(args, message);
	
	char buffer[MaxMessageLength];
	char* p = buffer;

	// Do printf % magic
	const int n = _vsnprintf_s(buffer, sizeof buffer - 2, message, args);

	// Make sure the message is properly ended
	p += (n < 0) ? sizeof buffer - 2 : n;

	*p = '\n';
	*(p + 1) = '\0';

	va_end(args);

	if ((int)type_ & (int)LoggerType::ImGui)
	{
		switch (severity)
		{
		case Core::LoggerSeverity::Fatal:
			imGuiLogger_.Log("[Fatal] %s", buffer);
			break;
		case Core::LoggerSeverity::Error:
			imGuiLogger_.Log("[Error] %s", buffer);
			break;
		case Core::LoggerSeverity::Warn:
			imGuiLogger_.Log("[Warn] %s", buffer);
			break;
		case Core::LoggerSeverity::Info:
			imGuiLogger_.Log("[Info] %s", buffer);
			break;
		case Core::LoggerSeverity::Debug:
			imGuiLogger_.Log("[Debug] %s", buffer);
			break;
		case Core::LoggerSeverity::Trace:
			imGuiLogger_.Log("[Trace] %s", buffer);
			break;
		default:
			break;
		}
	}
	if ((int)type_ & (int)LoggerType::Console)
	{
		CorePlatform.OutputMessage(buffer, (uint8_t)severity);
	}
}

void Core::Logger::DrawImGuiLogger(const char* title, bool* open)
{
	if ((int)type_ & (int)LoggerType::ImGui)
	{
		imGuiLogger_.Draw(title, open);
	}
}
