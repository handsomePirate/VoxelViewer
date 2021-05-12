#include "Logger.hpp"
#include "Core/Platform/Platform.hpp"

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

	CorePlatform.OutputMessage(buffer, (uint8_t)severity);
}
