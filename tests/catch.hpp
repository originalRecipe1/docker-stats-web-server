#ifndef MINI_CATCH_HPP
#define MINI_CATCH_HPP

#include <iostream>
#include <stdexcept>
#include <vector>
#include <utility>
#include <functional>
#include <cstdlib>

namespace mini_catch {
    using TestFunc = std::function<void()>;
    inline std::vector<std::pair<std::string, TestFunc>>& registry() {
        static std::vector<std::pair<std::string, TestFunc>> tests;
        return tests;
    }

    inline int run() {
        int failed = 0;
        for (const auto& t : registry()) {
            try {
                t.second();
                std::cout << "[PASS] " << t.first << std::endl;
            } catch (const std::exception& e) {
                ++failed;
                std::cerr << "[FAIL] " << t.first << " - " << e.what() << std::endl;
            } catch (...) {
                ++failed;
                std::cerr << "[FAIL] " << t.first << std::endl;
            }
        }
        return failed;
    }
}

class TestRegistrar {
public:
    TestRegistrar(const std::string& name, mini_catch::TestFunc func) {
        mini_catch::registry().push_back({name, func});
    }
};

#define CONCAT_INNER(a, b) a##b
#define CONCAT(a, b) CONCAT_INNER(a, b)

#define TEST_CASE(name) \
    void CONCAT(test_func_, __LINE__)(); \
    static TestRegistrar CONCAT(test_reg_, __LINE__)(name, CONCAT(test_func_, __LINE__)); \
    void CONCAT(test_func_, __LINE__)()

#define REQUIRE(cond) \
    do { if(!(cond)) throw std::runtime_error("Requirement failed: " #cond); } while(0)

#ifdef CATCH_CONFIG_MAIN
int main() {
    return mini_catch::run();
}
#endif

#endif // MINI_CATCH_HPP
