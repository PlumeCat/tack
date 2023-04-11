#pragma once

#include <jlib/hash_map.h>

#define ast_type() ast(Unknown)\
    ast(StatList)\
    \
    ast(ConstDeclStat)\
    ast(VarDeclStat)\
    ast(AssignStat)\
    ast(IfStat)\
    ast(WhileStat)\
    ast(ReturnStat)\
    \
    ast(TernaryExp)\
    \
    ast(OrExp) ast(AndExp)\
    ast(BitOrExp) ast(BitAndExp) ast(BitXorExp)\
    ast(EqExp) ast(NotEqExp) ast(LessExp) ast(GreaterExp) ast(LessEqExp) ast(GreaterEqExp)\
    ast(ShiftLeftExp) ast(ShiftRightExp)\
    ast(AddExp) ast(SubExp) ast(MulExp) ast(DivExp) ast(ModExp) ast(PowExp)\
    \
    ast(NegateExp) ast(NotExp) ast(BitNotExp) ast(LenExp)\
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
inline std::string to_string(AstType type) {
    static const hash_map<AstType, std::string> type_to_string = { ast_type() };
    return type_to_string[type];
}
#undef ast

struct AstNode {
    AstType type;
    std::vector<AstNode> children = {};
    std::string data_s = "";
    double data_d = 0.f;
    uint32_t line_number = 0;

    AstNode() = default;
    AstNode(const AstNode& node) = default;
    AstNode(AstType type): type(type) {}
    template<typename... Children> AstNode(AstType type, Children... ch)
        : type(type), children({ ch... }) {} // TODO: check forwarding
    AstNode(AstType type, const std::string& data_s)
        : type(type), data_s(data_s) {}
    AstNode(double d)
        : type(AstType::NumLiteral), data_d(d) {}

    std::string tostring(const std::string& indent = "") const {
        using namespace std; // TODO: remove at earliest opportunity
        auto s = indent + to_string(type) + 
            (type == AstType::Identifier ? " " + data_s : ""s) + 
            (type == AstType::NumLiteral ? " "s + std::to_string(data_d) : "") +
            ' ' + std::to_string(line_number);
        for (const auto& c : children) {
            s += "\n" + indent + c.tostring(indent + "  ");
        }
        return s;
    }
};

bool parse(const std::string& code, AstNode& out_ast);
