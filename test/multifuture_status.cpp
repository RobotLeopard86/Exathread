// Test 10: MultiFuture::checkStatus() failure detection.
//
// Previously, both MultiFuture<T>::checkStatus() and MultiFuture<void>::checkStatus()
// had a bug where the line handling Status::Failed read:
//     fail = false;   // should have been: fail = true;
// This caused a multi-future where one task fails to report Status::Complete
// instead of Status::Failed. This test verifies that bug is now fixed.

#include "exathread.hpp"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <vector>

static int maybeThrow(int x) {
    if (x == 2)
        throw std::runtime_error("deliberate failure");
    return x;
}

static void voidMaybeThrow(int x) {
    if (x == 2)
        throw std::runtime_error("deliberate void failure");
}

int main() {
    auto pool = exathread::Pool::Create(2);

    // ---- MultiFuture<int>: one task fails -> must report Failed ----
    {
        std::vector<int> inputs = {1, 2, 3};
        auto mfut = pool->batch(inputs, maybeThrow);
        mfut.await();

        if (mfut.checkStatus() != exathread::Status::Failed) {
            std::cerr << "FAIL: MultiFuture<int>::checkStatus() should be Failed when a sub-task threw, got "
                      << static_cast<int>(mfut.checkStatus()) << "\n";
            return -1;
        }
    }

    // ---- MultiFuture<void>: one task fails -> must report Failed ----
    {
        std::vector<int> inputs = {1, 2, 3};
        auto mfut = pool->batch(inputs, voidMaybeThrow);
        mfut.await();

        if (mfut.checkStatus() != exathread::Status::Failed) {
            std::cerr << "FAIL: MultiFuture<void>::checkStatus() should be Failed when a sub-task threw, got "
                      << static_cast<int>(mfut.checkStatus()) << "\n";
            return -1;
        }
    }

    // ---- MultiFuture<int>: all succeed -> must report Complete ----
    {
        std::vector<int> inputs = {1, 3, 5};
        auto mfut   = pool->batch(inputs, maybeThrow);
        auto results = mfut.results();

        if (mfut.checkStatus() != exathread::Status::Complete) {
            std::cerr << "FAIL: all-success MultiFuture should be Complete, got "
                      << static_cast<int>(mfut.checkStatus()) << "\n";
            return -1;
        }
        if (results.size() != 3 || results[0] != 1 || results[1] != 3 || results[2] != 5) {
            std::cerr << "FAIL: unexpected results from all-success batch\n";
            return -1;
        }
    }

    std::cout << "PASS\n";
    return 0;
}
