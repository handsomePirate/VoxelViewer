#pragma once
#include "Common.hpp"

class Platform
{
public:
	Platform();
	~Platform();

	class Window
	{
	public:
		uint64_t GetSystemID() const;
		void PollMessages();
	private:
		friend class Platform;
		Window();
		~Window();
		struct Private;
		Private* p_;
	};

	static void OutputMessage(const char* message, uint8_t color);
	static void Sleep(uint64_t ms);
	Window* CreateWindow(const char* name, uint32_t x, uint32_t y, uint32_t width, uint32_t height) const;
	void DestroyWindow(Window* window) const;

private:
	struct Private;
	Private* p_;
};