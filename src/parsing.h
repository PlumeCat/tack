#pragma once

#include "parser_lib.h"

using namespace std;

#define ast_type() ast(Unknown)\
    ast(StatList)\
    \
    ast(VarDeclStat)\
    ast(AssignStat)\
    ast(PrintStat)\
    ast(IfStat)\
    ast(WhileStat)\
    ast(ReturnStat)\
    \
    ast(TernaryExp)\
    ast(OrExp) ast(AndExp)\
    ast(BitOrExp) ast(BitAndExp) ast(BitXorExp)\
    ast(EqExp) ast(NotEqExp) ast(LessExp) ast(GreaterExp) ast(LessEqExp) ast(GreaterEqExp)\
    ast(ShiftLeftExp) ast(ShiftRightExp)\
    ast(AddExp) ast(SubExp) ast(MulExp) ast(DivExp) ast(ModExp) ast(PowExp)\
    ast(NegateExp) ast(NotExp) ast(BitNotExp)\
    \
    ast(CallExp) ast(ArgList)\
    ast(IndexExp)\
    ast(AccessExp)\
    \
    ast(FuncLiteral)\
    ast(ParamDef)\
    ast(NumLiteral) ast(StringLiteral) ast(ArrayLiteral) ast(ObjectLiteral)\
    ast(Identifier)\


#define ast(x) x,
enum class AstType { ast_type() };
#undef ast
#define ast(x) { AstType::x, #x },
static const unordered_map<AstType, string> type_to_string = { ast_type() };
#undef ast

struct AstNode {
    AstType type;
    vector<AstNode> children = {};
    string data_s = ""s; // string literal, identifier
    double data_d = 0.f; // num literal

    AstNode() = default;
    AstNode(const AstNode& node) = default;
    template<typename... Children> AstNode(AstType type, Children... ch) : type(type), children({ ch... }) {} // TODO: check forwarding
    AstNode(AstType type, const string& data_s): type(type), data_s(data_s) {}
    AstNode(double d): type(AstType::NumLiteral), data_d(d) {}

    string tostring(const string& indent = "") const {
        auto s = indent + type_to_string.at(type) + (type == AstType::Identifier ? " " + data_s : ""s) + (type == AstType::NumLiteral ? " "s + to_string(data_d) : "");
        for (const auto& c : children) {
            s += "\n" + indent + c.tostring(indent + "  ");
        }
        return s;
    }
};


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
DEFPARSER(literal, {
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
    SUBPARSER(func_literal, {
        SUBPARSER(param_def, {
            auto p = AstNode(AstType::ParamDef);
            while (true) {
                TRY(identifier) {
                    p.children.emplace_back(identifier);
                    TRYs(',') {} else break; // TODO: disallow trailing comma
                } else break;
            }
            SUCCESS(p);
        })
        EXPECT("fn")
        // TRY(identifier) {} // TODO: named vs anon functions (a named function is a VarDeclStat, so can't be an expression?)
        TRYs('(') {} else throw runtime_error("expected parameter definition after 'fn'");
        TRY(param_def) {
            TRYs(')') {} else throw runtime_error("expected ')' after parameter definition");
            TRY(block) {
                SUCCESS(AstType::FuncLiteral, param_def, block)
            } else throw runtime_error("expected block after parameter definition");
        }
    });
    TRY(string_literal) SUCCESS(string_literal);
    TRY(num_literal) SUCCESS(num_literal);
    TRY(func_literal) SUCCESS(func_literal);
})
DEFPARSER(primary_exp, {
    TRY(literal) SUCCESS(literal)
    TRY(identifier) SUCCESS(identifier)
    TRYs('(') {
        TRY(exp) {
            TRYs(')') {} else throw runtime_error("expected closing ')'");
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
            } else throw runtime_error("expected identifier after '.'");
        }
    });
    SUBPARSER(index_postfix, {
        // index expr
        TRYs('[') {
            TRY(exp) {
                TRYs(']') {
                    SUCCESS(exp)
                } else throw runtime_error("expected ']'");
            } else throw runtime_error("expected expression in square brackets");
        }
    });
    SUBPARSER(arg_list, {
        auto res = AstNode(AstType::ArgList);

        TRY(exp) {
            res.children.emplace_back(exp);
        } else SUCCESS(res)

        while (true) {
            TRYs(',') {
                TRY(exp) {
                    res.children.emplace_back(exp);
                } else throw runtime_error("unexpected symbol in argument list");
            } else break;
        }
        SUCCESS(res)
    });
    SUBPARSER(call_postfix, {
        TRYs('(') {
            TRY(arg_list) {
                TRYs(')') {
                    SUCCESS(arg_list)
                } else throw runtime_error("expected ')' after argument list");
            } else throw runtime_error("expected ')' or argument list after '('");
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
    }
    TRY(postfix_exp) SUCCESS(postfix_exp)
});
DEFPARSER(unary_exp, {
    TRY(prefix_exp) SUCCESS(prefix_exp)
});

BINOP(pow_exp, PowExp, unary_exp, "**");
BINOP(mod_exp, ModExp, pow_exp, '%');
BINOP(div_exp, DivExp, mod_exp, '/');
BINOP(mul_exp, MulExp, div_exp, '*');
BINOP(sub_exp, SubExp, mul_exp, '-');
BINOP(add_exp, AddExp, sub_exp, '+');
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
})

BINOP(bit_and_exp, BitAndExp, cmp_exp, '&');
BINOP(bit_xor_exp, BitXorExp, bit_and_exp, '^');
BINOP(bit_or_exp, BitOrExp, bit_xor_exp, '|');
BINOP(and_exp, AndExp, bit_or_exp, "&&");
BINOP(or_exp, OrExp, and_exp, "||");

DEFPARSER(ternary_exp, {
    TRY(or_exp) {
        TRYs('?') {
            TRY(exp) {
                auto e1 = exp;
                TRYs(':') {} else throw runtime_error("Expected ':' in ternary expression");
                TRY(exp) {} else throw runtime_error("Expected expression after ':'");
                SUCCESS(AstType::TernaryExp, or_exp, e1, exp);
            } else throw runtime_error("Expected expression after '?'");
        }
        SUCCESS(or_exp);
    }
});

DEFPARSER(exp, { TRY(ternary_exp) SUCCESS(ternary_exp); });

DEFPARSER(var_decl_stat, {
    EXPECT("let")
    TRY(identifier) {
        EXPECT('=')
        TRY(exp) {
            SUCCESS(AstType::VarDeclStat, identifier, exp)
        }
    }
})
DEFPARSER(assign_stat, {
    TRY(postfix_exp) {
        TRYs('=') {
            TRY(exp) {
                SUCCESS(AstType::AssignStat, postfix_exp, exp);
            } else throw runtime_error("expected expression after '='");
        }
    }
})
DEFPARSER(block, {
    EXPECT('{')
    TRY(stat_list) {
        EXPECT('}')
        SUCCESS(stat_list)
    }
    })
DEFPARSER(return_stat, {
    EXPECT("return")
    TRY(exp) {
        SUCCESS(AstType::ReturnStat, exp);
    }
    SUCCESS(AstType::ReturnStat);
});

// TODO: remove
DEFPARSER(print_stat, {
    EXPECT("print")
    EXPECT('(')
    TRY(exp) {}
    EXPECT(')')
    SUCCESS(AstType::PrintStat, exp)
})
DEFPARSER(if_stat, {
    SUBPARSER(else_block, {
        EXPECT("else")
        TRY(if_stat) {
            SUCCESS(if_stat);
        }
        TRY(block) {
            SUCCESS(block);
        }
    })

    EXPECT("if")
    TRY2(exp, block) {
        TRY(else_block) {
            SUCCESS(AstType::IfStat, exp, block, else_block)
        }
        SUCCESS(AstType::IfStat, exp, block)
    }
    FAIL()
})
DEFPARSER(while_stat, {
    EXPECT("while");
    TRY2(exp, block) {
        SUCCESS(AstType::WhileStat, exp, block)
    }
    FAIL()
})
DEFPARSER(stat_list, {
    out.type = AstType::StatList;
    while (true) {
        auto n = AstNode {};
        if (0
            ||parse_var_decl_stat(code, n)
            ||parse_assign_stat(code, n)
            ||parse_if_stat(code, n)
            ||parse_while_stat(code, n)
            ||parse_print_stat(code, n)
            ||parse_return_stat(code, n)
            ||parse_exp(code, n)
            ) {
            out.children.emplace_back(n);
        } else {
            break;
        }
    }
    return true;
})
DEFPARSER(module, {
    auto res = parse_stat_list(code, out);
    if (res) {
        skip_whitespace(code);
    }
    return res;
})


bool parse(const string& code, AstNode& out_ast) {
    auto s_code = string_view { code };
    out_ast = AstNode { AstType::Unknown };
    auto res = parse_module(s_code, out_ast);
    if (!res) {
        // TODO: parse error
        throw runtime_error("parser error: " + string(s_code.substr(0, 100)));
    }
    if (s_code.size()) {
        // TODO: expected end of file error
        throw runtime_error("expected end of file: " + string(s_code.substr(0, 100)));
    }
    return true;
}