// Copyright (C) 2017-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <cppast/cpp_expression.hpp>
#include <cppast/cpp_function.hpp>
#include <cppast/cpp_member_function.hpp>

#include "test_parser.hpp"

using namespace cppast;

TEST_CASE("cpp_call_expression")
{
    auto code = R"(
#include <algorithm>
#include <numeric>
#include <vector>

namespace detail {
struct C {
    /// auto add(int x,int y);
    auto add(int x, int y) { return x + y; }
};
}

class A {
public:
    A() { }

    /// int add(int x,int y);
    int add(int x, int y) { return m_c.add(x, y); }

    /// int add3(int x,int y,int z);
    int add3(int x, int y, int z)
    {
        std::vector<int> v;
        v.push_back(x);
        v.push_back(y);
        v.push_back(z);
        auto res = add(v[0], v[1]) + v[2];
        log_result(res);
        return res;
    }

    /// void log_result(int r);
    void log_result(int r) { }

private:
    detail::C m_c{};
};

class B {
public:
    /// B(A &a);
    B(A &a)
        : m_a{a}
    {
    }

    /// int wrap_add(int x,int y);
    int wrap_add(int x, int y)
    {
        auto res = m_a.add(x, y);
        m_a.log_result(res);
        return res;
    }

    /// int wrap_add3(int x,int y,int z);
    int wrap_add3(int x, int y, int z)
    {
        auto res = m_a.add3(x, y, z);
        m_a.log_result(res);
        return res;
    }

private:
    A &m_a;
};


/// int tmain();
int tmain()
{
    A a;
    B b(a);

    return b.wrap_add3(1, 2, 3);
}
)";

    cpp_entity_index idx;
    auto             file  = parse(idx, "cpp_call_expression.cpp", code);
    auto             count = test_visit<cpp_function>(*file, [&](const cpp_entity& e) {
        if (e.kind() == cpp_entity_kind::function_t)
        {
            if (e.name() == "tmain")
            {
                const auto& tmain = static_cast<const cpp_function&>(e);
                REQUIRE(tmain.function_calls().size() == 1);

                const auto& wrap_add3
                    = static_cast<const cpp_member_function_call&>(*tmain.function_calls()[0]);
                REQUIRE(!wrap_add3.get_caller().has_value());
                REQUIRE(wrap_add3.get_callee().value().name() == "B");
                REQUIRE(wrap_add3.get_member_function() == "wrap_add3");
            }
        }
    });

    REQUIRE(count == 1u);

    count = test_visit<cpp_member_function>(*file, [&](const cpp_entity& e) {
        if (e.kind() == cpp_entity_kind::member_function_t)
        {
            if (e.name() == "wrap_add")
            {
                const auto& wrap_add = static_cast<const cpp_function&>(e);
                REQUIRE(wrap_add.function_calls().size() == 2);

                const auto& add
                    = static_cast<const cpp_member_function_call&>(*wrap_add.function_calls()[0]);
                REQUIRE(add.get_caller().value().name() == "B");
                REQUIRE(add.get_callee().value().name() == "A");
                REQUIRE(add.get_member_function() == "add");

                const auto& log_result
                    = static_cast<const cpp_member_function_call&>(*wrap_add.function_calls()[1]);
                REQUIRE(log_result.get_caller().value().name() == "B");
                REQUIRE(log_result.get_callee().value().name() == "A");
                REQUIRE(log_result.get_member_function() == "log_result");
            }
            else if (e.name() == "wrap_add3")
            {
                const auto& wrap_add3 = static_cast<const cpp_function&>(e);
                REQUIRE(wrap_add3.function_calls().size() == 2);

                const auto& add3
                    = static_cast<const cpp_member_function_call&>(*wrap_add3.function_calls()[0]);
                REQUIRE(add3.get_caller().value().name() == "B");
                REQUIRE(add3.get_callee().value().name() == "A");
                REQUIRE(add3.get_member_function() == "add3");

                const auto& log_result
                    = static_cast<const cpp_member_function_call&>(*wrap_add3.function_calls()[1]);
                REQUIRE(log_result.get_caller().value().name() == "B");
                REQUIRE(log_result.get_callee().value().name() == "A");
                REQUIRE(log_result.get_member_function() == "log_result");
            }
        }
    });
    REQUIRE(count == 6u);
}
