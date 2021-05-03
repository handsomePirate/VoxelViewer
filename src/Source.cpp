#include "Common.hpp"
#include "Platform/Platform.hpp"
#include "Logger/Logger.hpp"
#include "Events/EventSystem.hpp"
#include <vector>
#include <string>
#include <functional>

#include <iostream>

struct ButtonPressedLogger
{
	bool KeyPressed(Core::EventCode code, Core::EventData context)
	{
		std::string keyName;

		switch (context.data.u16[0])
		{
		case (uint16_t)Core::Key::Ascii:
			keyName = (unsigned char)context.data.u16[1];
			break;
		case (uint16_t)Core::Key::Space:
			keyName = "Space";
			break;
		case (uint16_t)Core::Key::Enter:
			keyName = "Enter";
			break;
		case (uint16_t)Core::Key::Escape:
			keyName = "Esc";
			CorePlatform.Quit();
			break;
		case (uint16_t)Core::Key::Arrow:
			static std::string arrowDirections[] = { "left", "right", "up", "down" };
			keyName = "Arrow " + arrowDirections[context.data.u16[1]];
			break;
		}

		CoreLogger.Log(Core::LoggerSeverity::Trace, "Button pressed; data = %s", keyName.c_str());
		return true;
	}
};

int main(int argc, char* argv[])
{
	ButtonPressedLogger dummy;
	auto fnc = std::bind(&ButtonPressedLogger::KeyPressed, &dummy, std::placeholders::_1, std::placeholders::_2);
	CoreEventSystem.SubscribeToEvent(Core::EventCode::KeyPressed, fnc, &dummy);
	std::vector<Core::Window*> windows;
	int activeWindows = 0;
	for (int i = 0; i < Core::MaxWindows; ++i)
	{
		std::string windowName = "Window" + std::to_string(i + 1);
		windows.push_back(CorePlatform.GetNewWindow(windowName.c_str(), 50, 50, 640, 480));
		if (windows[i])
		{
			++activeWindows;
		}
	}
	while (activeWindows != 0)
	{
		for (int i = 0; i < windows.size(); ++i)
		{
			if (windows[i])
			{
				windows[i]->PollMessages();
				if (windows[i]->ShouldClose())
				{
					CorePlatform.DeleteWindow(windows[i]);
					windows[i] = nullptr;
					--activeWindows;
				}
			}
		}
	}
	
	return 0;
}