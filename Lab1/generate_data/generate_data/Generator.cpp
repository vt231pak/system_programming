#include "Shared.h"
#include <ctime>

int wmain(int argc, wchar_t* argv[]) {
    const wchar_t* fileName = (argc > 1) ? argv[1] : DEFAULT_FILE_NAME;
    std::wcout << L"Generator App. Target: " << fileName << std::endl;

    // 1. Â²ÄÊÐÈÒÒß ÔÀÉËÓ
    HANDLE hFile = CreateFile(
        fileName,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        std::cerr << "Error opening file: " << GetLastError() << std::endl;
        return 1;
    }

    // 2. ÑÈÍÕÐÎÍ²ÇÀÖ²ß (MUTEX)
    HANDLE hMutex = CreateMutex(GetNullSecurityAttributes(), FALSE, MUTEX_NAME);

    std::cout << "Waiting for access..." << std::endl;
    // ×åêàºìî, ïîêè ôàéë çâ³ëüíèòüñÿ (íàïðèêëàä, ñîðòåð çàê³í÷èòü ³òåðàö³þ)
    WaitForSingleObject(hMutex, INFINITE);

    // --- ÏÎ×ÀÒÎÊ ÊÐÈÒÈ×ÍÎ¯ ÑÅÊÖ²¯ ---

    // 3. ÃÅÍÅÐÀÖ²ß ÄÀÍÈÕ
    SharedData data;
    srand(static_cast<unsigned int>(time(0)));

    std::cout << "Generated new values: ";
    for (int i = 0; i < DATA_SIZE; ++i) {
        data.values[i] = MIN_VAL + rand() % (MAX_VAL - MIN_VAL + 1);
        std::cout << data.values[i] << " ";
    }
    std::cout << std::endl;

    // 4. ÏÅÐÅÇÀÏÈÑ ÄÀÍÈÕ
    // Ïåðåì³ùàºìî êóðñîð çàïèñó íà ïî÷àòîê ôàéëó
    SetFilePointer(hFile, 0, NULL, FILE_BEGIN);

    DWORD written;
    if (!WriteFile(hFile, &data, sizeof(SharedData), &written, NULL)) {
        std::cerr << "Write failed! Error: " << GetLastError() << std::endl;
    }
    else {
        // ßêùî ôàéë áóâ á³ëüøèì (ðàïòîì), îáð³çàºìî éîãî äî ðîçì³ðó ñòðóêòóðè
        SetEndOfFile(hFile);
        // Ïðèìóñîâî çàïèñóºìî çì³íè íà äèñê
        FlushFileBuffers(hFile);
        std::cout << "Data updated successfully (" << written << " bytes)." << std::endl;
    }

    // --- Ê²ÍÅÖÜ ÊÐÈÒÈ×ÍÎ¯ ÑÅÊÖ²¯ ---
    ReleaseMutex(hMutex);
    CloseHandle(hMutex);

    std::cout << "Press any key to exit..." << std::endl;
    system("pause");

    CloseHandle(hFile);
    return 0;
}