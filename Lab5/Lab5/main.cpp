#include "../Interface/ICompute.h"
#include <iostream>
#include <windows.h>
#include <vector>
#include <string>
#include <chrono>
#include <psapi.h>

size_t GetMemoryUsage() {
    PROCESS_MEMORY_COUNTERS_EX pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
        return pmc.WorkingSetSize;
    }
    return 0;
}

int main() {
    SetConsoleOutputCP(1251);

    std::cout << "=== Лабораторна робота: DLL та потоки ===\n";

    std::cout << "[Main] Генерація тестових даних (10 млн елементів)...\n";
    std::vector<int> testData(10000000, 2);
    std::cout << "[System] Використання RAM: " << GetMemoryUsage() / 1024 / 1024 << " MB\n\n";

    while (true) {
        std::cout << "\nМеню:\n";
        std::cout << "1. Завантажити Compute.dll і виконати обчислення\n";
        std::cout << "2. Перевірити статус DLL (GetModuleHandle)\n";
        std::cout << "0. Вихід\n";
        std::cout << "> ";

        int choice;
        std::cin >> choice;

        if (choice == 0) break;

        if (choice == 2) {
            HMODULE hCheck = GetModuleHandleW(L"Compute.dll");
            if (hCheck) std::cout << "-> DLL зараз завантажена у пам'ять.\n";
            else std::cout << "-> DLL НЕ завантажена.\n";
            continue;
        }

        if (choice == 1) {
            if (GetModuleHandleW(L"Compute.dll") != NULL) {
                std::cout << "[Warning] DLL вже завантажена! Спочатку вивантажте її (логіка тесту).\n";
            }

            std::cout << "[Loader] Завантаження Compute.dll...\n";
            HMODULE hCompute = LoadLibraryW(L"Compute.dll");

            if (!hCompute) {
                std::cerr << "[Error] Не вдалося завантажити Compute.dll. Код помилки: " << GetLastError() << "\n";
                continue;
            }

            auto createFunc = (CreateCompute_t)GetProcAddress(hCompute, "CreateCompute");
            if (!createFunc) {
                std::cerr << "[Error] Функцію CreateCompute не знайдено.\n";
                FreeLibrary(hCompute);
                continue;
            }

            ICompute* calculator = createFunc();
            std::cout << "[Object] Створено об'єкт версії: " << calculator->GetVersion() << "\n";

            std::cout << "[Object] Передача даних...\n";
            calculator->SetData(testData);
            std::cout << "[System] RAM після передачі даних: " << GetMemoryUsage() / 1024 / 1024 << " MB\n";

            int threadCounts[] = { 1, 2, 4, 8, 16 };
            std::cout << "\n--- Запуск тестів продуктивності ---\n";

            for (int t : threadCounts) {
                auto start = std::chrono::high_resolution_clock::now();

                calculator->Run(t);

                auto end = std::chrono::high_resolution_clock::now();
                std::chrono::duration<double> diff = end - start;

                std::cout << "Потоків: " << t
                    << " | Час: " << diff.count() << " с"
                    << " | Результат: " << calculator->GetResult() << "\n";
            }

            std::cout << "\n[Cleanup] Видалення об'єкта та вивантаження DLL...\n";
            calculator->Release();
            FreeLibrary(hCompute);

            std::cout << "[System] RAM після очищення: " << GetMemoryUsage() / 1024 / 1024 << " MB\n";
        }
    }

    return 0;
}