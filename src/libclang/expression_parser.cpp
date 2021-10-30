// Copyright (C) 2017-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <cppast/cpp_expression.hpp>

#include "libclang_visitor.hpp"
#include "parse_functions.hpp"

using namespace cppast;

std::unique_ptr<cpp_expression> detail::parse_expression(const detail::parse_context& context,
                                                         const CXCursor&              cur)
{
    auto kind = clang_getCursorKind(cur);
    DEBUG_ASSERT(clang_isExpression(kind), detail::assert_handler{});

    detail::cxtokenizer    tokenizer(context.tu, context.file, cur);
    detail::cxtoken_stream stream(tokenizer, cur);

    auto type = parse_type(context, cur, clang_getCursorType(cur));
    auto expr = to_string(stream, stream.end());
    if (kind == CXCursor_CallExpr || kind == CXCursor_DeclRefExpr
        || kind == CXCursor_OverloadedDeclRef)
    {
        if (expr.empty() || expr.back().spelling != ")")
        {
            // we have a call expression that doesn't end in a closing parentheses
            // this means default constructor, don't parse it at all
            // so, for example a variable doesn't have a default value
            return nullptr;
        }

        auto referenced      = clang_getCursorReferenced(cur);
        auto referenced_kind = clang_getCursorKind(referenced);
        if (referenced_kind == CXCursor_CXXMethod)
        {
            cpp_entity_id caller        = *context.current_function;
            cpp_entity_id caller_method = *context.current_function;
            cpp_entity_id callee = detail::get_entity_id(clang_getCursorSemanticParent(referenced));
            cpp_entity_id callee_method = detail::get_entity_id(referenced);

            if (context.current_class.has_value())
            {
                caller = *context.current_class;
            }

            return cpp_member_function_call::build(std::move(type), std::move(caller),
                                                   std::move(caller_method), std::move(callee),
                                                   std::move(callee_method));
        }
        else if (referenced_kind == CXCursor_FunctionDecl)
        {
            auto current_function = context.current_function;
            auto current_function_usr = context.current_function_usr;

            auto callee_entity = try_parse_cpp_function_template_specialization(context, referenced, false);

            context.current_function = current_function;
            context.current_function_usr = current_function_usr;

            cpp_entity_id caller        = *context.current_function;
            cpp_entity_id caller_method = *context.current_function;
            cpp_entity_id callee        = detail::get_entity_id(referenced);
            cpp_entity_id callee_method = detail::get_entity_id(referenced);

            (void)callee_entity;
            if (context.current_class.has_value())
            {
                caller = *context.current_class;
            }

            auto fc = cpp_member_function_call::build(std::move(type), std::move(caller),
                                                   std::move(caller_method), std::move(callee),
                                                   std::move(callee_method));
            fc->callee_entity_ = std::move(callee_entity);

            return fc;
        }
        else if (referenced_kind == CXCursor_FunctionTemplate)
        {
            cpp_entity_id caller        = *context.current_function;
            cpp_entity_id caller_method = *context.current_function;

            CXCursor callee_cursor;
            detail::visit_children(
                referenced,
                [&](const CXCursor& child) {
                    (void)child;
                    if (clang_getCursorKind(child) == CXCursor_FunctionDecl)
                        callee_cursor = child;
                },
                true);

            cpp_entity_id callee        = detail::get_entity_id(callee_cursor);
            cpp_entity_id callee_method = detail::get_entity_id(callee_cursor);

            if (context.current_class.has_value())
            {
                caller = *context.current_class;
            }

            return cpp_member_function_call::build(std::move(type), std::move(caller),
                                                   std::move(caller_method), std::move(callee),
                                                   std::move(callee_method));
        }
        else
        {
            context.logger->log("libclang parser",
                                format_diagnostic(severity::debug, detail::make_location(cur),
                                                  "cannot parse call expression: '",
                                                  detail::get_cursor_kind_spelling(cur).c_str(),
                                                  "'"));
        }

        return nullptr;
    }
    else if (kind == CXCursor_CharacterLiteral || kind == CXCursor_CompoundLiteralExpr
             || kind == CXCursor_FloatingLiteral || kind == CXCursor_ImaginaryLiteral
             || kind == CXCursor_IntegerLiteral || kind == CXCursor_StringLiteral
             || kind == CXCursor_CXXBoolLiteralExpr || kind == CXCursor_CXXNullPtrLiteralExpr)
        return cpp_literal_expression::build(std::move(type), expr.as_string());
    else
        return cpp_unexposed_expression::build(std::move(type), std::move(expr));
}

std::unique_ptr<cpp_expression> detail::parse_raw_expression(const parse_context&,
                                                             cxtoken_stream&           stream,
                                                             cxtoken_iterator          end,
                                                             std::unique_ptr<cpp_type> type)
{
    if (stream.done())
        return nullptr;

    auto expr = to_string(stream, std::prev(end)->value() == ";" ? std::prev(end) : end);
    return cpp_unexposed_expression::build(std::move(type), std::move(expr));
}
