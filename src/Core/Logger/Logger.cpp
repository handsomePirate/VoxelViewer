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
#ifdef IMGUI_LOGGER_USE_COLORS
		case Core::LoggerSeverity::Fatal:
			imGuiLogger_.Log({ 1.f, .3f, .3f, 1.f }, "[Fatal] %s", buffer);
			break;
		case Core::LoggerSeverity::Error:
			imGuiLogger_.Log({ .9f, 0.f, 0.f, 1.f }, "[Error] %s", buffer);
			break;
		case Core::LoggerSeverity::Warn:
			imGuiLogger_.Log({ .8f, .7f, 0.f, 1.f }, "[Warn] %s", buffer);
			break;
		case Core::LoggerSeverity::Info:
			imGuiLogger_.Log({ .9f, .9f, .9f, 1.f }, "[Info] %s", buffer);
			break;
		case Core::LoggerSeverity::Debug:
			imGuiLogger_.Log({ .4f, .8f, .4f, 1.f }, "[Debug] %s", buffer);
			break;
		case Core::LoggerSeverity::Trace:
			imGuiLogger_.Log({ .5f, .5f, .5f, .8f }, "[Trace] %s", buffer);
			break;
#else
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
#endif
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
