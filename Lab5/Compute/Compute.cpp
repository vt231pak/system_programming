#include "../Interface/ICompute.h"
#include <vector>
#include <thread>
#include <numeric>
#include <algorithm>
#include <mutex>
#include <windows.h>

class Compute : public ICompute {
private:
    std::vector<int> data;
    long long result = 0;
    std::mutex mtx;

    void PartialSum(size_t start, size_t end) {
        long long partial = 0;
        for (size_t i = start; i < end; ++i) {
            partial += data[i];
            for (int k = 0; k < 100; k++) {
                volatile int x = k * k;
            }
        }

        std::lock_guard<std::mutex> lock(mtx);
        result += partial;
    }

public:
    void SetData(const std::vector<int>& inputData) override {
        data = inputData;
    }

    void Run(int threadsCount) override {
        result = 0;
        if (data.empty()) return;
        if (threadsCount < 1) threadsCount = 1;

        std::vector<std::thread> threads;
        size_t chunkSize = data.size() / threadsCount;

        for (int i = 0; i < threadsCount; ++i) {
            size_t start = i * chunkSize;
            size_t end = (i == threadsCount - 1) ? data.size() : (i + 1) * chunkSize;

            threads.emplace_back(&Compute::PartialSum, this, start, end);
        }

        for (auto& t : threads) {
            if (t.joinable()) t.join();
        }
    }

    long long GetResult() override {
        return result;
    }

    const char* GetVersion() override {
        return "1.0.0 (Multi-threaded Edition)";
    }

    void Release() override {
        delete this;
    }
};

extern "C" __declspec(dllexport) ICompute* CreateCompute() {
    return new Compute();
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    return TRUE;
}