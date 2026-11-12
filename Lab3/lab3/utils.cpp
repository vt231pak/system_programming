#include "utils.h"
#include <iomanip>
#include <sstream>
#include <ctime>
#include <iostream>

std::string FormatSize(unsigned long long bytes) {
    const char* suffixes[] = { "B", "KB", "MB", "GB", "TB" };
    int i = 0;
    double dblBytes = static_cast<double>(bytes);

    while (dblBytes >= 1024 && i < 4) {
        dblBytes /= 1024.0;
        i++;
    }

    std::ostringstream out;
    out << std::fixed << std::setprecision(2) << dblBytes << " " << suffixes[i];
    return out.str();
}

void WriteLog(const std::string& filename, const std::string& message) {
    std::ofstream logFile(filename, std::ios::app);
    if (logFile.is_open()) {
        time_t now = time(nullptr);
        char timeStr[26];
        ctime_s(timeStr, sizeof(timeStr), &now);
        timeStr[strlen(timeStr) - 1] = '\0';

        logFile << "[" << timeStr << "] " << message << std::endl;
    }
}

void SetColor(int color) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
}