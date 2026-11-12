#pragma once
#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <iostream>
#include <vector>
#include <string>
#include <sddl.h>

static const wchar_t* DEFAULT_FILE_NAME = L"data.dat";
static const wchar_t* MMF_NAME = L"Global\\MySharedMemoryLab";
static const wchar_t* MUTEX_NAME = L"Global\\MyMutexLab";

static const int DATA_SIZE = 30;
static const int MIN_VAL = 10;
static const int MAX_VAL = 100;

struct SharedData {
    int values[DATA_SIZE];
};

inline LPSECURITY_ATTRIBUTES GetNullSecurityAttributes() {
    static SECURITY_ATTRIBUTES sa;
    static PSECURITY_DESCRIPTOR pSD = NULL;

    if (pSD == NULL) {
        pSD = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH);
        InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION);
        SetSecurityDescriptorDacl(pSD, TRUE, NULL, FALSE);
    }

    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = pSD;
    sa.bInheritHandle = FALSE;
    return &sa;
}