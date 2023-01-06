#pragma once

#include <tuple>
#include <string>
#include <string_view>

using namespace std;


bool is_identifier_char(char c) {
	return isalnum(c) || c == '_';
}
bool is_identifier_start_char(char c) {
	return isalpha(c) || c == '_';
}
void skip_whitespace(string_view& code) {
	auto n = 0;
	for (; n < code.size(); n++) {
		if (!isspace(code[n])) {
			break;
		}
	}
	code.remove_prefix(n);
}

bool parse_raw_number(string_view& code, double& out) {
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
bool parse_raw_string_literal(string_view& code, string& out) {
	skip_whitespace(code);
	if (code.size() && code[0] == '"') {
		for (auto n = 1; n < code.size(); n++) {
			if (code[n] == '"') {
				out = code.substr(1, n - 1);
				code.remove_prefix(n + 1);
				return true;
			}
		}
		throw runtime_error("Expected closing quote '\"'");
	}
	return false;
}
bool parse_raw_identifier(string_view& code, string& out) {
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
bool parse_raw_string(string_view& code, char c) {
	skip_whitespace(code);
	if (code.size() && code[0] == c) {
		code.remove_prefix(1);
		return true;
	}
	return false;
}
bool parse_raw_string(string_view& code, const string& c) {
	skip_whitespace(code);
	if (code.size() >= c.size() && code.substr(0, c.size()) == c) {
		code.remove_prefix(c.size());
		return true;
	}
	return false;
}



#define SUCCESS(...) { out = AstNode(__VA_ARGS__); return true; }
#define FAIL(); { code = _c; return false; }

#define paste(a, b) a##b
#define DECLPARSER(name) bool paste(parse_, name)(string_view& code, AstNode& out)
#define DEFPARSER(name, body) DECLPARSER(name) { auto _c = code; body; FAIL(); }
#define SUBPARSER(name, body) auto paste(parse_, name) = [&](string_view& code, AstNode& out) -> bool { auto _c = code; body; FAIL(); };

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


#define EXPECT(s) if (!parse_raw_string(code, s)) { FAIL(); }

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
					} else throw runtime_error("foo bar baz");\
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



//using ParseResult = pair<bool, size_t>;
//using ParseFunc = ParseResult(*)(string_view);
//#define fail() return { false, 0 };
//#define success(n) return { true, n };
//
//template<typename Derived>
//struct Parser {
//	string label;
//	virtual Derived& set_label(const string& l) final { label = l; return (Derived&)(*this); }
//	virtual ParseResult parse(string_view code) const = 0;
//};
//
//// COMBINATORS
//template<typename Arg = void> struct GenericParser : public Parser<GenericParser<Arg>> {
//	using FuncType = ParseResult(*)(string_view, const Arg&);
//
//	FuncType func;
//	Arg arg;
//
//	GenericParser(FuncType f, const Arg& a) : func(f), arg(a) {}
//	virtual ParseResult parse(string_view code) const override {
//		return func(code, arg);
//	}
//};
//template<> struct GenericParser<void> : public Parser<GenericParser<void>> {
//	ParseFunc func;
//	GenericParser(ParseFunc f) : func(f) {}
//	virtual ParseResult parse(string_view code) const override {
//		return func(code);
//	}
//};
//
//GenericParser<string> KeywordParser(const string& s) {
//	return GenericParser<string>(parse_string, s);
//}
//GenericParser<char> CharParser(char c) {
//	return GenericParser<char>(parse_char, c);
//}
//
//
//// TODO: use sfinae or similar to enforce that all template arguments are subclass of Parser
//
//template<typename LHS, typename RHS>
//struct OrParser : public Parser<OrParser<LHS, RHS>> {
//	LHS left;
//	RHS right;
//
//	OrParser(const LHS& l, const RHS& r) : left(l), right(r) {}
//	virtual ParseResult parse(string_view code) const override {
//		if (auto l = left.parse(code); l.first) {
//			success(l.second);
//		}
//		if (auto r = right.parse(code); r.first) {
//			success(r.second);
//		}
//		fail();
//	}
//};
//
//template<typename LHS, typename RHS>
//struct AndParser : public Parser<AndParser<LHS, RHS>> {
//	LHS left;
//	RHS right;
//
//	AndParser(const LHS& l, const RHS& r) : left(l), right(r) {}
//	virtual ParseResult parse(string_view code) const override {
//		auto l = left.parse(code);
//		if (l.first) {
//			auto r = right.parse(code.substr(l.second));
//			if (r.first) {
//				success(l.second + r.second);
//			}
//		}
//		fail();
//	}
//
//};
//
//// at-least-one repeated element
//template<typename Subparser>
//struct RepeatParser : public Parser<RepeatParser<Subparser>> {
//	Subparser subparser;
//
//	RepeatParser(const Subparser& sp) : subparser(sp) {}
//	virtual ParseResult parse(string_view code) const override {
//		auto result = false;
//		auto pos = 0;
//		while (true) {
//			auto res = subparser.parse(code.substr(pos));
//			if (res.first) {
//				result = true;
//				pos += res.second;
//			} else {
//				break;
//			}
//		}
//		return { result, pos };
//	}
//};
//
//template<typename... Args>
//struct SequenceParser : public Parser<SequenceParser<Args...>> {
//	using Subparsers = tuple<const Args...>;
//	static constexpr size_t Count = tuple_size<Subparsers>::value;
//
//	Subparsers subparsers;
//	SequenceParser(const Args... sp) : subparsers(sp...) {}
//
//	virtual ParseResult parse(string_view code) const override {
//		return subparse<0>(code);
//	}
//	template<size_t N> ParseResult subparse(string_view code, size_t offset = 0) const {
//		auto res = get<N>(subparsers).parse(code.substr(offset));
//		if (!res.first) {
//			fail();
//		}
//
//		if constexpr (N == Count - 1) {
//			success(offset + res.second);
//		} else {
//			return subparse<N + 1>(code, res.second);
//		}
//	}
//};
//
//template<typename Subparser>
//struct Optional : public Parser<Optional<Subparser>> {
//	Subparser subparser;
//	Optional(const Subparser& sp) : subparser(sp) {}
//	virtual ParseResult parse(string_view code) const override {
//		if (auto r = subparser.parse(code); r.first) {
//			success(r.second);
//		}
//		success(0);
//	}
//};
//
//
//#include <type_traits>
//template<typename LHS, typename RHS, typename = enable_if_t<is_base_of_v<Parser<LHS>, LHS> && is_base_of_v<Parser<RHS>, RHS>>>
//auto operator+(const LHS& l, const RHS& r) { return AndParser<LHS, RHS>(l, r); }
//template<typename LHS, typename = enable_if_t<is_base_of_v<Parser<LHS>, LHS>>>
//auto operator+(const LHS& l, char c) { return AndParser(l, CharParser(c)); }
//template<typename RHS, typename = enable_if_t<is_base_of_v<Parser<RHS>, RHS>>>
//auto operator+(char c, const RHS& r) { return AndParser(CharParser(c), r); }
//template<typename LHS, typename = enable_if_t<is_base_of_v<Parser<LHS>, LHS>>>
//auto operator+(const LHS& l, const string& s) { return AndParser(l, KeywordParser(s)); }
//template<typename RHS, typename = enable_if_t<is_base_of_v<Parser<RHS>, RHS>>>
//auto operator+(const string& s, const RHS& r) { return AndParser(KeywordParser(s), r); }
//// TODO: And      + RHS => Sequence(And.left, And.right, RHS)
////	   : Sequence + RHS => Sequence(..., RHS) probably more efficient than And(And(And(...
//
//template<typename LHS, typename RHS, typename = enable_if_t<is_base_of_v<Parser<LHS>, LHS> && is_base_of_v<Parser<RHS>, RHS>>>
//auto operator|(const LHS& l, const RHS& r) { return OrParser<LHS, RHS>(l, r); }
//
//template<typename LHS, typename = enable_if_t<is_base_of_v<Parser<LHS>, LHS>>>
//auto operator|(const LHS& l, char c) { return OrParser(l, CharParser(c)); }
//template<typename RHS, typename = enable_if_t<is_base_of_v<Parser<RHS>, RHS>>>
//auto operator|(char c, const RHS& r) { return OrParser(CharParser(c), r); }
//template<typename LHS, typename = enable_if_t<is_base_of_v<Parser<LHS>, LHS>>>
//auto operator|(const LHS& l, const string& s) { return OrParser(l, KeywordParser(s)); }
//template<typename RHS, typename = enable_if_t<is_base_of_v<Parser<RHS>, RHS>>>
//auto operator|(const string& s, const RHS& r) { return OrParser(KeywordParser(s), r); }






	//auto WS = GenericParser<void>(parse_whitespace);
	//auto Identifier = WS + GenericParser<void>(parse_identifier) + WS;
	//auto NumLiteral = WS + GenericParser<void>(parse_number) + WS;
	//auto StringLiteral = WS + GenericParser<void>(parse_string_literal) + WS;

	//Identifier.set_label("Identifier");
	//NumLiteral.set_label("NumLiteral");
	//StringLiteral.set_label("StringLiteral");
	//
	//auto PrimaryExp
	//	= Identifier
	//	| NumLiteral
	//	| StringLiteral
	//	| SequenceParser(CharParser('('), CharParser(')')).set_label("PrimaryExp") // TODO: '(' + Exp + ')'
	//	;

	//// TODO: second SubExp should be AddExp recursively
	//auto SubExp = PrimaryExp +		Optional('-' + PrimaryExp);
	//auto AddExp = SubExp + Optional('+' + SubExp);

	//auto& Exp = AddExp;

	//auto VarDeclStat = SequenceParser(KeywordParser("let"), Identifier, CharParser('='), Exp);
	//auto AssignStat = SequenceParser(Identifier, CharParser('='), Exp);
	//auto Block = CharParser('{') + '}'; // TODO:  + StatList + ...
	//auto IfStat = KeywordParser("if") + Exp + Block;
	//auto WhileStat = "while" + Exp + Block;
	//
	//auto PrintStat = KeywordParser("print") + CharParser('(') + Exp + CharParser(')');
	//PrintStat.set_label("print");

	//auto Statement
	//	= VarDeclStat
	//	| AssignStat
	//	| IfStat
	//	| PrintStat
	//	| WhileStat
	//	;

	// auto Subroutine = RepeatParser(Statement);
	// auto result = Subroutine.parse(string_view{ source });
	// auto result = PrintStat.parse(string_view{ source });
	/*if (result.first) {
		if (result.second == source.size()) {
			log("Parsing done!");
		} else {
			log("Parsing incomplete");
		}
	} else {
		log("Failed to parse");
	}
	*/