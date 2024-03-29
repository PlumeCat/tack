#include "parsing.h"
#include "interpreter.h"

#include <stdexcept>

// parsing utils
using namespace std::string_literals;

struct ParseContext : private std::string_view {
    uint32_t line_number = 1;
    Interpreter* vm;
    
    ParseContext(const std::string& s, Interpreter* vm) : std::string_view(s), vm(vm) {}
    void remove_prefix(std::string_view::size_type s) noexcept {
        for (auto i = 0u; i < s; i++) {
            if ((*this)[i] == '\n') {
                line_number += 1;
            }
        }
        std::string_view::remove_prefix(s);
    }

    using std::string_view::operator[];
    using std::string_view::size;
    using std::string_view::data;
    using std::string_view::substr;
};

#define SUCCESS(...) { out = AstNode(__VA_ARGS__); out.line_number = _c.line_number; return true; }
#define FAIL(); { code = _c; return false; }
#ifdef ERROR
#undef ERROR
#endif
#define ERROR(msg) code.vm->error("parsing error: "s + msg + " | line: " + std::to_string(code.line_number) + "\n'" + std::string(code.substr(0, 32))  + "... '");
#define EXPECT(s) if (!parse_raw_string(code, s)) { FAIL(); }
#define EXPECT_WS(s) if (!(parse_raw_string(code, s) && (isspace(code[0]) || !code.size()))) { FAIL(); }

#define paste(a, b) a##b
#define DECLPARSER(name) bool paste(parse_, name)(ParseContext& code, AstNode& out)
#define DEFPARSER(name, body) DECLPARSER(name) { skip_whitespace(code); auto _c = code; body; FAIL(); }
#define SUBPARSER(name, body) auto paste(parse_, name) = [&](ParseContext& code, AstNode& out) -> bool { skip_whitespace(code); auto _c = code; body; FAIL(); };

#define TRY4(n1,n2,n3,n4)\
    auto n1 = AstNode {}; auto n2 = AstNode {}; auto n3 = AstNode {}; auto n4 = AstNode {};\
    if (paste(parse_, n1)(code, n1) && paste(parse_, n2)(code, n2) && paste(parse_, n3) && paste(parse_, n4))
#define TRY3(n1,n2,n3)\
    auto n1 = AstNode {}; auto n2 = AstNode {}; auto n3 = AstNode {};\
    if (paste(parse_, n1)(code, n1) && paste(parse_, n2)(code, n2) && paste(parse_, n3))
#define TRY2(n1,n2)\
    auto n1 = AstNode {}; auto n2 = AstNode {};\
    if (paste(parse_, n1)(code, n1) && paste(parse_, n2)(code, n2))
#define TRY(n1)\
    auto n1 = AstNode {};\
    if (paste(parse_, n1)(code, n1))
#define TRYs(s)\
    if (parse_raw_string(code, s))

bool is_identifier_char(char c) {
    return isalnum(c) || c == '_';
}
bool is_identifier_start_char(char c) {
    return isalpha(c) || c == '_';
}
bool skip_whitespace(ParseContext& code) {
    auto n = 0u;
    for (; n < code.size(); n++) {
        if (!isspace(code[n])) {
            break;
        }
    }
    code.remove_prefix(n);
    return n > 0;
}

bool parse_raw_number(ParseContext& code, double& out) {
    auto end = (char*)nullptr;
    auto result = std::strtod(code.data(), &end);
    auto dist = end - code.data();
    if (dist != 0) {
        code.remove_prefix(dist);
        out = result;
        return true;
    }
    return false;
}
bool parse_raw_string_literal(ParseContext& code, std::string& out) {
    skip_whitespace(code);
    if (code.size() && code[0] == '"') {
        for (auto n = 1u; n < code.size(); n++) {
            if (code[n] == '"') {
                out = code.substr(1, n - 1);
                code.remove_prefix(n + 1);
                return true;
            }
        }
        ERROR("expected closing quote '\"'");
    }
    return false;
}
bool parse_raw_identifier(ParseContext& code, std::string& out) {
    skip_whitespace(code);
    if (code.size() && is_identifier_start_char(code[0])) {
        for (auto n = 1u; n < code.size(); n++) {
            if (!is_identifier_char(code[n])) {
                out = code.substr(0, n);
                code.remove_prefix(n);
                return true;
            }
        }
    }
    return false;
}
bool parse_raw_string(ParseContext& code, char c) {
    skip_whitespace(code);
    if (code.size() && code[0] == c) {
        code.remove_prefix(1);
        return true;
    }
    return false;
}
bool parse_raw_string(ParseContext& code, const std::string& c) {
    skip_whitespace(code);
    if (code.size() >= c.size() && code.substr(0, c.size()) == c) {
        code.remove_prefix(c.size());
        return true;
    }
    return false;
}

// left recursive binary operation
// eg SubExpr = SubExpr '-' MulExpr
//			  | MulExpr
#define BINOP(name, ty, precedent, op)\
    DEFPARSER(name, {\
        TRY(precedent) {\
            auto res = precedent;\
            while (true) {\
                TRYs(op) {\
                    TRY(precedent) {\
                        res = AstNode(AstType::ty, res, precedent);\
                    } else ERROR("expected RHS for binary op '" op "'");\
                } else break;\
            }\
            SUCCESS(res);\
        }\
   });
#define BINOP_S(name, ty, precedent, op)\
    DEFPARSER(name, {\
        TRY(precedent) {\
            auto res = precedent;\
            while (true) {\
                TRYs(op) {\
                    if (skip_whitespace(code)) {\
                        TRY(precedent) {\
                            res = AstNode(AstType::ty, res, precedent);\
                        } else ERROR("expected RHS for binary op '" op "'");\
                    } else FAIL() /*ERROR("expected space after '" op "' operator")*/\
                } else break;\
            }\
            SUCCESS(res);\
        }\
   });

// binary operation non recursive in the grammar sense
#define BINOP2(name, ty, precedent, op)\
    DEFPARSER(name, {\
        TRY(precedent) {\
            auto lhs = precedent;\
            TRYs(op) {\
                TRY(precedent) {\
                    SUCCESS(AstType::ty, lhs, precedent)\
                }\
            }\
            SUCCESS(lhs)\
        }\
   });

// language
DECLPARSER(stat_list);
DECLPARSER(exp);
DECLPARSER(block);

DEFPARSER(identifier, {
    auto identifier = ""s;
    if (parse_raw_identifier(code, identifier)) {
        SUCCESS(AstType::Identifier, identifier);
    }
});
DEFPARSER(param_def, {
    auto p = AstNode(AstType::ParamDef);
    while (true) {
        TRY(identifier) {
            p.children.push_back(identifier);
            TRYs(',') {} else break;
        } else break;
    }
    SUCCESS(p);
});
DEFPARSER(func_literal, {
    EXPECT("fn")
    TRYs('(') {} else ERROR("expected parameter definition after 'fn'");
    TRY(param_def) {
        TRYs(')') {} else ERROR("expected ')' after parameter definition");
        TRY(block) {
            SUCCESS(AstType::FuncLiteral, param_def, block);
        } else ERROR("expected block after parameter definition");
    }
});

DEFPARSER(literal, {
    SUBPARSER(symbol_literal, {
        TRYs("true") SUCCESS(true);
        TRYs("false") SUCCESS(false);
        TRYs("null") SUCCESS(AstType::NullLiteral);
    })
    SUBPARSER(num_literal, {
        auto num = 0.0;
        if (parse_raw_number(code, num)) {
            SUCCESS(num);
        }
    });
    SUBPARSER(string_literal, {
        auto str = ""s;
        if (parse_raw_string_literal(code, str)) {
            SUCCESS(AstType::StringLiteral, str);
        }
    });
    SUBPARSER(object_literal, {
        TRYs('{') {
            auto res = AstNode(AstType::ObjectLiteral);

            while (true) {
                TRY(identifier) {
                    TRYs('=') {
                        TRY(exp) {
                            res.children.emplace_back(AstType::AssignStat, identifier, exp);
                            TRYs(',') {} else break;
                        } else ERROR("expected expression after '=' (object)")
                    } else ERROR("expected '=' after key (object)")
                } else break;
            }

            TRYs('}') {} else ERROR("expected ',' or '}' after member")
            SUCCESS(res);
        }
    });
    SUBPARSER(array_literal, {
        TRYs('[') {
            auto res = AstNode(AstType::ArrayLiteral);
            while (true) {
                TRY(exp) {
                    res.children.emplace_back(exp);
                    TRYs(',') {} else break;
                } else break;
            }

            EXPECT(']');
            SUCCESS(res);
        }
    });
    TRY(string_literal) SUCCESS(string_literal);
    TRY(array_literal) SUCCESS(array_literal);
    TRY(object_literal) SUCCESS(object_literal);
    TRY(func_literal) SUCCESS(func_literal);
    TRY(num_literal) SUCCESS(num_literal);
    TRY(symbol_literal) SUCCESS(symbol_literal);
});

DEFPARSER(primary_exp, {
    TRY(literal) SUCCESS(literal)
    TRY(identifier) SUCCESS(identifier)
    TRYs('(') {
        TRY(exp) {
            TRYs(')') {} else ERROR("expected closing ')'");
            SUCCESS(exp)
        }
    }
});
DEFPARSER(postfix_exp, {
    SUBPARSER(access_postfix, {
        // access expr
        TRYs('.') {
            TRY(identifier) {
                SUCCESS(identifier)
            } else ERROR("expected identifier after '.'");
        }
    });
    SUBPARSER(index_postfix, {
        // index expr
        TRYs('[') {
            TRY(exp) {
                TRYs(']') {
                    SUCCESS(exp)
                }
            }
        }
    });
    SUBPARSER(call_postfix, {
        SUBPARSER(arg_list, {
            auto res = AstNode(AstType::ArgList);

            TRY(exp) {
                res.children.emplace_back(exp);
            } else SUCCESS(res)

            while (true) {
                TRYs(',') {
                    TRY(exp) {
                        res.children.emplace_back(exp);
                    } else ERROR("unexpected symbol in argument list");
                } else break;
            }
            SUCCESS(res)
        });
        TRYs('(') {
            TRY(arg_list) {
                TRYs(')') {
                    SUCCESS(arg_list)
                } else ERROR("expected ')' after argument list");
            } else ERROR("expected ')' or argument list after '('");
        }
    });
    TRY(primary_exp) {
        while (true) {
            TRY(call_postfix) {
                primary_exp = AstNode(AstType::CallExp, primary_exp, call_postfix);
                continue;
            }
            TRY(access_postfix) {
                primary_exp = AstNode(AstType::AccessExp, primary_exp, access_postfix); // access_postfix is Identifier
                continue;
            }
            TRY(index_postfix) {
                primary_exp = AstNode(AstType::IndexExp, primary_exp, index_postfix); // index_postfix is Exp
                continue;
            }
            break;
        }
        SUCCESS(primary_exp)
    }
});
DEFPARSER(prefix_exp, {
    TRYs('-') {
        TRY(prefix_exp) SUCCESS(AstType::NegateExp, prefix_exp);
    } else TRYs('!') {
        TRY(prefix_exp) SUCCESS(AstType::NotExp, prefix_exp);
    } else TRYs('~') {
        TRY(prefix_exp) SUCCESS(AstType::BitNotExp, prefix_exp);
    } else TRYs('#') {
        TRY(prefix_exp) SUCCESS(AstType::LenExp, prefix_exp);
    }
    TRY(postfix_exp) SUCCESS(postfix_exp)
});
DEFPARSER(unary_exp, {
    TRY(prefix_exp) SUCCESS(prefix_exp)
});

BINOP(pow_exp, PowExp, unary_exp, "**");
BINOP(mod_exp, ModExp, pow_exp, "%");
BINOP(div_exp, DivExp, mod_exp, "/");
BINOP(mul_exp, MulExp, div_exp, "*");
BINOP(sub_exp, SubExp, mul_exp, "-");
BINOP(add_exp, AddExp, sub_exp, "+");
BINOP(shr_exp, ShiftRightExp, add_exp, ">>");
BINOP(shl_exp, ShiftLeftExp, shr_exp, "<<");

DEFPARSER(cmp_exp, {
    TRY(shl_exp) {
        auto lhs = shl_exp;
        TRYs("==") { TRY(shl_exp) { SUCCESS(AstType::EqExp,       lhs, shl_exp); } }
        TRYs("!=") { TRY(shl_exp) { SUCCESS(AstType::NotEqExp,    lhs, shl_exp); } }
        TRYs("<=") { TRY(shl_exp) { SUCCESS(AstType::LessEqExp,   lhs, shl_exp); } }
        TRYs(">=") { TRY(shl_exp) { SUCCESS(AstType::GreaterEqExp,lhs, shl_exp); } }
        TRYs("<")  { TRY(shl_exp) { SUCCESS(AstType::LessExp,     lhs, shl_exp); } }
        TRYs(">")  { TRY(shl_exp) { SUCCESS(AstType::GreaterExp,  lhs, shl_exp); } }
        SUCCESS(lhs);
    }
});

BINOP(bit_and_exp, BitAndExp, cmp_exp, "&");
BINOP(bit_xor_exp, BitXorExp, bit_and_exp, "^");
BINOP(bit_or_exp, BitOrExp, bit_xor_exp, "|");
BINOP(in_exp, InExp, bit_or_exp, "in ");
BINOP(and_exp, AndExp, in_exp, "and ");
BINOP(or_exp, OrExp, and_exp, "or ");

DEFPARSER(ternary_exp, {
    TRY(or_exp) {
        TRYs('?') {
            TRY(exp) {
                auto e1 = exp;
                TRYs(':') {} else ERROR("Expected ':' in ternary expression");
                TRY(exp) {} else ERROR("Expected expression after ':'");
                SUCCESS(AstType::TernaryExp, or_exp, e1, exp);
            } else ERROR("Expected expression after '?'");
        }
        SUCCESS(or_exp);
    }
});

DEFPARSER(exp, { TRY(or_exp) SUCCESS(or_exp); });

// or, and, in, |, ^, &, cmp, <<, >>, +, -, *, /, %, **


DEFPARSER(const_decl_stat, {
    auto is_export = parse_raw_string(code, "export") && skip_whitespace(code);
    EXPECT_WS("const")
    TRY(identifier) {
        identifier.data_d = is_export;
        EXPECT('=')
        TRY(exp) { SUCCESS(AstType::ConstDeclStat, identifier, exp) }
    }
});
DEFPARSER(var_decl_stat, {
    auto is_export = parse_raw_string(code, "export") && skip_whitespace(code);
    EXPECT_WS("let")
    TRY(identifier) {
        identifier.data_d = is_export;
        EXPECT('=')
        TRY(exp) {
            SUCCESS(AstType::VarDeclStat, identifier, exp)
        }
    }
});
DEFPARSER(func_decl_stat, {
    auto is_export = parse_raw_string(code, "export") && skip_whitespace(code);
    EXPECT_WS("fn")
    TRY(identifier) { identifier.data_d = is_export; } else { FAIL(); } // parsing failure not an error - could be a freestanding anonymous fn
    TRYs('(') {} else ERROR("expected parameter definition after identifier");
    TRY(param_def) {
        TRYs(')') {} else ERROR("expected ')' after parameter definition");
        TRY(block) {
            SUCCESS(AstType::FuncDeclStat, identifier,
                AstNode(AstType::FuncLiteral, param_def, block)
            );
        }
    }
});

DEFPARSER(assign_stat, {
    TRY(postfix_exp) {
        TRYs('=') {
            TRY(exp) {
                SUCCESS(AstType::AssignStat, postfix_exp, exp);
            } else ERROR("expected expression after '='");
        }
    }
});
DEFPARSER(block, {
    EXPECT('{')
    TRY(stat_list) {
        EXPECT('}')
        SUCCESS(stat_list)
    }
});
DEFPARSER(return_stat, {
    TRYs("return") {
        TRY(exp) {
            SUCCESS(AstType::ReturnStat, exp);
        }
        SUCCESS(AstType::ReturnStat);
    }
});

DEFPARSER(if_stat, {
    SUBPARSER(else_block, {
        EXPECT_WS("else")
        TRY(if_stat) {
            SUCCESS(if_stat);
        }
        TRY(block) { // TODO: can't parse 'else{' without the whitespace
            SUCCESS(block);
        }
    });

    EXPECT_WS("if")
    TRY2(exp, block) {
        TRY(else_block) {
            SUCCESS(AstType::IfStat, exp, block, else_block)
        }
        SUCCESS(AstType::IfStat, exp, block)
    }
});
DEFPARSER(while_stat, {
    EXPECT_WS("while");
    TRY2(exp, block) {
        SUCCESS(AstType::WhileStat, exp, block)
    }
});

DEFPARSER(for_stat, {
    // "for i in array/object/func { ... }"
    // "for i, v in object { ... }"
    // "for i in 1, 10 { ... }"

    EXPECT_WS("for");
    TRY(identifier) {
        auto i1 = identifier;
        auto i2 = AstNode{};
        auto has_i2 = false;

        // optional second identifier for object-style loop
        TRYs(',') {
            TRY(identifier) {
                i2 = identifier;
                has_i2 = true;
            } else ERROR("expected identifier after ','");
        }
        TRYs("in") {
            TRY(exp) {
                auto e1 = exp;
                auto e2 = AstNode{};
                auto has_e2 = false;
                TRYs(',') {
                    TRY(exp) {
                        has_e2 = true;
                        e2 = exp;
                    } else ERROR("expected expression after ',' after 'in'");
                }
                // exp must evaluate to array, object or function

                TRY(block) {
                    if (has_i2) {
                        if (has_e2) {
                            ERROR("2-ary for must only have one loop expression");
                        }
                        SUCCESS(AstType::ForStat2, i1, i2, exp, block)
                    } else {
                        if (has_e2) {
                            SUCCESS(AstType::ForStatInt, i1, e1, e2, block);
                        } else {
                            SUCCESS(AstType::ForStat, i1, exp, block);
                        }
                    }
                } else ERROR("expected block");
            } else ERROR("expected expression");
        } else ERROR("expected 'in' keyword");
    } else ERROR("expected identifier");
});

DEFPARSER(import_stat, {
    EXPECT_WS("import");
    TRY(identifier) {
        SUCCESS(AstType::ImportStat, identifier);
    }
});

DEFPARSER(stat_list, {
    out.type = AstType::StatList;
    out.line_number = _c.line_number;
    while (true) {
        auto n = AstNode {};
        if (0
            || parse_import_stat(code, n)
            || parse_const_decl_stat(code, n)
            || parse_var_decl_stat(code, n)
            || parse_func_decl_stat(code, n)
            || parse_assign_stat(code, n)
            || parse_if_stat(code, n)
            || parse_while_stat(code, n)
            || parse_for_stat(code, n)
            || parse_return_stat(code, n)
            || parse_exp(code, n)
            ) {
            out.children.emplace_back(n);
        } else {
            break;
        }
    }
    return true;
});
DEFPARSER(module, {
    auto res = parse_stat_list(code, out);
    if (res) {
        skip_whitespace(code);
    }
    return res;
});


bool Interpreter::parse(const std::string& code, AstNode& out_ast) {
    auto s_code = ParseContext(code, this);
    out_ast = AstNode { AstType::Unknown };
    auto res = parse_module(s_code, out_ast);
    if (!res) {
        error("parser error: "s + std::string(s_code.substr(0, 100)));
    }
    if (s_code.size()) {
        error("parser error: expected end of file: "s + std::string(s_code.substr(0, 100)));
    }
    return true;
}
