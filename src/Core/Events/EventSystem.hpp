#pragma once
#include "Core/Common.hpp"
#include "Core/Singleton.hpp"
#include <map>
#include <functional>

namespace Core
{
    struct EventData
    {
        // 128 bits
        union
        {
            int64_t i64[2];
            uint64_t u64[2];
            double f64[2];

            int32_t i32[4];
            uint32_t u32[4];
            float f32[4];

            int16_t i16[8];
            uint16_t u16[8];

            int8_t i8[16];
            uint8_t u8[16];

            char c[16];
        } data;
    };

    enum class EventCode;

    // TODO: See if we don't need a void* sender as well.
    typedef std::function<bool(EventCode, EventData)> OnEventFunc;

    struct EventSystem
    {
    public:
        EventSystem() = default;
        ~EventSystem() = default;

        void SubscribeToEvent(EventCode code, OnEventFunc fnc, void* const listener);
        void UnsubscribeFromEvent(EventCode code, void* const listener);
        bool SignalEvent(EventCode code, EventData context);
    private:
        std::map<EventCode, std::map<void* const, OnEventFunc>> registeredEvents_;
    };

    enum class EventCode
    {
        // Shuts the application down on the next frame.
        ApplicationQuit = 0x01,
        
        // Keyboard key pressed.
        /* Context usage:
        * u16 keyCode = context.data.u16[0];
        */
        KeyPressed = 0x02,

        // Keyboard key released.
        /* Context usage:
        * u16 keyCode = context.data.u16[0];
        */
        KeyReleased = 0x03,

        // Mouse button pressed.
        /* Context usage:
        * u16 buttonCode = context.data.u16[0];
        */
        MouseButtonPressed = 0x04,

        // Mouse button released.
        /* Context usage:
        * u16 buttonCode = context.data.u8[5];
        */
        MouseButtonReleased = 0x05,

        // Mouse moved.
        /* Context usage:
        * u16 x = context.data.u16[0];
        * u16 y = context.data.u16[1];
        */
        MouseMoved = 0x06,

        // Mouse moved.
        /* Context usage:
        * i8 delta = context.data.i8[0];
        * i8 horizontal = context.data.u8[1];
        */
        MouseWheel = 0x07,

        // Resized/resolution changed from the OS.
        /* Context usage:
        * u16 width = context.data.u16[0];
        * u16 height = context.data.u16[1];
        */
        WindowResized = 0x08,

        // Closed the window.
        /* Context usage:
        * u64 hwnd = context.data.u64[0]
        */
        WindowClosed = 0x09,

        // A debug message caught by Vulkan validation layers.
        /* Context usage:
        * u64 callbackData = context.data.u64[0]
        * u32 severityFlags = context.data.u32[2]
        */
        VulkanValidation = 0x0A,

        // A character being input.
        /* Context usage:
        * u16 UTF16Character = context.data.u16[0]
        */
        CharacterInput = 0x0B,

        MaxCode = 0xFF
    };
}

#define CoreEventSystem (::Core::Singleton<::Core::EventSystem>::GetInstance())
