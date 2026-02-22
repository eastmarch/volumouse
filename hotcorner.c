#define WIN32_LEAN_AND_MEAN
#define CORNER_SIZE 20
#define KEYDOWN(k) ((k) & 0x8000)
#define MAX_MONITORS 16

#include <stdlib.h>
#include <windows.h>

#pragma comment(lib, "USER32")
#pragma comment(linker, "/SUBSYSTEM:WINDOWS")

// Monitor info cache
typedef struct {
    HMONITOR hMonitor;
    MONITORINFO info;
    RECT kTopLeftHotCorner;
    RECT kTopRightHotCorner;
} CachedMonitor;
static CachedMonitor monitorCache[MAX_MONITORS];
static int monitorCount = 0;

// Per-monitor DPI awareness context
static const HANDLE kDpiAwarenessContextPerMonitorAwareV2 = (HANDLE)-4;

// Inputs to inject when corner activated
static INPUT kVolumeUpInput[] = {
    {INPUT_KEYBOARD, .ki = {VK_VOLUME_UP, .dwFlags = 0}},
    {INPUT_KEYBOARD, .ki = {VK_VOLUME_UP, .dwFlags = KEYEVENTF_KEYUP}},
};

static INPUT kVolumeDownInput[] = {
    {INPUT_KEYBOARD, .ki = {VK_VOLUME_DOWN, .dwFlags = 0}},
    {INPUT_KEYBOARD, .ki = {VK_VOLUME_DOWN, .dwFlags = KEYEVENTF_KEYUP}},
};

static INPUT kDesktopLeftInput[] = {
    {INPUT_KEYBOARD, .ki = {VK_CONTROL, .dwFlags = 0}},
    {INPUT_KEYBOARD, .ki = {VK_LWIN, .dwFlags = 0}},
    {INPUT_KEYBOARD, .ki = {VK_LEFT, .dwFlags = 0}},
    {INPUT_KEYBOARD, .ki = {VK_LEFT, .dwFlags = KEYEVENTF_KEYUP}},
    {INPUT_KEYBOARD, .ki = {VK_LWIN, .dwFlags = KEYEVENTF_KEYUP}},
    {INPUT_KEYBOARD, .ki = {VK_CONTROL, .dwFlags = KEYEVENTF_KEYUP}},
};

static INPUT kDesktopRightInput[] = {
    {INPUT_KEYBOARD, .ki = {VK_CONTROL, .dwFlags = 0}},
    {INPUT_KEYBOARD, .ki = {VK_LWIN, .dwFlags = 0}},
    {INPUT_KEYBOARD, .ki = {VK_RIGHT, .dwFlags = 0}},
    {INPUT_KEYBOARD, .ki = {VK_RIGHT, .dwFlags = KEYEVENTF_KEYUP}},
    {INPUT_KEYBOARD, .ki = {VK_LWIN, .dwFlags = KEYEVENTF_KEYUP}},
    {INPUT_KEYBOARD, .ki = {VK_CONTROL, .dwFlags = KEYEVENTF_KEYUP}},
};

static INPUT kTaskViewInput[] = {
    {INPUT_KEYBOARD, .ki = {VK_LWIN, .dwFlags = 0}},
    {INPUT_KEYBOARD, .ki = {VK_TAB, .dwFlags = 0}},
    {INPUT_KEYBOARD, .ki = {VK_TAB, .dwFlags = KEYEVENTF_KEYUP}},
    {INPUT_KEYBOARD, .ki = {VK_LWIN, .dwFlags = KEYEVENTF_KEYUP}},
};

// Update corner coordinates with the hotkey CTRL+ALT+F12
// Quit application with ALT+SHIFT+F12
static const DWORD kHotKeyModUpdate = MOD_CONTROL | MOD_ALT;
static const DWORD kHotKeyModQuit = MOD_ALT | MOD_SHIFT;
static const DWORD kHotKey = VK_F12;

// Log a message to Windows Event Log
static void LogEvent(WORD eventType, const char* message) {
#ifdef EVENT_DEBUG
#pragma message("EVENT_DEBUG is enabled")
    HANDLE hEventLog = RegisterEventSourceA(NULL, "Volumouse");
    if (hEventLog) {
        ReportEventA(hEventLog, eventType, 0, 0, NULL, 1, 0, &message, NULL);
        DeregisterEventSource(hEventLog);
    }
#else
    (void)eventType;
    (void)message;
#endif
}

// Callback invoked once per monitor
static BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData) {
    (void)hdcMonitor;
    (void)lprcMonitor;
    (void)dwData;

    // Stop enumerating if we exceed the cache size
    if (monitorCount >= MAX_MONITORS) return FALSE;

    CachedMonitor* entry = &monitorCache[monitorCount];
    entry->hMonitor = hMonitor;
    entry->info.cbSize = sizeof(MONITORINFO);

    // Calculate hot corners for this monitor
    if (GetMonitorInfo(hMonitor, &entry->info)) {
        entry->kTopLeftHotCorner.left = entry->info.rcMonitor.left;
        entry->kTopLeftHotCorner.top = entry->info.rcMonitor.top;
        entry->kTopLeftHotCorner.right = entry->info.rcMonitor.left + CORNER_SIZE;
        entry->kTopLeftHotCorner.bottom = entry->info.rcMonitor.top + CORNER_SIZE;

        entry->kTopRightHotCorner.left = entry->info.rcMonitor.right - CORNER_SIZE;
        entry->kTopRightHotCorner.top = entry->info.rcMonitor.top;
        entry->kTopRightHotCorner.right = entry->info.rcMonitor.right;
        entry->kTopRightHotCorner.bottom = entry->info.rcMonitor.top + CORNER_SIZE;

        // Add to cache only if monitor data was successfully retrieved
        monitorCount++;
    }

    return TRUE;
}

// Cache monitor corner data
static void CacheAllMonitors() {
    monitorCount = 0;
    EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, 0);
    LogEvent(EVENTLOG_INFORMATION_TYPE, "Monitor cache updated");
}

static inline BOOL NoModifierKeysPressedDown() {
    return !(KEYDOWN(GetAsyncKeyState(VK_SHIFT)) || KEYDOWN(GetAsyncKeyState(VK_CONTROL)) ||
             KEYDOWN(GetAsyncKeyState(VK_LWIN)) || KEYDOWN(GetAsyncKeyState(VK_LBUTTON)) ||
             KEYDOWN(GetAsyncKeyState(VK_RBUTTON)));
}

// Find cached monitor index for a given point
static inline int FindMonitorIndexFromPoint(POINT pt) {
    HMONITOR hm = MonitorFromPoint(pt, MONITOR_DEFAULTTONULL);
    if (!hm) return -1;
    for (int i = 0; i < monitorCount; ++i) {
        if (monitorCache[i].hMonitor == hm) return i;
    }
    return -1;
}

// Check if point is in the top-left hot corner of the current monitor
static inline BOOL IsInTopLeftHotCorner(POINT pt) {
    int idx = FindMonitorIndexFromPoint(pt);
    if (idx < 0) return FALSE;
    return PtInRect(&monitorCache[idx].kTopLeftHotCorner, pt);
}

// Check if point is in the top-right hot corner of the current monitor
static inline BOOL IsInTopRightHotCorner(POINT pt) {
    int idx = FindMonitorIndexFromPoint(pt);
    if (idx < 0) return FALSE;
    return PtInRect(&monitorCache[idx].kTopRightHotCorner, pt);
}

static LRESULT HandleMouseWheelEvent(int nCode, WPARAM wParam, LPARAM lParam) {
    MSLLHOOKSTRUCT* evt = (MSLLHOOKSTRUCT*)lParam;
    short wheelDelta = HIWORD(evt->mouseData);

    if (IsInTopLeftHotCorner(evt->pt) && NoModifierKeysPressedDown()) {
        if (wheelDelta > 0) {
            SendInput(_countof(kVolumeUpInput), kVolumeUpInput, sizeof(INPUT));
        } else {
            SendInput(_countof(kVolumeDownInput), kVolumeDownInput, sizeof(INPUT));
        }
        // Prevents the event from being handled by the application underneath
        return 1;
    }
    if (IsInTopRightHotCorner(evt->pt) && NoModifierKeysPressedDown()) {
        if (wheelDelta > 0) {
            SendInput(_countof(kDesktopLeftInput), kDesktopLeftInput, sizeof(INPUT));
        } else {
            SendInput(_countof(kDesktopRightInput), kDesktopRightInput, sizeof(INPUT));
        }
        return 1;
    }

    // Pass the event to be handled by the next application in the chain
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

static LRESULT HandleMiddleButtonUpEvent(int nCode, WPARAM wParam, LPARAM lParam) {
    MSLLHOOKSTRUCT* evt = (MSLLHOOKSTRUCT*)lParam;

    if (IsInTopLeftHotCorner(evt->pt) && NoModifierKeysPressedDown()) {
        SendInput(_countof(kTaskViewInput), kTaskViewInput, sizeof(INPUT));
        return 1;
    }

    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

// Mouse event handler
static LRESULT CALLBACK MouseHookCallback(int nCode, WPARAM wParam, LPARAM lParam) {
    if (wParam == WM_MBUTTONDOWN) {
        return HandleMiddleButtonUpEvent(nCode, wParam, lParam);
    }
    if (wParam == WM_MOUSEWHEEL) {
        return HandleMouseWheelEvent(nCode, wParam, lParam);
    }
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

// Message handler for hidden window
static LRESULT CALLBACK MessageWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (uMsg == WM_DISPLAYCHANGE) {
        LogEvent(EVENTLOG_INFORMATION_TYPE, "Monitor, resolution or DPI change detected");
        CacheAllMonitors();
        return 0;
    }
    return DefWindowProcA(hwnd, uMsg, wParam, lParam);
}

// Enable per-monitor DPI awareness if available
static void EnableDPIAwareness() {
    // Try to load user32.dll for SetProcessDpiAwarenessContext (Windows 10 1703+)
    // MinGW missing headers for static linking
    HMODULE user32 = GetModuleHandleA("user32.dll");
    if (user32) {
        typedef BOOL(WINAPI * SetProcessDpiAwarenessContextFunc)(HANDLE);
        SetProcessDpiAwarenessContextFunc pSetProcessDpiAwarenessContext =
            (SetProcessDpiAwarenessContextFunc)GetProcAddress(user32, "SetProcessDpiAwarenessContext");
        if (pSetProcessDpiAwarenessContext) {
            pSetProcessDpiAwarenessContext(kDpiAwarenessContextPerMonitorAwareV2);
            return;
        }
    }

    // Fallback to older API (Windows Vista)
    // All monitors will be treated as the same DPI
    LogEvent(EVENTLOG_WARNING_TYPE,
             "Using fallback DPI awareness mode (SetProcessDPIAware). "
             "This application requires Windows 10 1703+ or later for full DPI awareness support. "
             "Some corners might not be detected correctly when using multiple monitors with different DPI scales.");
    SetProcessDPIAware();
}

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    MSG Msg;
    HHOOK MouseHook;
    WNDCLASSA wc = {0};
    HWND hwnd;

    SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
    EnableDPIAwareness();
    CacheAllMonitors();

    // Register and create hidden window for monitor/resolution change detection
    wc.lpfnWndProc = MessageWindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "HotCornerMsgWnd";
    RegisterClassA(&wc);
    hwnd = CreateWindowExA(0, "HotCornerMsgWnd", "Volumouse", 0, 0, 0, 0, 0, NULL, NULL, hInstance, NULL);
    if (!hwnd) {
        LogEvent(EVENTLOG_ERROR_TYPE, "Failed to create message window");
        return 1;
    }

    // Detect mouse events globally with a low-level hook
    if (!(MouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseHookCallback, NULL, 0))) {
        LogEvent(EVENTLOG_ERROR_TYPE, "Failed to install mouse hook");
        DestroyWindow(hwnd);
        return 1;
    }

    // Message loop to keep the application running and process hotkeys
    RegisterHotKey(NULL, 1, kHotKeyModQuit, kHotKey);
    RegisterHotKey(NULL, 2, kHotKeyModUpdate, kHotKey);
    while (GetMessage(&Msg, NULL, 0, 0)) {
        if (Msg.message == WM_HOTKEY) {
            if (LOWORD(Msg.lParam) == kHotKeyModQuit) break;
            if (LOWORD(Msg.lParam) == kHotKeyModUpdate) CacheAllMonitors();
        }
        DispatchMessage(&Msg);
    }

    UnhookWindowsHookEx(MouseHook);
    DestroyWindow(hwnd);
    return Msg.wParam;
}
