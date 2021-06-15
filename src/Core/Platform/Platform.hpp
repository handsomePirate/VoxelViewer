#pragma once
#include "Core/Common.hpp"
#include "Core/Singleton.hpp"
#include <filesystem>

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

		uint32_t GetWidth() const;
		uint32_t GetHeight() const;

		void ClipMousePosition(uint16_t& x, uint16_t& y) const;

	private:
		friend class Platform;
		Window();
		~Window();
		struct Private;
		Private* p_;
	};

	enum class CursorType
	{
		None = -1,
		Arrow = 0,
		TextInput,
		ResizeAll,
		ResizeNS,
		ResizeEW,
		ResizeNESW,
		ResizeNWSE,
		Hand,
		NotAllowed,

		CursorTypeCount
	};

	class Platform
	{
	public:
		Platform();
		~Platform();

		static void OutputMessage(const char* message, uint8_t color);
		static void Sleep(uint32_t ms);
		Window* GetNewWindow(const char* name, uint32_t x, uint32_t y, uint32_t width, uint32_t height,
			bool resizable) const;
		void DeleteWindow(Window* window) const;

		void SetCursor(CursorType type) const;

		static uint64_t GetProgramID();

		static const char* GetVulkanSurfacePlatformExtension();

		uint64_t LoadDynamicLibrary(const std::string& path);
		void UnloadDynamicLibrary(uint64_t handle);

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

		bool IsPathRelative(const std::string& path) const;
		std::string ExecutableName() const;
		std::string GetAbsolutePath(const std::string& relativePath) const;
		bool FileExists(const std::string& path) const;
		size_t GetFileSize(const std::string& path) const;
		void ReadFile(const std::string& path, void* data, size_t size) const;
		std::string Filename(const std::string& path) const;
		std::string Extension(const std::string& path) const;
		std::string RemoveExtension(const std::string& path) const;
		void RemoveExtension(std::string& path) const;

	private:
		struct Private;
		Private* p_;
	};
}

#define CorePlatform (::Core::Singleton<::Core::Platform>::GetInstance())
#define CoreFilesystem (::Core::Singleton<::Core::Filesystem>::GetInstance())