#include "Shared.h"
#include <conio.h>

int wmain(int argc, wchar_t* argv[]) {
    const wchar_t* fileName = (argc > 1) ? argv[1] : DEFAULT_FILE_NAME;
    std::wcout << L"Sorter REVERSE (Selection Descending). File: " << fileName << std::endl;
    std::cout << "Press SPACE to start sorting..." << std::endl;

    while (_getch() != ' ');

    HANDLE hFile = CreateFile(fileName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return 1;

    HANDLE hMap = CreateFileMapping(hFile, GetNullSecurityAttributes(), PAGE_READWRITE, 0, 0, MMF_NAME);
    SharedData* pData = (SharedData*)MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SharedData));
    HANDLE hMutex = CreateMutex(GetNullSecurityAttributes(), FALSE, MUTEX_NAME);

    // Àëãîðèòì Selection Sort (DESCENDING)
    for (int i = 0; i < DATA_SIZE - 1; ++i) {

        // --- ÊÐÈÒÈ×ÍÀ ÑÅÊÖ²ß ---
        WaitForSingleObject(hMutex, INFINITE);

        int max_idx = i;
        for (int j = i + 1; j < DATA_SIZE; j++) {
            if (pData->values[j] > pData->values[max_idx]) {
                max_idx = j;
            }
        }

        if (max_idx != i) {
            int temp = pData->values[i];
            pData->values[i] = pData->values[max_idx];
            pData->values[max_idx] = temp;
            std::cout << "Swapped " << temp << " with " << pData->values[i] << std::endl;
        }

        FlushViewOfFile(pData, sizeof(SharedData));
        ReleaseMutex(hMutex);
        // --- Ê²ÍÅÖÜ ÑÅÊÖ²¯ ---

        Sleep(1000);
    }

    std::cout << "Reverse Sorting Complete!" << std::endl;

    UnmapViewOfFile(pData);
    CloseHandle(hMap);
    CloseHandle(hFile);
    CloseHandle(hMutex);
    system("pause");
    return 0;
}