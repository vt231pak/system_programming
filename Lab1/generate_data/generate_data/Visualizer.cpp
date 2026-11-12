#include "Shared.h"
#include <string>

// Глобальні змінні
SharedData* g_pData = nullptr;
SharedData g_localBuffer;
HANDLE g_hMap = NULL;
HANDLE g_hFile = INVALID_HANDLE_VALUE;
HANDLE g_hMutex = NULL;
bool g_isConnected = false;

// Функція спроби підключення (викликатиметься таймером)
void TryConnect(const wchar_t* fileName) {
    if (g_isConnected) return; // Вже працюємо

    // 1. Пробуємо відкрити файл
    if (g_hFile == INVALID_HANDLE_VALUE) {
        g_hFile = CreateFile(fileName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        // Якщо не вийшло RW, пробуємо ReadOnly (на випадок якщо генератор ще пише)
        if (g_hFile == INVALID_HANDLE_VALUE) {
            g_hFile = CreateFile(fileName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        }
    }

    if (g_hFile == INVALID_HANDLE_VALUE) return; // Файлу все ще немає

    // 2. Пробуємо MMF
    if (!g_hMap) {
        g_hMap = CreateFileMapping(g_hFile, GetNullSecurityAttributes(), PAGE_READWRITE, 0, 0, MMF_NAME);
        if (!g_hMap) g_hMap = OpenFileMapping(FILE_MAP_READ, FALSE, MMF_NAME);
    }

    if (!g_hMap) return; // MMF помилка

    // 3. Мапимо
    if (!g_pData) {
        g_pData = (SharedData*)MapViewOfFile(g_hMap, FILE_MAP_READ, 0, 0, sizeof(SharedData));
    }

    // 4. М'ютекс
    if (!g_hMutex) {
        g_hMutex = CreateMutex(GetNullSecurityAttributes(), FALSE, MUTEX_NAME);
    }

    if (g_pData && g_hMutex) {
        g_isConnected = true; // Успіх
    }
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    static const wchar_t* fileName = DEFAULT_FILE_NAME;

    switch (uMsg) {
    case WM_CREATE:
        SetTimer(hwnd, 1, 500, NULL); // Таймер перевірки
        return 0;

    case WM_TIMER:
        // Якщо ще не підключені — пробуємо підключитися
        if (!g_isConnected) {
            TryConnect(fileName);
            InvalidateRect(hwnd, NULL, TRUE); // Перемалювати (щоб оновити статус текст)
        }
        else {
            // Якщо підключені — читаємо дані
            if (WaitForSingleObject(g_hMutex, 50) == WAIT_OBJECT_0) {
                memcpy(&g_localBuffer, g_pData, sizeof(SharedData));
                ReleaseMutex(g_hMutex);
                InvalidateRect(hwnd, NULL, TRUE);
            }
            else {
                // Якщо раптом м'ютекс помер або файл закрили
                g_isConnected = false;
            }
        }
        return 0;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        if (!g_isConnected) {
            // Виводимо повідомлення, якщо немає з'єднання
            const wchar_t* msg = L"Waiting for Generator (data.dat)...";
            TextOut(hdc, 10, 10, msg, wcslen(msg));
        }
        else {
            // Малюємо графік
            int y = 10;
            for (int i = 0; i < DATA_SIZE; ++i) {
                int val = g_localBuffer.values[i];
                if (val < 0) val = 0;
                if (val > 200) val = 200;

                std::wstring text = std::to_wstring(val) + L" | ";
                for (int k = 0; k < val; ++k) text += L"*";

                TextOut(hdc, 10, y, text.c_str(), text.length());
                y += 18;
            }
        }
        EndPaint(hwnd, &ps);
    }
    return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow) {
    // Реєстрація класу вікна
    const wchar_t CLASS_NAME[] = L"SmartVisClass";
    WNDCLASS wc = { };
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(0, CLASS_NAME, L"Smart Visualizer", WS_OVERLAPPEDWINDOW | WS_VSCROLL,
        CW_USEDEFAULT, CW_USEDEFAULT, 500, 700, NULL, NULL, hInstance, NULL);

    if (hwnd == NULL) return 0;
    ShowWindow(hwnd, nCmdShow);

    MSG msg = { };
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Очищення
    if (g_pData) UnmapViewOfFile(g_pData);
    if (g_hMap) CloseHandle(g_hMap);
    if (g_hFile != INVALID_HANDLE_VALUE) CloseHandle(g_hFile);
    if (g_hMutex) CloseHandle(g_hMutex);

    return 0;
}