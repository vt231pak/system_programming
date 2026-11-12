#include "MemoryInfo.h"
#include "utils.h"
#include <windows.h>
#include <iostream>

void PrintMemoryInfo() {
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(memInfo);
    GlobalMemoryStatusEx(&memInfo);

    std::cout << "\n[ Системна пам'ять ]\n";
    std::cout << "Завантаження RAM:     " << memInfo.dwMemoryLoad << "%\n";
    std::cout << "Всього RAM:           " << FormatSize(memInfo.ullTotalPhys) << "\n";
    std::cout << "Вільно RAM:           " << FormatSize(memInfo.ullAvailPhys) << "\n";
    std::cout << "Всього PageFile:      " << FormatSize(memInfo.ullTotalPageFile) << "\n";
    std::cout << "Віртуальна пам'ять:   " << FormatSize(memInfo.ullTotalVirtual) << "\n";
}