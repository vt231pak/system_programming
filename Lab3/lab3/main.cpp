#include <windows.h>
#include <iostream>
#include "DiskInfo.h"
#include "MemoryInfo.h"
#include "UserInfo.h"
#include "Watcher.h"
#include "utils.h"

int main() {
    // Налаштування кодування для коректного виводу кирилиці
    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);

    SetColor(10); // Green
    std::cout << "=== DiskInfo Utility ===\n\n";
    SetColor(7);  // White

    // 1. Інформація про диски (Завдання 1-4)
    PrintAllDrivesInfo();

    // 2. Інформація про пам'ять (Завдання 5)
    PrintMemoryInfo();

    // 3. Користувач та Система (Завдання 6, 7, 8 + Bonus E)
    PrintComputerAndUserInfo();
    PrintProcessorInfo(); // Bonus
    PrintSystemDirectories();

    std::cout << "\n------------------------------------------------\n";
    std::cout << "МЕНЮ ДІЙ:\n";
    std::cout << "1. Запустити моніторинг каталогу (Завдання 9)\n";
    std::cout << "2. Запустити тест швидкості диску (Bonus A)\n";
    std::cout << "3. Вихід\n";
    std::cout << "Ваш вибір: ";

    char choice;
    std::cin >> choice;

    if (choice == '1') {
        char path[MAX_PATH];
        std::cout << "Введіть шлях (наприклад C:\\Temp): ";
        std::cin.ignore();
        std::cin.getline(path, MAX_PATH);
        StartDirectoryWatcher(path);
    }
    else if (choice == '2') {
        char driveLetter[5];
        std::cout << "Введіть диск (наприклад C:\\): ";
        std::cin >> driveLetter;
        RunDiskBenchmark(driveLetter);
    }

    std::cout << "Робота завершена.\n";
    return 0;
}