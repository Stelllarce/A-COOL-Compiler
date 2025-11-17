#pragma once

#include "CoolParserBaseVisitor.h"
#include "antlr4-runtime/antlr4-runtime.h"

#include "CoolLexer.h"
#include "CoolParser.h"
#include "CoolParserBaseVisitor.h"
#include <string>
#include <any>

using namespace std;
using namespace antlr4;
using namespace antlr4::tree;

class TreePrinter : public CoolParserBaseVisitor {
private:
    CoolLexer *lexer_;
    CoolParser *parser_;
    std::string file_name_;
    int indent_ = 0;

    void print_indent();

public:
    TreePrinter(CoolLexer *lexer, CoolParser *parser, const std::string &file_name);

    void print();

    /**
     * @brief Visit parse tree and print in indented format
     * Overall calls accept on the tree
     */
    std::any visit(antlr4::tree::ParseTree *tree) override;
    // ---------------- Overriden visit methods for diffrent printing of each rule ----------------
    std::any visitProgram(CoolParser::ProgramContext *ctx) override;
    std::any visitClass(CoolParser::ClassContext *ctx) override;
    std::any visitAttr(CoolParser::AttrContext *ctx) override;
    std::any visitFormal(CoolParser::FormalContext *ctx) override;
    std::any visitAssign(CoolParser::AssignContext *ctx) override;
    std::any visitMethod(CoolParser::MethodContext *ctx) override;
    std::any visitObject(CoolParser::ObjectContext *ctx) override;
    std::any visitInt(CoolParser::IntContext *ctx) override;
    std::any visitString(CoolParser::StringContext *ctx) override;
    std::any visitBool(CoolParser::BoolContext *ctx) override;
    std::any visitMultdiv(CoolParser::MultdivContext *ctx) override;
    std::any visitSubadd(CoolParser::SubaddContext *ctx) override;
    std::any visitComp(CoolParser::CompContext *ctx) override;
    std::any visitNeg(CoolParser::NegContext *ctx) override;
    std::any visitIsvoid(CoolParser::IsvoidContext *ctx) override;
    std::any visitNot(CoolParser::NotContext *ctx) override;
    std::any visitParen(CoolParser::ParenContext *ctx) override;
    std::any visitStatdispatch(CoolParser::StatdispatchContext *ctx) override;
    std::any visitDispatch(CoolParser::DispatchContext *ctx) override;
    std::any visitSelfdispatch(CoolParser::SelfdispatchContext *ctx) override;
    std::any visitCond(CoolParser::CondContext *ctx) override;
    std::any visitLoop(CoolParser::LoopContext *ctx) override;
    std::any visitBlock(CoolParser::BlockContext *ctx) override;
    std::any visitLet(CoolParser::LetContext *ctx) override;
    std::any visitCase(CoolParser::CaseContext *ctx) override;
    std::any visitNew(CoolParser::NewContext *ctx) override;
    std::any visitChildren(antlr4::tree::ParseTree *node) override;

private:
    // Helper methods for common patterns
    template <typename T>
    std::any visitBinaryOp(T *ctx, const string &opName) {
        print_indent();
        cout << '#' << ctx->getStart()->getLine() << endl;
        print_indent();
        cout << opName << endl;
        indent_ += 2;
        visit(ctx->expr(0));
        visit(ctx->expr(1));
        indent_ -= 2;
        print_indent();
        cout << ": _no_type" << endl;
        return std::any{};
    }
    
    template <typename T>
    std::any visitUnaryOp(T *ctx, const string &opName) {
        print_indent();
        cout << '#' << ctx->getStart()->getLine() << endl;
        print_indent();
        cout << opName << endl;
        indent_ += 2;
        visit(ctx->expr());
        indent_ -= 2;
        print_indent();
        cout << ": _no_type" << endl;
        return std::any{};
    }

    /**
     * @brief Helper method to recursively visit let bindings
     * @param bindings Vector of let binding contexts (let_binding grammar rule)
     * @param index Current index in the bindings vector
     * @param body The body of the let
     * @param let_index The line number of the let expression for printing
     */
    void visitLetRecursion(const std::vector<CoolParser::Let_bindingContext *> &bindings, size_t index, CoolParser::ExprContext *body, size_t let_index);
};
