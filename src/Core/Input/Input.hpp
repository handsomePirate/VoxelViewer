#pragma once
#include "Core/Common.hpp"
#include "Core/Singleton.hpp"
#include "Core/Events/EventSystem.hpp"

namespace Core
{
	class Input
	{
	public:
		Input();
		~Input();

        enum class Keys;
        enum class MouseButtons;

        bool IsKeyPressed(Keys keyCode) const;
        bool IsMouseButtonPressed(MouseButtons buttonCode) const;
        uint16_t GetMouseX() const;
        uint16_t GetMouseY() const;

        enum class MouseButtons
        {
            Left = 0,
            Right = 1,
            Middle = 2,
            ButtonCount
        };
        
        enum class Keys
        {
            Backspace = 0x08,
            Enter = 0x0D,
            Tab = 0x09,
            Shift = 0x10,
            Control = 0x11,
            Alt = 0x12,
            
            Pause = 0x13,
            Capital = 0x14,
            
            Escape = 0x1B,
            
            Convert = 0x1C,
            Nonconvert = 0x1D,
            Accept = 0x1E,
            ModeChange = 0x1F,
            
            Space = 0x20,
            Prior = 0x21,
            Next = 0x22,
            End = 0x23,
            Home = 0x24,
            Left = 0x25,
            Up = 0x26,
            Right = 0x27,
            Down = 0x28,
            Select = 0x29,
            Print = 0x2A,
            Execute = 0x2B,
            Snapshot = 0x2C,
            Insert = 0x2D,
            Delete = 0x2E,
            Help = 0x2F,
            
            A = 0x41,
            B = 0x42,
            C = 0x43,
            D = 0x44,
            E = 0x45,
            F = 0x46,
            G = 0x47,
            H = 0x48,
            I = 0x49,
            J = 0x4A,
            K = 0x4B,
            L = 0x4C,
            M = 0x4D,
            N = 0x4E,
            O = 0x4F,
            P = 0x50,
            Q = 0x51,
            R = 0x52,
            S = 0x53,
            T = 0x54,
            U = 0x55,
            V = 0x56,
            W = 0x57,
            X = 0x58,
            Y = 0x59,
            Z = 0x5A,
            
            LWin = 0x5B,
            RWin = 0x5C,
            Apps = 0x5D,
            
            Sleep  = 0x5F,
            
            Num0 = 0x60,
            Num1 = 0x61,
            Num2 = 0x62,
            Num3 = 0x63,
            Num4 = 0x64,
            Num5 = 0x65,
            Num6 = 0x66,
            Num7 = 0x67,
            Num8 = 0x68,
            Num9 = 0x69,
            Multiply = 0x6A,
            Add = 0x6B,
            Separator = 0x6C,
            Subtract = 0x6D,
            Decimal = 0x6E,
            Divide = 0x6F,
            F1 = 0x70,
            F2 = 0x71,
            F3 = 0x72,
            F4 = 0x73,
            F5 = 0x74,
            F6 = 0x75,
            F7 = 0x76,
            F8 = 0x77,
            F9 = 0x78,
            F10 = 0x79,
            F11 = 0x7A,
            F12 = 0x7B,
            F13 = 0x7C,
            F14 = 0x7D,
            F15 = 0x7E,
            F16 = 0x7F,
            F17 = 0x80,
            F18 = 0x81,
            F19 = 0x82,
            F20 = 0x83,
            F21 = 0x84,
            F22 = 0x85,
            F23 = 0x86,
            F24 = 0x87,
            
            NumLock = 0x90,
            Scroll = 0x91,
            
            NumEqual = 0x92,
            
            LShift = 0xA0,
            RShift = 0xA1,
            LControl = 0xA2,
            RControl = 0xA3,
            LMenu = 0xA4,
            RMenu = 0xA5,
            
            Semicolon = 0xBA,
            Plus = 0xBB,
            Comma = 0xBC,
            Minus = 0xBD,
            Period = 0xBE,
            Slash = 0xBF,
            Grave = 0xC0,
            
            KeyCount
        };
    private:
        bool KeySetPressed(Core::EventCode code, Core::EventData context);
        bool KeySetReleased(Core::EventCode code, Core::EventData context);
        
        bool MouseButtonSetPressed(Core::EventCode code, Core::EventData context);
        bool MouseButtonSetReleased(Core::EventCode code, Core::EventData context);

        bool SetMousePosition(Core::EventCode code, Core::EventData context);

        bool keysPressed_[256];
        bool mouseButtonsPressed_[8];

        uint16_t mouseX_;
        uint16_t mouseY_;
	};
}

#define CoreInput (::Core::Singleton<::Core::Input>::GetInstance())
