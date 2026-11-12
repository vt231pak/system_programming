#include "DiskInfo.h"
#include "utils.h"
#include <iostream>
#include <vector>
#include <string>
#include <iomanip>

// Допоміжна: розшифровка типу диска
const char* GetDriveTypeString(UINT type) {
    switch (type) {
    case DRIVE_UNKNOWN: return "Невідомо";
    case DRIVE_NO_ROOT_DIR: return "Шлях недійсний";
    case DRIVE_REMOVABLE: return "Знімний (USB/SD)";
    case DRIVE_FIXED: return "Фіксований (HDD/SSD)";
    case DRIVE_REMOTE: return "Мережевий";
    case DRIVE_CDROM: return "CD-ROM";
    case DRIVE_RAMDISK: return "RAM-Disk";
    default: return "Інше";
    }
}

// Допоміжна: розшифровка прапорів ФС
void PrintFileSystemFlags(DWORD flags) {
    std::cout << "      [+] Можливості:\n";
    if (flags & FILE_CASE_SENSITIVE_SEARCH)    std::cout << "         Case-Sensitive\n";
    if (flags & FILE_CASE_PRESERVED_NAMES)     std::cout << "         Case-Preserved\n";
    if (flags & FILE_UNICODE_ON_DISK)          std::cout << "         Unicode\n";
    if (flags & FILE_PERSISTENT_ACLS)          std::cout << "         ACL\n";
    if (flags & FILE_FILE_COMPRESSION)         std::cout << "         Compression\n";
    if (flags & FILE_VOLUME_IS_COMPRESSED)     std::cout << "         Volume-Compressed\n";
    if (flags & FILE_SUPPORTS_ENCRYPTION)      std::cout << "         EFS\n";
    if (flags & FILE_SUPPORTS_OBJECT_IDS)      std::cout << "         ObjectID\n";
    if (flags & FILE_SUPPORTS_REPARSE_POINTS)  std::cout << "         ReparsePoints\n";
    if (flags & FILE_SUPPORTS_SPARSE_FILES)    std::cout << "         Sparse\n";
    if (flags & FILE_SUPPORTS_TRANSACTIONS)    std::cout << "         TxF\n";
    if (flags & FILE_NAMED_STREAMS)            std::cout << "         ADS\n";
    if (flags & FILE_READ_ONLY_VOLUME)         std::cout << "         Read-Only\n";
    std::cout << "\n";
}

void PrintAllDrivesInfo() {
    char buffer[256];
    // Отримуємо рядки типу "C:\<null>D:\<null>"
    GetLogicalDriveStringsA(sizeof(buffer), buffer);

    char* drive = buffer;
    while (*drive) {
        SetColor(11); // Cyan
        std::cout << "\n>>> ДИСК: " << drive << "\n";
        SetColor(7); // White

        // 1. Тип диска
        UINT type = GetDriveTypeA(drive);
        std::cout << "   Тип: " << GetDriveTypeString(type) << "\n";

        // Якщо диск доступний, читаємо деталі
        if (type != DRIVE_NO_ROOT_DIR) {
            char volName[MAX_PATH];
            char fsName[MAX_PATH];
            DWORD serial, maxCompLen, flags;

            // 2. Інформація про том
            if (GetVolumeInformationA(drive, volName, MAX_PATH, &serial, &maxCompLen, &flags, fsName, MAX_PATH)) {
                std::cout << "   Мітка: " << (volName[0] ? volName : "(Без мітки)") << "\n";
                std::cout << "   Файлова система: " << fsName << "\n";
                std::cout << "   Серійний номер: " << serial << "\n";
                PrintFileSystemFlags(flags);
            }

            // 3. Місце на диску
            ULARGE_INTEGER freeBytes, totalBytes, totalFree;
            if (GetDiskFreeSpaceExA(drive, &freeBytes, &totalBytes, &totalFree)) {
                double usage = 100.0 * (1.0 - (double)totalFree.QuadPart / (double)totalBytes.QuadPart);

                std::cout << "   Загальний об'єм: " << FormatSize(totalBytes.QuadPart) << "\n";
                std::cout << "   Вільно:          " << FormatSize(totalFree.QuadPart) << "\n";
                std::cout << "   Зайнято:         " << std::fixed << std::setprecision(1) << usage << "%\n";
            }
        }

        // Перехід до наступного рядка (наступної букви диску)
        drive += strlen(drive) + 1;
    }
}

// BONUS A: Простий бенчмарк
void RunDiskBenchmark(const char* drivePath) {
    std::string testFile = std::string(drivePath) + "benchmark_test.dat";
    const DWORD bufferSize = 1024 * 1024; // 1 MB buffer
    const int iterations = 50; // 50 MB test
    char* data = new char[bufferSize];
    memset(data, 'A', bufferSize);

    std::cout << "\n--- Benchmark (Test 50MB) для " << drivePath << " ---\n";

    // Тест запису
    HANDLE hFile = CreateFileA(testFile.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        std::cout << "Помилка доступу для тесту (потрібні права адміна або запису).\n";
        delete[] data;
        return;
    }

    DWORD written;
    DWORD start = GetTickCount();
    for (int i = 0; i < iterations; i++) {
        WriteFile(hFile, data, bufferSize, &written, NULL);
    }
    DWORD end = GetTickCount();
    CloseHandle(hFile);

    double timeSec = (end - start) / 1000.0;
    if (timeSec > 0)
        std::cout << "Швидкість запису: " << (iterations / timeSec) << " MB/s\n";

    // Тест читання
    hFile = CreateFileA(testFile.c_str(), GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    DWORD read;
    start = GetTickCount();
    for (int i = 0; i < iterations; i++) {
        ReadFile(hFile, data, bufferSize, &read, NULL);
    }
    end = GetTickCount();
    CloseHandle(hFile);

    timeSec = (end - start) / 1000.0;
    if (timeSec > 0)
        std::cout << "Швидкість читання: " << (iterations / timeSec) << " MB/s\n";

    // Прибирання
    DeleteFileA(testFile.c_str());
    delete[] data;
}