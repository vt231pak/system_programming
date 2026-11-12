#include "Watcher.h"
#include "utils.h"
#include <windows.h>
#include <iostream>
#include <conio.h>

void PrintAction(DWORD action) {
    switch (action) {
    case FILE_ACTION_ADDED:
        std::cout << "[CREATE]\n";
        break;
    case FILE_ACTION_REMOVED:
        std::cout << "[DELETE]\n";
        break;
    case FILE_ACTION_MODIFIED:
        std::cout << "[MODIFY]\n";
        break;
    case FILE_ACTION_RENAMED_OLD_NAME:
        std::cout << "[RENAME-OLD]\n";
        break;
    case FILE_ACTION_RENAMED_NEW_NAME:
        std::cout << "[RENAME-NEW]\n";
        break;
    default:
        std::cout << "[UNKNOWN]\n";
    }
}


void StartDirectoryWatcher(const char* path) {
    std::cout << "\n[ МОНІТОРИНГ ]\n";
    std::cout << "Відстеження каталогу: " << path << "\n";
    std::cout << "Натисніть ESC для виходу.\n";

    HANDLE hDir = CreateFileA(
        path,
        FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS,
        NULL
    );

    if (hDir == INVALID_HANDLE_VALUE) {
        std::cout << "ПОМИЛКА: Не вдалося відкрити каталог (" << GetLastError() << ")\n";
        return;
    }

    BYTE buffer[4096];
    DWORD bytesReturned;

    WriteLog("monitor.log", "--- START MONITORING ---");

    while (true) {
        if (ReadDirectoryChangesW(
            hDir,
            buffer,
            sizeof(buffer),
            TRUE,
            FILE_NOTIFY_CHANGE_FILE_NAME |
            FILE_NOTIFY_CHANGE_DIR_NAME |
            FILE_NOTIFY_CHANGE_LAST_WRITE |
            FILE_NOTIFY_CHANGE_SIZE,
            &bytesReturned,
            NULL,
            NULL
        )) {
            FILE_NOTIFY_INFORMATION* info =
                (FILE_NOTIFY_INFORMATION*)buffer;

            while (true) {
                std::wstring fileName(
                    info->FileName,
                    info->FileNameLength / sizeof(WCHAR)
                );

                PrintAction(info->Action);
                std::wcout << fileName << L"\n";

                WriteLog("monitor.log", "Action detected");

                if (info->NextEntryOffset == 0)
                    break;

                info = (FILE_NOTIFY_INFORMATION*)
                    ((BYTE*)info + info->NextEntryOffset);
            }
        }

        if (_kbhit()) {
            if (_getch() == 27)
                break;
        }
    }

    CloseHandle(hDir);
    WriteLog("monitor.log", "--- STOP MONITORING ---");
    std::cout << "Моніторинг завершено.\n";
}