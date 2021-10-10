// Copyright (C) 2017-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef CPPAST_CPP_EXPRESSION_HPP_INCLUDED
#define CPPAST_CPP_EXPRESSION_HPP_INCLUDED

#include <atomic>
#include <memory>

#include <cppast/cpp_token.hpp>
#include <cppast/cpp_type.hpp>

namespace cppast
{
class cpp_member_function;
/// The kind of a [cppast::cpp_expression]().
enum class cpp_expression_kind
{
    literal_t,
    member_function_call_t,

    unexposed_t,
};

/// Base class for all C++ expressions.
class cpp_expression
{
public:
    cpp_expression(const cpp_expression&) = delete;
    cpp_expression& operator=(const cpp_expression&) = delete;

    virtual ~cpp_expression() noexcept = default;

    /// \returns The [cppast::cpp_expression_kind]().
    cpp_expression_kind kind() const noexcept
    {
        return do_get_kind();
    }

    /// \returns The type of the expression.
    const cpp_type& type() const noexcept
    {
        return *type_;
    }

    /// \returns The specified user data.
    void* user_data() const noexcept
    {
        return user_data_.load();
    }

    /// \effects Sets some kind of user data.
    ///
    /// User data is just some kind of pointer, there are no requirements.
    /// The class will do no lifetime management.
    ///
    /// User data is useful if you need to store additional data for an entity without the need to
    /// maintain a registry.
    void set_user_data(void* data) const noexcept
    {
        user_data_ = data;
    }

protected:
    /// \effects Creates it given the type.
    /// \requires The type must not be `nullptr`.
    cpp_expression(std::unique_ptr<cpp_type> type) : type_(std::move(type)), user_data_(nullptr)
    {
        DEBUG_ASSERT(type_ != nullptr, detail::precondition_error_handler{});
    }

private:
    /// \returns The [cppast::cpp_expression_kind]().
    virtual cpp_expression_kind do_get_kind() const noexcept = 0;

    std::unique_ptr<cpp_type>  type_;
    mutable std::atomic<void*> user_data_;
};

/// An unexposed [cppast::cpp_expression]().
///
/// There is no further information than a string available.
class cpp_unexposed_expression final : public cpp_expression
{
public:
    /// \returns A newly created unexposed expression.
    static std::unique_ptr<cpp_unexposed_expression> build(std::unique_ptr<cpp_type> type,
                                                           cpp_token_string          str)
    {
        return std::unique_ptr<cpp_unexposed_expression>(
            new cpp_unexposed_expression(std::move(type), std::move(str)));
    }

    /// \returns The expression as a string.
    const cpp_token_string& expression() const noexcept
    {
        return str_;
    }

private:
    cpp_unexposed_expression(std::unique_ptr<cpp_type> type, cpp_token_string str)
    : cpp_expression(std::move(type)), str_(std::move(str))
    {}

    cpp_expression_kind do_get_kind() const noexcept override
    {
        return cpp_expression_kind::unexposed_t;
    }

    cpp_token_string str_;
};

/// A [cppast::cpp_expression]() that is a literal.
class cpp_literal_expression final : public cpp_expression
{
public:
    /// \returns A newly created literal expression.
    static std::unique_ptr<cpp_literal_expression> build(std::unique_ptr<cpp_type> type,
                                                         std::string               value)
    {
        return std::unique_ptr<cpp_literal_expression>(
            new cpp_literal_expression(std::move(type), std::move(value)));
    }

    /// \returns The value of the literal, as string.
    const std::string& value() const noexcept
    {
        return value_;
    }

private:
    cpp_literal_expression(std::unique_ptr<cpp_type> type, std::string value)
    : cpp_expression(std::move(type)), value_(std::move(value))
    {}

    cpp_expression_kind do_get_kind() const noexcept override
    {
        return cpp_expression_kind::literal_t;
    }

    std::string value_;
};

/// A [cppast::cpp_expression]() that is a member function call.
class cpp_member_function_call final : public cpp_expression
{
public:
    /// \returns A newly created member function call.
    static std::unique_ptr<cpp_member_function_call> build(
        std::unique_ptr<cpp_type> type, type_safe::optional_ref<const cpp_entity> caller,
        type_safe::optional_ref<const cpp_entity> callee, std::string member_function);

    type_safe::optional_ref<const cpp_entity> get_caller() const
    {
        return caller_;
    }

    type_safe::optional_ref<const cpp_entity> get_callee() const
    {
        return callee_;
    }

    std::string get_member_function() const
    {
        return member_function_;
    }

private:
    cpp_member_function_call(std::unique_ptr<cpp_type>                 type,
                             type_safe::optional_ref<const cpp_entity> caller,
                             type_safe::optional_ref<const cpp_entity> callee,
                             std::string member_function);

    cpp_expression_kind do_get_kind() const noexcept override
    {
        return cpp_expression_kind::member_function_call_t;
    }

    /// Reference to the scope where the call is made from
    /// e.g. cpp_function or cpp_member_function
    type_safe::optional_ref<const cpp_entity> caller_;

    /// Reference to the target (e.g. cpp_class)
    type_safe::optional_ref<const cpp_entity> callee_;

    /// Reference to the member function
    std::string member_function_;
};

/// \exclude
namespace detail
{
    void write_expression(code_generator::output& output, const cpp_expression& expr);
} // namespace detail
} // namespace cppast

#endif // CPPAST_CPP_EXPRESSION_HPP_INCLUDED
