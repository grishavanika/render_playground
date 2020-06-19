#pragma once
#include <Windows.h>
#include <functional>
#include <unordered_map>
#include "utils.h"

class StubWindow
{
public:
    explicit StubWindow(const char* class_name)
        : class_name_(class_name)
        , wnd_(create())
    {
    }

    ~StubWindow()
    {
        destroy();
    }

    StubWindow(const StubWindow& rhs) = delete;
    StubWindow& operator=(const StubWindow& rhs) = delete;

    StubWindow(StubWindow&& rhs) noexcept
        : class_name_(rhs.class_name_)
        , wnd_(rhs.wnd_)
    {
        rhs.class_name_ = nullptr;
        rhs.wnd_ = nullptr;
    }

    StubWindow& operator=(StubWindow&& rhs) noexcept
    {
        if (&rhs != this)
        {
            destroy();
            class_name_ = rhs.class_name_;
            wnd_ = rhs.wnd_;
            rhs.class_name_ = nullptr;
            rhs.wnd_ = nullptr;
        }
        return *this;
    }

    HWND wnd() const
    {
        return wnd_;
    }

    using MessageCallback = std::function<LRESULT
        (HWND /*hwnd*/, UINT /*msg*/, WPARAM /*wparam*/, LPARAM /*lparam*/)>;

    void on_message(UINT msg, MessageCallback handler)
    {
        Panic(!!handler);
        handlers_[msg] = std::move(handler);
    }

    void set_message_handler(MessageCallback handler)
    {
        wnd_handler_ = handler;
    }

private:
    LRESULT on_wnd_message(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
    {
        if (wnd_handler_
            && (wnd_handler_(hwnd, msg, wparam, lparam) == TRUE))
        {
            return TRUE;
        }

        const auto it = handlers_.find(msg);
        if (it != handlers_.end())
        {
            const MessageCallback& callback = it->second;
            return callback(hwnd, msg, wparam, lparam);
        }
        return ::DefWindowProc(hwnd, msg, wparam, lparam);
    }

    HWND create()
    {
        HINSTANCE app_handle = ::GetModuleHandle(nullptr);
        auto register_wnd_class = [&]
        {
            WNDCLASSEXA wcx{};
            wcx.cbSize = sizeof(wcx);
            wcx.style = CS_HREDRAW | CS_VREDRAW;
            wcx.lpfnWndProc = &WndProc;
            wcx.hInstance = app_handle;
            wcx.hIcon = ::LoadIcon(nullptr/*standard icon*/, IDI_APPLICATION);
            wcx.hCursor = ::LoadCursor(nullptr/*standard cursor*/, IDC_ARROW);
            wcx.hbrBackground = reinterpret_cast<HBRUSH>(::GetStockObject(WHITE_BRUSH));
            wcx.lpszClassName = class_name_;
            wcx.hIconSm = nullptr;

            return ::RegisterClassExA(&wcx);
        };

        auto create_window = [&]
        {
            HWND hwnd = ::CreateWindowA(
                class_name_,
                "",
                WS_OVERLAPPEDWINDOW,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                nullptr, // no parent wnd
                nullptr, // no menu
                app_handle,
                this); // to be passed to WndProc
            return hwnd;
        };

        if (register_wnd_class())
        {
            return create_window();
        }
        return nullptr;
    }

    void destroy()
    {
        if (!wnd_)
        {
            return;
        }

#if (0) // #TODO: investigate why this is not equal to `this`.
        StubWindow* self1 = reinterpret_cast<StubWindow*>(::GetWindowLongPtr(wnd_, GWLP_USERDATA));
        Panic(self1 == this);
#endif
#if (0)
        const StubWindow* self = reinterpret_cast<const StubWindow*>(
            ::SetWindowLongPtr(wnd_, GWLP_USERDATA, LONG_PTR(0)));
        Panic(self == this);

        BOOL ok = ::DestroyWindow(wnd_);
        Panic(ok);
#endif

        HINSTANCE app_handle = ::GetModuleHandle(nullptr);
        BOOL ok = ::UnregisterClassA(class_name_, app_handle);
        Panic(ok);

        wnd_ = nullptr;
        class_name_ = nullptr;
    }

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
    {
        if (msg == WM_NCCREATE)
        {
             const CREATESTRUCT* cs = reinterpret_cast<const CREATESTRUCT*>(lparam);
             Panic(cs && cs->lpCreateParams);
             StubWindow* self = static_cast<StubWindow*>(cs->lpCreateParams);

             // ::SetWindowLongPtr().
             // To determine success or failure, clear the last error information by
             // calling SetLastError with 0, then call SetWindowLongPtr.
             // Function failure will be indicated by a return value
             // of zero and a GetLastError result that is nonzero.
             ::SetLastError(0);
             if ((::SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self)) == 0)
                 && (::GetLastError() != 0))
             {
                 // Can't set the data. Fail ::CreateWindow() call.
                 return FALSE;
             }
             return TRUE;
        }

        StubWindow* self = reinterpret_cast<StubWindow*>(::GetWindowLongPtr(hwnd, GWLP_USERDATA));
        if (!self)
        {
            return ::DefWindowProc(hwnd, msg, wparam, lparam);
        }
        return self->on_wnd_message(hwnd, msg, wparam, lparam);
    }

private:
    const char* class_name_ = nullptr;
    MessageCallback wnd_handler_;
    std::unordered_map<UINT, MessageCallback> handlers_;
    HWND wnd_ = nullptr;
};
