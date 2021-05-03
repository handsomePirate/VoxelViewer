#pragma once
#include "Common.hpp"
#include "Singleton.hpp"

namespace Core
{
	const int MaxWindows = 1;

	class Window
	{
	public:
		uint64_t GetSystemID() const;
		void PollMessages();
		bool ShouldClose() const;

		void SetShouldClose();
	private:
		friend class Platform;
		Window();
		~Window();
		struct Private;
		Private* p_;
	};

	class Platform
	{
	public:
		Platform();
		~Platform();

		static void OutputMessage(const char* message, uint8_t color);
		static void Sleep(uint32_t ms);
		Window* GetNewWindow(const char* name, uint32_t x, uint32_t y, uint32_t width, uint32_t height) const;
		void DeleteWindow(Window* window) const;

		void Quit();

	private:
		struct Private;
		Private* p_;
	};

	enum class Key
	{
		Enter,
		Space,
		Ascii,
		Escape,
		Arrow,

		Max = 0xFF
	};

	enum class MouseButton
	{
		Left,
		Right,

		Max = 0xFF
	};
}

#define CorePlatform (::Core::Singleton<::Core::Platform>::GetInstance())