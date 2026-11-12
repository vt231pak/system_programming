#pragma once
#include <windows.h>
#include <string>
#include <fstream>

// Конвертація байтів у читабельний вигляд (MB, GB)
std::string FormatSize(unsigned long long bytes);

// Логування подій у файл
void WriteLog(const std::string& filename, const std::string& message);

// Встановлення кольору консолі (для краси)
void SetColor(int color);