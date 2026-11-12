#include "UserInfo.h"
#include <windows.h>
#include <iostream>
#include <vector>
#include <sddl.h>

void PrintComputerAndUserInfo() {
	char buffer[256];
	DWORD size;

	std::cout << "\n[ Користувач та Комп'ютер ]\n";

	size = sizeof(buffer);
	if (GetComputerNameA(buffer, &size))
		std::cout << "  [+] Computer (NetBIOS)        : " << buffer << "\n";

	size = sizeof(buffer);
	if (GetComputerNameExA(ComputerNameDnsHostname, buffer, &size))
		std::cout << "  [+] DNS Hostname              : " << buffer << "\n";

	size = sizeof(buffer);
	if (GetComputerNameExA(ComputerNameDnsFullyQualified, buffer, &size))
		std::cout << "  [+] DNS Fully Qualified       : " << buffer << "\n";

	size = sizeof(buffer);
	if (GetComputerNameExA(ComputerNamePhysicalDnsHostname, buffer, &size))
		std::cout << "  [+] Physical DNS Hostname     : " << buffer << "\n";

	size = sizeof(buffer);
	if (GetUserNameA(buffer, &size))
		std::cout << "  [+] User Name                 : " << buffer << "\n";

	SID_NAME_USE sidType;
	char domain[256];
	DWORD domainSize = sizeof(domain);
	DWORD sidSize = 0;

	LookupAccountNameA(
		NULL,
		buffer,
		NULL,
		&sidSize,
		domain,
		&domainSize,
		&sidType
	);

	PSID sid = (PSID)malloc(sidSize);
	domainSize = sizeof(domain);

	if (LookupAccountNameA(
		NULL,
		buffer,
		sid,
		&sidSize,
		domain,
		&domainSize,
		&sidType
	)) {
		LPSTR sidStr;
		ConvertSidToStringSidA(sid, &sidStr);
		std::cout << "  [+] User SID                  : " << sidStr << "\n";
		LocalFree(sidStr);
	}

	BOOL isAdmin = FALSE;
	HANDLE token = NULL;

	if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) {
		TOKEN_ELEVATION elevation;
		DWORD retSize;

		if (GetTokenInformation(
			token,
			TokenElevation,
			&elevation,
			sizeof(elevation),
			&retSize
		)) {
			isAdmin = elevation.TokenIsElevated;
		}
		CloseHandle(token);
	}

	std::cout << "  [+] Administrator (UAC)       : "
		<< (isAdmin ? "YES" : "NO") << "\n";

	// --- Групи безпеки (спрощено) ---
	std::cout << "  [+] Security Groups           : ";
	if (isAdmin)
		std::cout << "Administrators ";
	std::cout << "Users\n";

	free(sid);
}

void PrintSystemDirectories() {
	char path[MAX_PATH];

	std::cout << "\n[ Системні шляхи ]\n";

	if (GetSystemDirectoryA(path, MAX_PATH))
		std::cout << "  [+] System32          : " << path << "\n";

	if (GetWindowsDirectoryA(path, MAX_PATH))
		std::cout << "  [+] Windows           : " << path << "\n";

	if (GetTempPathA(MAX_PATH, path))
		std::cout << "  [+] Temp              : " << path << "\n";

	if (GetCurrentDirectoryA(MAX_PATH, path))
		std::cout << "  [+] Current Directory : " << path << "\n";
}

void PrintProcessorInfo() {
	SYSTEM_INFO nativeInfo;
	SYSTEM_INFO wow64Info;
	OSVERSIONINFOEXA osInfo;

	ZeroMemory(&osInfo, sizeof(osInfo));
	osInfo.dwOSVersionInfoSize = sizeof(osInfo);

	// Інформація про процесор (реальна архітектура)
	GetNativeSystemInfo(&nativeInfo);

	// Інформація, яку бачить процес (важливо для x86 на x64)
	GetSystemInfo(&wow64Info);

	// Версія Windows
	#pragma warning(push)
	#pragma warning(disable : 4996)
	GetVersionExA((OSVERSIONINFOA*)&osInfo);
	#pragma warning(pop)

	std::cout << "\n[ Процесор та Система ]\n";

	// --- Ядра ---
	std::cout << "  [+] Логічні процесори        : "
		<< nativeInfo.dwNumberOfProcessors << "\n";

	// --- Архітектура (Native) ---
	std::cout << "  [+] Архітектура CPU          : ";
	switch (nativeInfo.wProcessorArchitecture) {
	case PROCESSOR_ARCHITECTURE_AMD64:
		std::cout << "x64 (AMD / Intel)\n"; break;
	case PROCESSOR_ARCHITECTURE_ARM64:
		std::cout << "ARM64\n"; break;
	case PROCESSOR_ARCHITECTURE_ARM:
		std::cout << "ARM\n"; break;
	case PROCESSOR_ARCHITECTURE_INTEL:
		std::cout << "x86\n"; break;
	default:
		std::cout << "Unknown\n"; break;
	}

	// --- Архітектура процесу ---
	std::cout << "  [+] Архітектура процесу      : ";
	switch (wow64Info.wProcessorArchitecture) {
	case PROCESSOR_ARCHITECTURE_AMD64:
		std::cout << "x64\n"; break;
	case PROCESSOR_ARCHITECTURE_INTEL:
		std::cout << "x86 (WOW64)\n"; break;
	default:
		std::cout << "Unknown\n"; break;
	}

	// --- Версія Windows ---
	std::cout << "  [+] Windows Version          : "
		<< osInfo.dwMajorVersion << "."
		<< osInfo.dwMinorVersion
		<< " (Build " << osInfo.dwBuildNumber << ")\n";

	std::cout << "  [+] Service Pack             : "
		<< (osInfo.szCSDVersion[0] ? osInfo.szCSDVersion : "None")
		<< "\n";
}