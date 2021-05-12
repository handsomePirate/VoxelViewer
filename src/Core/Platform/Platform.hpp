#pragma once
#include "Core/Common.hpp"
#include "Core/Singleton.hpp"

namespace Core
{
	const int MaxWindows = 1;

	class Window
	{
	public:
		uint64_t GetHandle() const;
		void PollMessages();
		bool ShouldClose() const;
		bool IsMinimized() const;

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

		static uint64_t GetProgramID();

		static const char* GetVulkanSurfacePlatformExtension();

		void Quit();

	private:
		struct Private;
		Private* p_;
	};

	class Filesystem
	{
	public:
		Filesystem();
		~Filesystem();

		std::string ExecutableName() const;
		std::string GetAbsolutePath(const std::string& relativePath) const;
		bool FileExists(const std::string& path) const;

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
#define CoreFilesystem (::Core::Singleton<::Core::Filesystem>::GetInstance())