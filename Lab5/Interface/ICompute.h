#pragma once
#include <vector>

#ifdef INTERFACE_EXPORTS
#define INTERFACE_API __declspec(dllexport)
#else
#define INTERFACE_API __declspec(dllimport)
#endif

class INTERFACE_API ICompute {
public:
    virtual void SetData(const std::vector<int>& data) = 0;
    virtual void Run(int threadsCount) = 0;
    virtual long long GetResult() = 0;
    virtual const char* GetVersion() = 0;
    virtual void Release() = 0;
    virtual ~ICompute() {}
};

typedef ICompute* (*CreateCompute_t)();