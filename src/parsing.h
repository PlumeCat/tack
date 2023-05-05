#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#define ast_type()\
    ast(Unknown)\
    \
    ast(StatList)\
    ast(ImportStat)\
    ast(ConstDeclStat)\
    ast(VarDeclStat)\
    ast(FuncDeclStat)\
    ast(AssignStat)\
    ast(IfStat)\
    ast(WhileStat)\
    ast(ForStat)\
    ast(ForStat2)\
    ast(ForStatInt)\
    ast(ReturnStat)\
    \
    ast(TernaryExp)\
    ast(Identifier)\
    \
    ast(OrExp)\
    ast(AndExp)\
    \
    ast(EqExp)\
    ast(NotEqExp)\
    ast(LessExp)\
    ast(GreaterExp)\
    ast(LessEqExp)\
    ast(GreaterEqExp)\
    \
    ast(BitOrExp)\
    ast(BitAndExp)\
    ast(BitXorExp)\
    ast(ShiftLeftExp)\
    ast(ShiftRightExp)\
    \
    ast(AddExp)\
    ast(SubExp)\
    ast(MulExp)\
    ast(DivExp)\
    ast(ModExp)\
    ast(PowExp)\
    \
    ast(NegateExp)\
    ast(NotExp)\
    ast(BitNotExp)\
    ast(LenExp)\
    \
    ast(NumLiteral)\
    ast(BoolLiteral)\
    ast(NullLiteral)\
    ast(StringLiteral)\
    ast(FuncLiteral)\
    ast(ParamDef)\
    ast(ArrayLiteral)\
    ast(ObjectLiteral)\
    \
    ast(CallExp)\
    ast(ArgList)\
    ast(IndexExp)\
    ast(AccessExp)\


#define ast(x) x,
enum class AstType { ast_type() };
#undef ast
#define ast(x) { AstType::x, #x },
inline std::string to_string(AstType type) {
    static const std::unordered_map<AstType, std::string> type_to_string = { ast_type() };
    return type_to_string.at(type);
}
#undef ast

struct AstNode {
    AstType type = AstType::Unknown;
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
    AstNode(bool b)
        : type(AstType::BoolLiteral), data_d(b) {}

    std::string tostring(const std::string& indent = "") const {
        auto s = indent + to_string(type) + 
            (type == AstType::Identifier ? " " + data_s : "") + 
            (type == AstType::NumLiteral ? " " + std::to_string(data_d) : "") +
            " (" + std::to_string(line_number) + ')';
        for (const auto& c : children) {
            s += "\n" + indent + c.tostring(indent + "  ");
        }
        return s;
    }
};

bool parse(const std::string& code, AstNode& out_ast);
