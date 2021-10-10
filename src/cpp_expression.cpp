// Copyright (C) 2017-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <cppast/cpp_expression.hpp>
#include <cppast/cpp_member_function.hpp>

using namespace cppast;

namespace cppast
{
cpp_member_function_call::cpp_member_function_call(std::unique_ptr<cpp_type>                 type,
                                                   type_safe::optional_ref<const cpp_entity> caller,
                                                   type_safe::optional_ref<const cpp_entity> callee,
                                                   std::string member_function)
: cpp_expression(std::move(type)), caller_(std::move(caller)), callee_(std::move(callee)),
  member_function_(std::move(member_function))
{}

std::unique_ptr<cpp_member_function_call> cpp_member_function_call::build(
    std::unique_ptr<cpp_type> type, type_safe::optional_ref<const cpp_entity> caller,
    type_safe::optional_ref<const cpp_entity> callee, std::string member_function)
{
    return std::unique_ptr<cpp_member_function_call>(
        new cpp_member_function_call(std::move(type), std::move(caller), std::move(callee),
                                     std::move(member_function)));
}

} // namespace cppast
namespace
{
void write_literal(code_generator::output& output, const cpp_literal_expression& expr)
{
    auto type_kind = cpp_void;
    if (expr.type().kind() == cpp_type_kind::builtin_t)
        type_kind = static_cast<const cpp_builtin_type&>(expr.type()).builtin_type_kind();
    else if (expr.type().kind() == cpp_type_kind::pointer_t)
    {
        auto& pointee = static_cast<const cpp_pointer_type&>(expr.type()).pointee();
        if (pointee.kind() == cpp_type_kind::builtin_t)
        {
            auto& builtin_pointee = static_cast<const cpp_builtin_type&>(pointee);
            if (builtin_pointee.builtin_type_kind() == cpp_char
                || builtin_pointee.builtin_type_kind() == cpp_wchar
                || builtin_pointee.builtin_type_kind() == cpp_char16
                || builtin_pointee.builtin_type_kind() == cpp_char32)
                // pointer to char aka string
                type_kind = builtin_pointee.builtin_type_kind();
        }
    }

    switch (type_kind)
    {
    case cpp_void:
        output << token_seq(expr.value());
        break;

    case cpp_bool:
        output << keyword(expr.value());
        break;

    case cpp_uchar:
    case cpp_ushort:
    case cpp_uint:
    case cpp_ulong:
    case cpp_ulonglong:
    case cpp_uint128:
    case cpp_schar:
    case cpp_short:
    case cpp_int:
    case cpp_long:
    case cpp_longlong:
    case cpp_int128:
        output << int_literal(expr.value());
        break;

    case cpp_float:
    case cpp_double:
    case cpp_longdouble:
    case cpp_float128:
        output << float_literal(expr.value());
        break;

    case cpp_char:
    case cpp_wchar:
    case cpp_char16:
    case cpp_char32:
        output << string_literal(expr.value());
        break;

    case cpp_nullptr:
        output << keyword(expr.value());
        break;
    }
}

void write_unexposed(code_generator::output& output, const cpp_unexposed_expression& expr)
{
    detail::write_token_string(output, expr.expression());
}

void write_member_function_call(code_generator::output&         output,
                                const cpp_member_function_call& expr)
{
    // detail::write_token_string(output, expr.expression());
    (void)output;
    (void)expr;
}
} // namespace

void detail::write_expression(code_generator::output& output, const cpp_expression& expr)
{
    switch (expr.kind())
    {
    case cpp_expression_kind::literal_t:
        write_literal(output, static_cast<const cpp_literal_expression&>(expr));
        break;
    case cpp_expression_kind::unexposed_t:
        write_unexposed(output, static_cast<const cpp_unexposed_expression&>(expr));
        break;
    case cpp_expression_kind::member_function_call_t:
        write_member_function_call(output, static_cast<const cpp_member_function_call&>(expr));
        break;
    }
}
