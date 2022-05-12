// Copyright (C) 2017-2022 Jonathan Müller and cppast contributors
// SPDX-License-Identifier: MIT

#include <cppast/cpp_member_variable.hpp>
#include <cppast/cpp_template.hpp>

#include "test_parser.hpp"

using namespace cppast;

TEST_CASE("cpp_member_variable")
{
    auto code = R"(
struct foo
{
    /// int a;
    int a;
    /// float b=3.14f;
    float b = 3.14f;
    /// mutable char c;
    mutable char c;
};
)";

    cpp_entity_index idx;
    auto             file = parse(idx, "cpp_member_variable.cpp", code);
    auto count = test_visit<cpp_member_variable>(*file, [&](const cpp_member_variable& var) {
        if (var.name() == "a")
        {
            auto type = cpp_builtin_type::build(cpp_int);
            REQUIRE(equal_types(idx, var.type(), *type));
            REQUIRE(!var.default_value());
            REQUIRE(!var.is_mutable());
        }
        else if (var.name() == "b")
        {
            auto type = cpp_builtin_type::build(cpp_float);
            REQUIRE(equal_types(idx, var.type(), *type));

            // all initializers are unexposed
            auto def = cpp_unexposed_expression::build(cpp_builtin_type::build(cpp_float),
                                                       cpp_token_string::tokenize("3.14f"));
            REQUIRE(var.default_value());
            REQUIRE(equal_expressions(var.default_value().value(), *def));

            REQUIRE(!var.is_mutable());
        }
        else if (var.name() == "c")
        {
            auto type = cpp_builtin_type::build(cpp_char);
            REQUIRE(equal_types(idx, var.type(), *type));
            REQUIRE(!var.default_value());
            REQUIRE(var.is_mutable());
        }
        else
            REQUIRE(false);
    });
    REQUIRE(count == 3u);
}

TEST_CASE("cpp_bitfield")
{
    auto code = R"(
struct foo
{
    /// char a:3;
    char a : 3;
    /// mutable char b:2;
    mutable char b : 2;
    /// char:0;
    char : 0;
    /// char c:3;
    char c : 3;
    /// char:4;
    char : 4;
};
)";

    auto file  = parse({}, "cpp_bitfield.cpp", code);
    auto count = test_visit<cpp_bitfield>(*file, [&](const cpp_bitfield& bf) {
        REQUIRE(!bf.default_value());
        REQUIRE(equal_types({}, bf.type(), *cpp_builtin_type::build(cpp_char)));

        if (bf.name() == "a")
        {
            REQUIRE(bf.no_bits() == 3u);
            REQUIRE(!bf.is_mutable());
        }
        else if (bf.name() == "b")
        {
            REQUIRE(bf.no_bits() == 2u);
            REQUIRE(bf.is_mutable());
        }
        else if (bf.name() == "c")
        {
            REQUIRE(bf.no_bits() == 3u);
            REQUIRE(!bf.is_mutable());
        }
        else if (bf.name() == "")
        {
            REQUIRE(!bf.is_mutable());
            if (bf.no_bits() != 0u && bf.no_bits() != 4u)
            {
                INFO(bf.no_bits());
                REQUIRE(false);
            }
        }
        else
            REQUIRE(false);
    });
    REQUIRE(count == 5u);
}

TEST_CASE("cpp_member_variable_template_instantiation")
{
    auto code = R"(
#include <string>
#include <tuple>

template <typename T> struct A {
    /// T at;
    T at;
};

template <typename T, typename... Args> struct B {
    /// T bt;
    T bt;

    /// std::tuple<Args...> bargs;
    std::tuple<Args...> bargs;
};

struct foo
{
    /// A<int> aint;
    A<int> aint;

    /// A<std::string> astring;
    A<std::string> astring;

    /// B<int,A<float>> bintafloat;
    B<int, A<float>> bintafloat;
};
)";

    cpp_entity_index idx;
    auto             file = parse(idx, "cpp_member_variable_template_instantiation.cpp", code);
    auto count = test_visit<cpp_member_variable>(*file, [&](const cpp_member_variable& var) {
        if (var.name() == "at")
        {
            // ignore
        }
        else if (var.name() == "bt")
        {
            // ignore
        }
        else if (var.name() == "bargs")
        {
            // ignore
        }
        else if (var.name() == "aint")
        {
            REQUIRE(var.type().kind() == cppast::cpp_type_kind::template_instantiation_t);
            const auto& vartype = static_cast<const cpp_template_instantiation_type&>(var.type());
            REQUIRE(vartype.arguments_exposed());

            REQUIRE(cppast::to_string(vartype) == "A<int>");

            REQUIRE(vartype.arguments().has_value());
            REQUIRE(vartype.arguments().value().size().get() == 1U);
        }
        else if (var.name() == "astring")
        {
            REQUIRE(var.type().kind() == cppast::cpp_type_kind::template_instantiation_t);
            const auto& vartype = static_cast<const cpp_template_instantiation_type&>(var.type());
            REQUIRE(vartype.arguments_exposed());

            REQUIRE(cppast::to_string(vartype) == "A<std::string>");

            REQUIRE(vartype.arguments().has_value());
            REQUIRE(vartype.arguments().value().size().get() == 1U);
            REQUIRE(!var.default_value());
            REQUIRE(!var.is_mutable());
        }
        else if (var.name() == "bintafloat")
        {
            REQUIRE(var.type().kind() == cppast::cpp_type_kind::template_instantiation_t);
            const auto& vartype = static_cast<const cpp_template_instantiation_type&>(var.type());
            REQUIRE(vartype.arguments_exposed());

            REQUIRE(cppast::to_string(vartype) == "B<int,A<float>>");

            REQUIRE(vartype.arguments().has_value());
            REQUIRE(vartype.arguments().value().size().get() == 2U);

            REQUIRE(!var.default_value());
            REQUIRE(!var.is_mutable());
        }
        else
            REQUIRE(false);
    });
    REQUIRE(count == 6u);
}
