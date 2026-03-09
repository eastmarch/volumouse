#define WIN32_LEAN_AND_MEAN
#include <stdio.h>
#include <wchar.h>
#include <windows.h>

/* ------------------------------------------------------------------ */
/*  Monitor cache                                                      */
/* ------------------------------------------------------------------ */

#define MAX_MONITORS 16

typedef struct {
    HMONITOR hMonitor;
    MONITORINFO info;
} CachedMonitor;

static CachedMonitor monitorCache[MAX_MONITORS];
static int monitorCount = 0;

/* Callback invoked once per monitor by EnumDisplayMonitors */
static BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData) {
    (void)hdcMonitor;
    (void)lprcMonitor;
    (void)dwData;

    if (monitorCount >= MAX_MONITORS) return FALSE; /* stop enumerating */

    CachedMonitor* entry = &monitorCache[monitorCount];
    entry->hMonitor = hMonitor;
    entry->info.cbSize = sizeof(MONITORINFO);

    if (GetMonitorInfo(hMonitor, &entry->info)) monitorCount++;

    return TRUE; /* continue enumerating */
}

/* Call once at startup (or whenever the display layout changes) */
static void CacheAllMonitors(void) {
    monitorCount = 0;
    EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, 0);
}

/* Look up cached info by HMONITOR handle.
   Returns a pointer into the cache, or NULL if not found. */
static const MONITORINFO* FindCachedMonitorInfo(HMONITOR hMonitor) {
    for (int i = 0; i < monitorCount; i++) {
        if (monitorCache[i].hMonitor == hMonitor) return &monitorCache[i].info;
    }
    return NULL;
}

/* ------------------------------------------------------------------ */
/*  Helper: print all cached monitors                                  */
/* ------------------------------------------------------------------ */

static void PrintCachedMonitors(void) {
    printf("=== Cached monitors: %d ===\n", monitorCount);
    for (int i = 0; i < monitorCount; i++) {
        const MONITORINFO* mi = &monitorCache[i].info;
        printf(
            "  [%d] hMonitor=0x%p  rcMonitor=(%ld,%ld)-(%ld,%ld)  "
            "rcWork=(%ld,%ld)-(%ld,%ld)%s\n",
            i, (void*)monitorCache[i].hMonitor, mi->rcMonitor.left, mi->rcMonitor.top, mi->rcMonitor.right,
            mi->rcMonitor.bottom, mi->rcWork.left, mi->rcWork.top, mi->rcWork.right, mi->rcWork.bottom,
            (mi->dwFlags & MONITORINFOF_PRIMARY) ? "  [PRIMARY]" : "");
    }
    printf("\n");
}

/* ------------------------------------------------------------------ */
/*  Example usage                                                      */
/* ------------------------------------------------------------------ */

int main(void) {
    /* 1. Cache all monitor info once at startup */
    CacheAllMonitors();
    PrintCachedMonitors();

    /* Loop that reads any key pressed */
    printf("Press enter key to get cursor position (press 'q' to quit):\n");
    while (1) {
        int ch = getchar();
        if (ch == 'q' || ch == 'Q') break;

        /* 2. Get the current cursor position */
        POINT cursorPos;
        if (!GetCursorPos(&cursorPos)) {
            printf("GetCursorPos failed\n");
            return 1;
        }

        /* 3. Find which monitor the cursor is on */
        HMONITOR hMon = MonitorFromPoint(cursorPos, MONITOR_DEFAULTTONEAREST);

        /* 4. Look up that monitor in the cache (no extra GetMonitorInfo call) */
        const MONITORINFO* mi = FindCachedMonitorInfo(hMon);
        if (!mi) {
            printf("Monitor not found in cache — display layout may have changed.\n");
            /* Optional: rebuild the cache and retry */
            CacheAllMonitors();
            mi = FindCachedMonitorInfo(hMon);
            if (!mi) {
                printf("Still not found after refresh. Aborting.\n");
                return 1;
            }
        }

        /* 5. Compute position relative to the monitor */
        int relX = cursorPos.x - mi->rcMonitor.left;
        int relY = cursorPos.y - mi->rcMonitor.top;

        printf("Absolute cursor pos:  (%ld, %ld)\n", cursorPos.x, cursorPos.y);
        printf("Monitor top-left:     (%ld, %ld)\n", mi->rcMonitor.left, mi->rcMonitor.top);
        printf("Relative to monitor:  (%d, %d)\n", relX, relY);
        printf("Monitor id:  (0x%p)\n", (void*)hMon);
    }

    return 0;
}
