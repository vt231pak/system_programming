#include "Shared.h"
#include <conio.h>

int wmain(int argc, wchar_t* argv[]) {
    const wchar_t* fileName = (argc > 1) ? argv[1] : DEFAULT_FILE_NAME;
    std::wcout << L"Sorter (Bubble Ascending). File: " << fileName << std::endl;
    std::cout << "Press SPACE to start sorting..." << std::endl;

    while (_getch() != ' ');

    // 1. Відкриття файлу
    HANDLE hFile = CreateFile(fileName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        std::cerr << "File not found! Run Generator first." << std::endl;
        return 1;
    }

    // 2. Створення MMF
    HANDLE hMap = CreateFileMapping(hFile, GetNullSecurityAttributes(), PAGE_READWRITE, 0, 0, MMF_NAME);
    if (!hMap) {
        std::cerr << "CreateFileMapping failed: " << GetLastError() << std::endl;
        CloseHandle(hFile);
        return 1;
    }

    // 3. Відображення (Map)
    SharedData* pData = (SharedData*)MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SharedData));
    if (!pData) {
        std::cerr << "MapViewOfFile failed." << std::endl;
        CloseHandle(hMap);
        CloseHandle(hFile);
        return 1;
    }

    // 4. Створення Mutex
    HANDLE hMutex = CreateMutex(GetNullSecurityAttributes(), FALSE, MUTEX_NAME);
    if (!hMutex) {
        std::cerr << "CreateMutex failed." << std::endl;
        return 1;
    }

    // 5. Сортування (Bubble Sort)
    bool sorting = true;
    while (sorting) {
        sorting = false; // Використовуємо модифікований bubble sort, щоб не ганяти зайві цикли

        // Зовнішній цикл для пауз
        for (int i = 0; i < DATA_SIZE - 1; ++i) {

            // --- КРИТИЧНА СЕКЦІЯ ---
            DWORD waitResult = WaitForSingleObject(hMutex, INFINITE);
            if (waitResult == WAIT_OBJECT_0) {
                // Один прохід бульбашки
                for (int j = 0; j < DATA_SIZE - 1 - i; ++j) {
                    if (pData->values[j] > pData->values[j + 1]) {
                        int temp = pData->values[j];
                        pData->values[j] = pData->values[j + 1];
                        pData->values[j + 1] = temp;
                        sorting = true;
                    }
                }
                FlushViewOfFile(pData, sizeof(SharedData)); // Збереження змін
                ReleaseMutex(hMutex);
                std::cout << "Iteration " << i << " done. Data flushed." << std::endl;
            }
            else if (waitResult == WAIT_ABANDONED) {
                // Інший процес впав, не звільнивши м'ютекс
                ReleaseMutex(hMutex);
            }

            Sleep(1000);
        }
        if (!sorting) break; // Якщо обмінів не було - готово
    }

    std::cout << "Sorting Complete!" << std::endl;

    UnmapViewOfFile(pData);
    CloseHandle(hMap);
    CloseHandle(hFile);
    CloseHandle(hMutex);
    system("pause");
    return 0;
}