#include "Input.hpp"
#include "Core/Logger/Logger.hpp"

Core::Input::Input()
{
	memset(keysPressed_, 0, sizeof(bool) * 256);
	memset(mouseButtonsPressed_, 0, sizeof(bool) * 8);

	auto keyPressFnc = std::bind(&Input::KeySetPressed, this, std::placeholders::_1, std::placeholders::_2);
	CoreEventSystem.SubscribeToEvent(Core::EventCode::KeyPressed, keyPressFnc, this);

	auto keyReleaseFnc = std::bind(&Input::KeySetReleased, this, std::placeholders::_1, std::placeholders::_2);
	CoreEventSystem.SubscribeToEvent(Core::EventCode::KeyReleased, keyReleaseFnc, this);

	auto mouseButtonPressFnc = std::bind(&Input::MouseButtonSetPressed, this, std::placeholders::_1, std::placeholders::_2);
	CoreEventSystem.SubscribeToEvent(Core::EventCode::MouseButtonPressed, mouseButtonPressFnc, this);

	auto mouseButtonReleaseFnc = std::bind(&Input::MouseButtonSetReleased, this, std::placeholders::_1, std::placeholders::_2);
	CoreEventSystem.SubscribeToEvent(Core::EventCode::MouseButtonReleased, mouseButtonReleaseFnc, this);

	auto mouseMovedFnc = std::bind(&Input::SetMousePosition, this, std::placeholders::_1, std::placeholders::_2);
	CoreEventSystem.SubscribeToEvent(Core::EventCode::MouseMoved, mouseMovedFnc, this);
}

Core::Input::~Input()
{
	CoreEventSystem.UnsubscribeFromEvent(Core::EventCode::KeyPressed, this);
	CoreEventSystem.UnsubscribeFromEvent(Core::EventCode::KeyReleased, this);
	CoreEventSystem.UnsubscribeFromEvent(Core::EventCode::MouseButtonPressed, this);
	CoreEventSystem.UnsubscribeFromEvent(Core::EventCode::MouseButtonReleased, this);
	CoreEventSystem.UnsubscribeFromEvent(Core::EventCode::MouseMoved, this);
}

bool Core::Input::IsKeyPressed(Keys keyCode) const
{
	return keysPressed_[(int)keyCode];
}

bool Core::Input::IsMouseButtonPressed(MouseButtons buttonCode) const
{
	return mouseButtonsPressed_[(int)buttonCode];
}

uint16_t Core::Input::GetMouseX() const
{
	return mouseX_;
}

uint16_t Core::Input::GetMouseY() const
{
	return mouseY_;
}

bool Core::Input::KeySetPressed(Core::EventCode code, Core::EventData context)
{
	Keys keyCode = (Keys)context.data.u16[0];
	keysPressed_[(int)keyCode] = true;
	return false;
}

bool Core::Input::KeySetReleased(Core::EventCode code, Core::EventData context)
{
	Keys keyCode = (Keys)context.data.u16[0];
	keysPressed_[(int)keyCode] = false;
	return false;
}

bool Core::Input::MouseButtonSetPressed(Core::EventCode code, Core::EventData context)
{
	MouseButtons mouseButtonCode = (MouseButtons)context.data.u16[0];
	mouseButtonsPressed_[(int)mouseButtonCode] = true;
	return false;
}

bool Core::Input::MouseButtonSetReleased(Core::EventCode code, Core::EventData context)
{
	MouseButtons mouseButtonCode = (MouseButtons)context.data.u16[0];
	mouseButtonsPressed_[(int)mouseButtonCode] = false;
	return false;
}

bool Core::Input::SetMousePosition(Core::EventCode code, Core::EventData context)
{
	mouseX_ = context.data.u16[0];
	mouseY_ = context.data.u16[1];
	return false;
}
