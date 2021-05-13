#include "EventSystem.hpp"

void Core::EventSystem::SubscribeToEvent(EventCode code, OnEventFunc fnc, void* const listener)
{
	registeredEvents_[code][listener] = fnc;
}

void Core::EventSystem::UnsubscribeFromEvent(EventCode code, void* const listener)
{
	registeredEvents_[code].erase(listener);
}

bool Core::EventSystem::SignalEvent(EventCode code, EventData context)
{
	bool returnValue = false;
	for (const auto& listenerEntry : registeredEvents_[code])
	{
		returnValue |= listenerEntry.second(code, context);
	}
	return returnValue;
}
