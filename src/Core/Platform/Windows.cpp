#include "Platform.hpp"
#include "Core/Events/EventSystem.hpp"
#include "Core/Input/Input.hpp"
#include "Core/Logger/Logger.hpp"

#ifdef PLATFORM_WINDOWS
#include <Windows.h>

LRESULT CALLBACK ProcessMessage(HWND hwnd, uint32_t msg, WPARAM wParam, LPARAM lParam);

struct Core::Platform::Private
{
    bool initialized = false;
    Window* windows[MaxWindows];
};

inline HINSTANCE GetHInstance()
{
    return ::GetModuleHandleA(0);
}

static const char* windowClassName = "my window class";

Core::Platform::Platform()
	: p_(new Platform::Private)
{
    HINSTANCE hInstance = GetHInstance();
    // Setup and register window class.
    HICON icon = LoadIcon(hInstance, IDI_APPLICATION);
    WNDCLASSA wc;
    memset(&wc, 0, sizeof(wc));
    wc.style = CS_DBLCLKS;
    wc.lpfnWndProc = ProcessMessage;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = icon;
    wc.hCursor = NULL;
    wc.hbrBackground = NULL;
    wc.lpszClassName = windowClassName;

    if (!::RegisterClassA(&wc)) {
        ::MessageBoxA(0, "Window registration failed", "Error", MB_ICONEXCLAMATION | MB_OK);
        return;
    }

    for (int w = 0; w < MaxWindows; ++w)
    {
        p_->windows[w] = nullptr;
    }

    HANDLE consoleHandle = ::GetStdHandle(STD_OUTPUT_HANDLE);
    ::SetConsoleTextAttribute(consoleHandle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);

    p_->initialized = true;
}

Core::Platform::~Platform()
{
    ::UnregisterClassA(windowClassName, GetHInstance());
    delete p_;
    HANDLE consoleHandle = ::GetStdHandle(STD_OUTPUT_HANDLE);
    ::SetConsoleTextAttribute(consoleHandle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
}

struct Core::Window::Window::Private
{
    HWND handle = NULL;
    bool shouldClose = false;

    bool CloseEvent(Core::EventCode code, Core::EventData context)
    {
        HWND otherHandle = (HWND)context.data.u64[0];
        if (otherHandle == handle)
        {
            shouldClose = true;
            return true;
        }
        return false;
    }

    bool QuitEvent(Core::EventCode code, Core::EventData context)
    {
        shouldClose = true;
        return true;
    }
};

uint64_t Core::Window::GetHandle() const
{
    return (uint64_t)p_->handle;
}

void Core::Window::PollMessages()
{
    MSG message;
    while (::PeekMessageA(&message, p_->handle, 0, 0, PM_REMOVE) > 0)
    {
        ::TranslateMessage(&message);
        ::DispatchMessageA(&message);
        if (message.message == WM_QUIT)
        {
            p_->shouldClose = true;
        }
    }
}

bool Core::Window::ShouldClose() const
{
    return p_->shouldClose;
}

bool Core::Window::IsMinimized() const
{
    return IsIconic(p_->handle);
}

void Core::Window::SetShouldClose()
{
    p_->shouldClose = true;
}

uint32_t Core::Window::GetWidth() const
{
    RECT rectangle;
    ::GetClientRect(p_->handle, &rectangle);
    return rectangle.right - rectangle.left;
}

uint32_t Core::Window::GetHeight() const
{
    RECT rectangle;
    ::GetClientRect(p_->handle, &rectangle);
    return rectangle.bottom - rectangle.top;
}

Core::Window::Window()
    : p_(new Window::Private)
{
    auto closeFnc = std::bind(&Window::Private::CloseEvent, p_, std::placeholders::_1, std::placeholders::_2);
    CoreEventSystem.SubscribeToEvent(Core::EventCode::WindowClosed, closeFnc, p_);
    auto quitFnc = std::bind(&Window::Private::QuitEvent, p_, std::placeholders::_1, std::placeholders::_2);
    CoreEventSystem.SubscribeToEvent(Core::EventCode::ApplicationQuit, quitFnc, p_);
}

Core::Window::~Window()
{
    delete p_;
}

void Core::Platform::OutputMessage(const char* message, uint8_t color)
{
    HANDLE consoleHandle = ::GetStdHandle(STD_OUTPUT_HANDLE);
    static uint8_t levels[6] = 
    { 
        BACKGROUND_RED | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY, // == Fatal
        FOREGROUND_RED | FOREGROUND_INTENSITY, // == Error
        FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY, // == Warn
        FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE, // == Info
        FOREGROUND_GREEN, // == Debug
        FOREGROUND_INTENSITY  // == Trace
    };
    ::SetConsoleTextAttribute(consoleHandle, levels[color]);
    ::OutputDebugStringA(message);
    uint64_t length = strlen(message);
    LPDWORD numberWritten = 0;
    ::WriteConsoleA(consoleHandle, message, (DWORD)length, numberWritten, 0);
}

void Core::Platform::Sleep(uint32_t ms)
{
    ::SleepEx(ms, FALSE);
}

Core::Window* Core::Platform::GetNewWindow(const char* name,
    uint32_t x, uint32_t y, uint32_t width, uint32_t height) const
{
    uint32_t windowStyle = WS_OVERLAPPED | WS_SYSMENU | WS_CAPTION;
    uint32_t windowExStyle = WS_EX_APPWINDOW;

    windowStyle |= WS_MAXIMIZEBOX;
    windowStyle |= WS_MINIMIZEBOX;
    windowStyle |= WS_THICKFRAME;

    RECT borderRectangle = { 0, 0, 0, 0 };
    ::AdjustWindowRectEx(&borderRectangle, windowStyle, 0, windowExStyle);

    uint32_t windowX = x + borderRectangle.left;
    uint32_t windowY = y + borderRectangle.top;
    uint32_t windowWidth = width + borderRectangle.right - borderRectangle.left;
    uint32_t windowHeight = height + borderRectangle.bottom - borderRectangle.top;

    HWND handle = ::CreateWindowExA(
        windowExStyle, windowClassName, name, windowStyle,
        windowX, windowY, windowWidth, windowHeight,
        0, 0, GetHInstance(), 0);

    Window* window = nullptr;

    if (handle == 0)
    {
        ::MessageBoxA(NULL, "Window creation failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        return nullptr;
    }
    else
    {
        // TODO: Make thread safe.
        for (int w = 0; w < MaxWindows; ++w)
        {
            if (p_->windows[w] == nullptr)
            {
                p_->windows[w] = new Window;
                window = p_->windows[w];
                window->p_->handle = handle;
                break;
            }
        }
    }

    if (window == nullptr)
    {
        ::MessageBoxA(NULL, "Maximum window count reached!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        return nullptr;
    }

    ::SetPropA(handle, "window ptr", window->p_);

    // TODO: If the window should not accept input, this should be false.
    bool shouldActivate = true;
    int cmdFlags = shouldActivate ? SW_SHOW : SW_SHOWNOACTIVATE;
    // If initially minimized, use SW_MINIMIZE : SW_SHOWMINNOACTIVE.
    // If initially maximized, use SW_SHOWMAXIMIZED : SW_MAXIMIZE.
    ::ShowWindow(handle, cmdFlags);

    return window;
}

void Core::Platform::DeleteWindow(Window* window) const
{
    HWND handle = window->p_->handle;
    ::DestroyWindow(handle);
    for (int w = 0; w < MaxWindows; ++w)
    {
        if (p_->windows[w] && (HWND)p_->windows[w]->GetHandle() == handle)
        {
            delete p_->windows[w];
            p_->windows[w] = nullptr;
            break;
        }
    }
}

static bool AllowCursorChange = true;
void Core::Platform::SetCursor(CursorType type) const
{
    if (!AllowCursorChange)
    {
        return;
    }

    if (type == CursorType::None)
    {
        ::SetCursor(NULL);
    }
    else
    {
        LPTSTR cursor = IDC_ARROW;
        
        switch (type)
        {
        case CursorType::Arrow:
        {
            cursor = IDC_ARROW;
            break;
        }
        case CursorType::TextInput:
        {
            cursor = IDC_IBEAM;
            break;
        }
        case CursorType::ResizeAll:
        {
            cursor = IDC_SIZEALL;
            break;
        }
        case CursorType::ResizeEW:
        {
            cursor = IDC_SIZEWE;
            break;
        }
        case CursorType::ResizeNS:
        {
            cursor = IDC_SIZENS;
            break;
        }
        case CursorType::ResizeNESW:
        {
            cursor = IDC_SIZENESW;
            break;
        }
        case CursorType::ResizeNWSE:
        {
            cursor = IDC_SIZENWSE;
            break;
        }
        case CursorType::Hand:
        {
            cursor = IDC_HAND;
            break;
        }
        case CursorType::NotAllowed:
        {
            cursor = IDC_NO;
            break;
        }
        }
        ::SetCursor(::LoadCursor(NULL, cursor));
    }
}

uint64_t Core::Platform::GetProgramID()
{
    return (uint64_t)GetHInstance();
}

const char* Core::Platform::GetVulkanSurfacePlatformExtension()
{
    static const char* extensionName = "VK_KHR_win32_surface";
    return extensionName;
}

void Core::Platform::Quit()
{
    CoreEventSystem.SignalEvent(Core::EventCode::ApplicationQuit, {});
}

LRESULT CALLBACK ProcessMessage(HWND hwnd, uint32_t msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_CREATE)
    {
        ::SetCursor(::LoadCursor(NULL, IDC_ARROW));
    }
    else if (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN || msg == WM_KEYUP || msg == WM_SYSKEYUP)
    {
        Core::EventCode code = (msg & 0x0001) ? Core::EventCode::KeyReleased : Core::EventCode::KeyPressed;
        Core::EventData eventData;
        eventData.data.u16[0] = (uint16_t)wParam;
        
        CoreEventSystem.SignalEvent(code, eventData);
        return TRUE;
    }
    else if (msg == WM_CHAR)
    {
        if (wParam > 0 && wParam < 0x10000)
        {
            Core::EventData eventData;
            eventData.data.u16[0] = (unsigned short)wParam;
            CoreEventSystem.SignalEvent(Core::EventCode::CharacterInput, eventData);
        }
        return TRUE;
    }
    else if (msg == WM_CLOSE)
    {
        Core::EventData eventData;
        eventData.data.u64[0] = (uint64_t)hwnd;
        CoreEventSystem.SignalEvent(Core::EventCode::WindowClosed, eventData);
        return TRUE;
    }
    else if (msg == WM_LBUTTONDOWN || msg == WM_LBUTTONDBLCLK || msg == WM_RBUTTONDOWN || msg == WM_RBUTTONDBLCLK)
    {
        uint16_t whichButton = msg & 0x0001 ? 0 /*left*/ : 1 /*right*/;
        Core::EventData eventData;
        eventData.data.u16[0] = whichButton;
        CoreEventSystem.SignalEvent(Core::EventCode::MouseButtonPressed, eventData);
        return TRUE;
    }
    else if (msg == WM_LBUTTONUP || msg == WM_RBUTTONUP)
    {
        uint16_t whichButton = msg & 0x0001 ? 1 /*left*/ : 0 /*right*/;
        Core::EventData eventData;
        eventData.data.u16[0] = whichButton;
        CoreEventSystem.SignalEvent(Core::EventCode::MouseButtonReleased, eventData);
        return TRUE;
    }
    else if (msg == WM_MBUTTONDOWN || msg == WM_MBUTTONUP || msg == WM_MBUTTONDBLCLK)
    {
        Core::EventCode code = msg & 0x0001 ? Core::EventCode::MouseButtonPressed : Core::EventCode::MouseButtonReleased;
        Core::EventData eventData;
        eventData.data.u16[0] = uint16_t(2);

        CoreEventSystem.SignalEvent(code, eventData);
        return TRUE;
    }
    else if (msg == WM_MOUSEWHEEL || msg == WM_MOUSEHWHEEL)
    {
        Core::EventData eventData;
        eventData.data.i8[0] = (int8_t)(GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA);
        eventData.data.u8[1] = (msg == WM_MOUSEHWHEEL) ? 1 : 0;

        CoreEventSystem.SignalEvent(Core::EventCode::MouseWheel, eventData);
        return TRUE;
    }
    else if (msg == WM_MOUSEMOVE)
    {
        POINT pos;
        ::GetCursorPos(&pos);
        ::ScreenToClient(hwnd, &pos);
        Core::EventData eventData;
        eventData.data.i16[0] = (int16_t)pos.x;
        eventData.data.i16[1] = (int16_t)pos.y;
        CoreEventSystem.SignalEvent(Core::EventCode::MouseMoved, eventData);
    }
    else if (msg == WM_SIZE)
    {
        Core::EventData eventData;
        RECT rectangle;
        ::GetClientRect(hwnd, &rectangle);
        eventData.data.u16[0] = (uint16_t)(rectangle.right - rectangle.left);
        eventData.data.u16[1] = (uint16_t)(rectangle.bottom - rectangle.top);
        CoreEventSystem.SignalEvent(Core::EventCode::WindowResized, eventData);
    }
    else if (msg == WM_SETCURSOR)
    {
        int pos = LOWORD(lParam);
        if (pos == HTRIGHT || pos == HTLEFT)
        {
            AllowCursorChange = false;
            SetCursor(LoadCursor(NULL, IDC_SIZEWE));
            return TRUE;
        }
        else if (pos == HTTOP || pos == HTBOTTOM)
        {
            AllowCursorChange = false;
            SetCursor(LoadCursor(NULL, IDC_SIZENS));
            return TRUE;
        }
        else if (pos == HTBOTTOMRIGHT || pos == HTTOPLEFT)
        {
            AllowCursorChange = false;
            SetCursor(LoadCursor(NULL, IDC_SIZENWSE));
            return TRUE;
        }
        else if (pos == HTBOTTOMLEFT || pos == HTTOPRIGHT)
        {
            AllowCursorChange = false;
            SetCursor(LoadCursor(NULL, IDC_SIZENESW));
            return TRUE;
        }
        else// (pos == HTCLIENT)
        {
            if (!AllowCursorChange)
            {
                AllowCursorChange = true;
                SetCursor(LoadCursor(NULL, IDC_ARROW));
            }
            return TRUE;
        }
    }

    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

struct Core::Filesystem::Private
{
    std::filesystem::path Root;
    std::filesystem::path ExecutableName;
};

Core::Filesystem::Filesystem()
    : p_(new Filesystem::Private)
{
    const unsigned long MAX_PATH_LENGTH = 256;
    char pathBuffer[MAX_PATH_LENGTH];
    int bytes = GetModuleFileNameA(NULL, pathBuffer, MAX_PATH_LENGTH);
    p_->Root = std::filesystem::path(pathBuffer);
    p_->ExecutableName = p_->Root.filename();
    p_->Root.remove_filename();
}

Core::Filesystem::~Filesystem()
{
    delete p_;
}

std::string Core::Filesystem::ExecutableName() const
{
    return p_->ExecutableName.replace_extension().string();
}

std::string Core::Filesystem::GetAbsolutePath(const std::string& relativePath) const
{
    std::filesystem::path relative(relativePath);
    std::filesystem::path absolute = p_->Root;
    absolute += relative;
    return absolute.lexically_normal().make_preferred().string();
}

bool Core::Filesystem::FileExists(const std::string& path) const
{
    std::filesystem::path fsPath(path);
    std::filesystem::path absolutePath = fsPath.is_relative() ? GetAbsolutePath(path) : fsPath;
    return std::filesystem::exists(absolutePath);
}

size_t Core::Filesystem::GetFileSize(const std::string& path) const
{
    FILE* f;
    fopen_s(&f, path.c_str(), "rb");

    if (!f)
    {
        CoreLogError("Failed to open file (%s).", path.c_str());
        return 0;
    }

    fseek(f, 0, SEEK_END);
    const size_t fileSize = ftell(f);
    fclose(f);
    return fileSize;
}

void Core::Filesystem::ReadFile(const std::string& path, void* data, size_t size) const
{
    FILE* f;
    fopen_s(&f, path.c_str(), "rb");

    if (!f)
    {
        CoreLogError("Failed to open file (%s).", path.c_str());
        return;
    }

    fread_s(data, size, 1, size, f);
    fclose(f);
}

std::string Core::Filesystem::Filename(const std::string& path) const
{
    std::filesystem::path fsPath(path);
    return fsPath.filename().string();
}

std::string Core::Filesystem::Extension(const std::string& path) const
{
    std::filesystem::path fsPath(path);
    return fsPath.extension().string();
}

std::string Core::Filesystem::RemoveExtension(const std::string& path) const
{
    std::filesystem::path fsPath(path);
    fsPath.replace_extension();
    return fsPath.string();
}

#endif
