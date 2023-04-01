#include "parsing.h"

// parsing utils
using namespace std;

struct ParseContext : private string_view {
	uint32_t line_number = 1;
	ParseContext(const string& s) : string_view(s) {}

	void remove_prefix(string_view::size_type s) noexcept {
		for (auto i = 0; i < s; i++) {
			if ((*this)[i] == '\n') {
				line_number += 1;
			}
		}
		string_view::remove_prefix(s);
	}

	using string_view::operator[];
	using string_view::size;
	using string_view::data;
	using string_view::substr;
};

#define SUCCESS(...) { out = AstNode(__VA_ARGS__); out.line_number = _c.line_number; return true; }
#define FAIL(); { code = _c; return false; }
#define ERROR(msg) throw runtime_error("parsing error: " msg );// | line: " + to_string(code.line_number) + "\n'" + string(code.substr(0, 32))  + "... '")
#define EXPECT(s) if (!parse_raw_string(code, s)) { FAIL(); }

#define paste(a, b) a##b
#define DECLPARSER(name) bool paste(parse_, name)(ParseContext& code, AstNode& out)
#define DEFPARSER(name, body) DECLPARSER(name) { auto _c = code; body; FAIL(); }
#define SUBPARSER(name, body) auto paste(parse_, name) = [&](ParseContext& code, AstNode& out) -> bool { auto _c = code; body; FAIL(); };

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
void skip_whitespace(ParseContext& code) {
	auto n = 0;
	for (; n < code.size(); n++) {
		if (!isspace(code[n])) {
			break;
		}
	}
	code.remove_prefix(n);
}

bool parse_raw_number(ParseContext& code, double& out) {
	auto end = (char*)nullptr;
	auto result = strtod(code.data(), &end);
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
		for (auto n = 1; n < code.size(); n++) {
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
		for (auto n = 1; n < code.size(); n++) {
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
	SUBPARSER(object_literal, {
		TRYs('{') {
			auto res = AstNode(AstType::ObjectLiteral);

			while (true) {
				TRY(identifier) {
					TRYs('=') {
						TRY(exp) {
							res.children.emplace_back(AstType::AssignStat, identifier, exp);
							TRYs(',') {} else break;
						} else FAIL();
					} else FAIL();
				} else break;
			}

			EXPECT('}');
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
		});
		EXPECT("fn")
		TRY(identifier) {}
		TRYs('(') {} else ERROR("expected parameter definition after 'fn'");
		TRY(param_def) {
			TRYs(')') {} else ERROR("expected ')' after parameter definition");
			TRY(block) {
				if (identifier.type == AstType::Identifier) {
					SUCCESS(AstType::FuncLiteral, param_def, block, identifier);
				} else {
					SUCCESS(AstType::FuncLiteral, param_def, block);
				}
			} else ERROR("expected block after parameter definition");
		}
	});
	TRY(string_literal) SUCCESS(string_literal);
	TRY(array_literal) SUCCESS(array_literal);
	TRY(object_literal) SUCCESS(object_literal);
	TRY(func_literal) SUCCESS(func_literal);
	TRY(num_literal) SUCCESS(num_literal);
});
// TODO: remove
DEFPARSER(clock_exp, {
	EXPECT("clock") EXPECT('(') EXPECT(')')
	SUCCESS(AstType::ClockExp)
});
DEFPARSER(random_exp, {
	EXPECT("random") EXPECT('(') EXPECT(')')
	SUCCESS(AstType::RandomExp)
});
DEFPARSER(primary_exp, {
	TRY(random_exp) SUCCESS(random_exp)
	TRY(clock_exp) SUCCESS(clock_exp)
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

DEFPARSER(const_decl_stat, {
	EXPECT("const")
	TRY(identifier) {
		EXPECT('=')
		TRY(exp) { SUCCESS(AstType::ConstDeclStat, identifier, exp) }
	}
});
DEFPARSER(var_decl_stat, {
	EXPECT("let")
	TRY(identifier) {
		EXPECT('=')
		TRY(exp) {
			SUCCESS(AstType::VarDeclStat, identifier, exp)
		}
	}
});
DEFPARSER(assign_stat, {
	TRY(postfix_exp) {
		TRYs('=') {
			TRY(exp) {
				SUCCESS(AstType::AssignStat, postfix_exp, exp);
			} else throw runtime_error("expected expression after '='");
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

// TODO: remove
DEFPARSER(print_stat, {
	EXPECT("print")
	EXPECT('(')
	TRY(exp) {}
	EXPECT(')')
	SUCCESS(AstType::PrintStat, exp)
});
DEFPARSER(if_stat, {
	SUBPARSER(else_block, {
		EXPECT("else")
		TRY(if_stat) {
			SUCCESS(if_stat);
		}
		TRY(block) {
			SUCCESS(block);
		}
	});

	EXPECT("if")
	TRY2(exp, block) {
		TRY(else_block) {
			SUCCESS(AstType::IfStat, exp, block, else_block)
		}
		SUCCESS(AstType::IfStat, exp, block)
	}
	FAIL()
});
DEFPARSER(while_stat, {
	EXPECT("while");
	TRY2(exp, block) {
		SUCCESS(AstType::WhileStat, exp, block)
	}
	FAIL()
});
DEFPARSER(stat_list, {
	out.type = AstType::StatList;
	out.line_number = _c.line_number;
	while (true) {
		auto n = AstNode {};
		if (0
			|| parse_const_decl_stat(code, n)
			|| parse_var_decl_stat(code, n)
			|| parse_assign_stat(code, n)
			|| parse_if_stat(code, n)
			|| parse_while_stat(code, n)
			|| parse_print_stat(code, n)
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


bool parse(const std::string& code, AstNode& out_ast) {
	auto s_code = ParseContext(code);
	out_ast = AstNode { AstType::Unknown };
	auto res = parse_module(s_code, out_ast);
	if (!res) {
		// TODO: parse error
		throw runtime_error("parser error: "s + string(s_code.substr(0, 100)));
	}
	if (s_code.size()) {
		// TODO: expected end of file error
		throw runtime_error("expected end of file: "s + string(s_code.substr(0, 100)));
	}
	return true;
}