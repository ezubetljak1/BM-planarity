#pragma once 

#include <exception>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace bm::test {

    using TestFunction = std::function<void()>;

    struct TestCase {
        std::string name;
        TestFunction function;
    };

    inline std::vector<TestCase>& registry() {
        static std::vector<TestCase> tests;
        return tests;
    }

    struct TestRegistrar {
        TestRegistrar(const std::string& name, TestFunction function ){
            registry().emplace_back(name, function);
        }
    };

    inline void assertTrue(
        bool condition,
        const char* expression,
        const char* file,
        int line
    ) {
        if (!condition) {
            throw std::runtime_error(
                std::string(file) +
                ":" + 
                std::to_string(line) +
                " assertion failed: " + 
                expression
            );
        }
    }

    inline int runAllTests() {
        int passed = 0;
        int failed = 0;

        for (const TestCase& test : registry()) {
            try {
                test.function();
                ++passed;
                std::cout << "[PASS] " << test.name << '\n';
            } catch (const std::exception& ex) {
                ++failed;
                std::cerr << "[FAIL] " << test.name << '\n';
                std::cerr << "       " << ex.what() << '\n';
            } catch (...) {
                ++failed;
                std::cerr << "[FAIL] " << test.name << '\n';
                std::cerr << "       unknown exception\n";
            }
        }

        std::cout << "\nPassed: " << passed << '\n';
        std::cout << "Failed: " << failed << '\n';

        return failed == 0 ? 0 : 1;
    }

} // namespace bm::test

#define BM_ASSERT(expr) \
    ::bm::test::assertTrue((expr), #expr, __FILE__, __LINE__)

#define BM_TEST(name) \
    void name(); \
    static ::bm::test::TestRegistrar registrar_##name(#name, name); \
    void name()


/*
*****************************************
*                                       *
*    Omogucuje pisanje testova tipa:    *
*                                       * 
*    BM_TEST(NekiTest) {                *
*        BM_ASSERT(1 + 1 == 2);         *
*    }                                  *
*                                       *
*****************************************
*/