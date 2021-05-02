#include "Platform.hpp"

#ifdef PLATFORM_WINDOWS
#include <Windows.h>

struct Platform::Private
{
    bool initialized = false;
};

inline HINSTANCE GetHInstance()
{
    return GetModuleHandleA(0);
}

LRESULT CALLBACK ProcessMessage(HWND hwnd, uint32_t msg, WPARAM w_param, LPARAM l_param);

static const char* windowClassName = "my window class";

Platform::Platform()
	: p_(new Platform::Private)
{
    HINSTANCE hInstance = GetHInstance();
    // Setup and register window class.
    HICON icon = LoadIcon(hInstance, IDI_APPLICATION);
    WNDCLASSA wc;
    memset(&wc, 0, sizeof(wc));
    wc.style = CS_DBLCLKS;  // Get double-clicks
    wc.lpfnWndProc = ProcessMessage;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = icon;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);  // NULL; // Manage the cursor manually
    wc.hbrBackground = NULL;                   // Transparent
    wc.lpszClassName = windowClassName;

    if (!RegisterClassA(&wc)) {
        MessageBoxA(0, "Window registration failed", "Error", MB_ICONEXCLAMATION | MB_OK);
        return;
    }

    p_->initialized = true;
}

Platform::~Platform()
{
    UnregisterClassA(windowClassName, GetHInstance());
}

LRESULT CALLBACK ProcessMessage(HWND hwnd, uint32_t msg, WPARAM w_param, LPARAM l_param)
{
    return DefWindowProcA(hwnd, msg, w_param, l_param);
}

#endif
