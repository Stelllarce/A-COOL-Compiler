#include "TreePrinter.h"
#include <iostream>
#include "CoolLexer.h"
#include "CoolParser.h"

using namespace std;

void TreePrinter::print_indent() { cout << string(indent_, ' '); }

TreePrinter::TreePrinter(CoolLexer *lexer, CoolParser *parser, const string &file_name)
    : lexer_(lexer), parser_(parser), file_name_(file_name) {}

void TreePrinter::print() { visitProgram(parser_->program()); }

std::any TreePrinter::visit(antlr4::tree::ParseTree *tree) { return tree->accept(this); }

std::any TreePrinter::visitProgram(CoolParser::ProgramContext *ctx) {
    cout << '#' << ctx->getStop()->getLine() << endl;
    // rules that are not labels support dynamic names
    cout << "_" << parser_->getRuleNames()[ctx->getRuleIndex()] << endl;
    indent_ += 2;
    visitChildren(ctx);

    indent_ -= 2;
    return std::any{};
}

std::any TreePrinter::visitClass(CoolParser::ClassContext *ctx) {
    print_indent();
    cout << '#' << ctx->getStop()->getLine() << endl;
    print_indent();
    // rules that are not labels support dynamic names
    cout << "_" << parser_->getRuleNames()[ctx->getRuleIndex()] << endl;
    indent_ += 2;

    print_indent();
    cout << ctx->TYPEID(0)->getText() << endl;

    print_indent();
    if (ctx->INHERITS()) {
        cout << ctx->TYPEID(1)->getText() << endl;
    } else {
        cout << "Object" << endl;
    }

    print_indent();
    cout << "\"" << file_name_ << "\"" << endl;
    print_indent();
    cout << "(" << endl;

    for (auto feature_ctx : ctx->feature()) {
        visit(feature_ctx);
    }

    print_indent();
    cout << ")" << endl;
    indent_ -= 2;
    return std::any{};
}

std::any TreePrinter::visitAttr(CoolParser::AttrContext *ctx) {
    print_indent();
    cout << '#' << ctx->getStart()->getLine() << endl;
    print_indent();
    // rules that are not labels support dynamic names
    cout << "_" << parser_->getRuleNames()[ctx->getRuleIndex()] << endl;
    indent_ += 2;

    print_indent();
    cout << ctx->OBJECTID()->getText() << endl;
    print_indent();
    cout << ctx->TYPEID()->getText() << endl;

    if (ctx->expr()) {
        visit(ctx->expr());
    } else {
        print_indent();
        cout << '#' << ctx->getStart()->getLine() << endl;
        print_indent();
        cout << "_no_expr" << endl;
        print_indent();
        cout << ": _no_type" << endl;
    }
    indent_ -= 2;
    return std::any{};
}

std::any TreePrinter::visitFormal(CoolParser::FormalContext *ctx) {
    print_indent();
    cout << '#' << ctx->getStart()->getLine() << endl;
    print_indent();
    // rules that are not labels support dynamic names
    cout << "_" << parser_->getRuleNames()[ctx->getRuleIndex()] << endl;
    indent_ += 2;

    print_indent();
    cout << ctx->OBJECTID()->getText() << endl;
    print_indent();
    cout << ctx->TYPEID()->getText() << endl;

    indent_ -= 2;
    return std::any{};
}

std::any TreePrinter::visitAssign(CoolParser::AssignContext *ctx) {
    print_indent();
    cout << '#' << ctx->getStart()->getLine() << endl;
    print_indent();
    cout << "_assign" << endl;
    indent_ += 2;

    print_indent();
    cout << ctx->OBJECTID()->getText() << endl;

    visit(ctx->expr());

    indent_ -= 2;
    print_indent();
    cout << ": _no_type" << endl;
    return std::any{};
}

std::any TreePrinter::visitMethod(CoolParser::MethodContext *ctx) {
    print_indent();
    cout << '#' << ctx->getStop()->getLine() << endl;
    print_indent();
    // rules that are not labels support dynamic names
    cout << "_" << parser_->getRuleNames()[ctx->getRuleIndex()] << endl;
    indent_ += 2;

    print_indent();
    cout << ctx->OBJECTID()->getText() << endl;

    for (auto formal_ctx : ctx->formal()) {
        visit(formal_ctx);
    }

    print_indent();
    cout << ctx->TYPEID()->getText() << endl;

    visit(ctx->expr());

    indent_ -= 2;
    return std::any{};
}

std::any TreePrinter::visitObject(CoolParser::ObjectContext *ctx) {
    print_indent();
    cout << '#' << ctx->getStart()->getLine() << endl;
    print_indent();
    cout << "_object" << endl;
    indent_ += 2;
    print_indent();
    cout << ctx->OBJECTID()->getText() << endl;
    indent_ -= 2;
    print_indent();
    cout << ": _no_type" << endl;
    return std::any{};
}

std::any TreePrinter::visitInt(CoolParser::IntContext *ctx) {
    print_indent();
    cout << '#' << ctx->getStart()->getLine() << endl;
    print_indent();
    cout << "_int" << endl;
    indent_ += 2;
    print_indent();
    cout << ctx->INT_CONST()->getText() << endl;
    indent_ -= 2;
    print_indent();
    cout << ": _no_type" << endl;
    return std::any{};
}

std::any TreePrinter::visitString(CoolParser::StringContext *ctx) {
    print_indent();
    cout << '#' << ctx->getStart()->getLine() << endl;
    print_indent();
    cout << "_string" << endl;
    indent_ += 2;
    print_indent();
    cout << "\"" << lexer_->get_string_value(ctx->STR_CONST()->getSymbol()->getStartIndex()) << "\"" << endl;
    indent_ -= 2;
    print_indent();
    cout << ": _no_type" << endl;
    return std::any{};
}

std::any TreePrinter::visitBool(CoolParser::BoolContext *ctx) {
    print_indent();
    cout << '#' << ctx->getStart()->getLine() << endl;
    print_indent();
    cout << "_bool" << endl;
    indent_ += 2;
    print_indent();
    cout << (lexer_->get_bool_value(ctx->BOOL_CONST()->getSymbol()->getStartIndex()) ? "1" : "0") << endl;
    indent_ -= 2;
    print_indent();
    cout << ": _no_type" << endl;
    return std::any{};
}

std::any TreePrinter::visitMultdiv(CoolParser::MultdivContext *ctx) {
    string opName = ctx->MULT() ? "_mul" : "_divide";
    return visitBinaryOp(ctx, opName);
}

std::any TreePrinter::visitSubadd(CoolParser::SubaddContext *ctx) {
    string opName = ctx->PLUS() ? "_plus" : "_sub";
    return visitBinaryOp(ctx, opName);
}

std::any TreePrinter::visitComp(CoolParser::CompContext *ctx) {
    string opName;
    if (ctx->LT())
        opName = "_lt";
    else if (ctx->LE())
        opName = "_leq";
    else
        opName = "_eq";
    return visitBinaryOp(ctx, opName);
}

std::any TreePrinter::visitNeg(CoolParser::NegContext *ctx) { return visitUnaryOp(ctx, "_neg"); }

std::any TreePrinter::visitIsvoid(CoolParser::IsvoidContext *ctx) { return visitUnaryOp(ctx, "_isvoid"); }

std::any TreePrinter::visitNot(CoolParser::NotContext *ctx) { return visitUnaryOp(ctx, "_comp"); }

std::any TreePrinter::visitParen(CoolParser::ParenContext *ctx) {
    return visit(ctx->expr());
}

std::any TreePrinter::visitStatdispatch(CoolParser::StatdispatchContext *ctx) {
    print_indent();
    cout << '#' << ctx->getStart()->getLine() << endl;
    print_indent();
    cout << "_static_dispatch" << endl;
    indent_ += 2;

    visit(ctx->expr(0));

    if (ctx->AT()) {
        print_indent();
        cout << ctx->TYPEID()->getText() << endl;
    }

    print_indent();
    cout << ctx->OBJECTID()->getText() << endl;

    print_indent();
    cout << "(" << endl;
    for (size_t i = 1; i < ctx->expr().size(); ++i) {
        visit(ctx->expr(i));
    }
    print_indent();
    cout << ")" << endl;

    indent_ -= 2;
    print_indent();
    cout << ": _no_type" << endl;
    return std::any{};
}

std::any TreePrinter::visitDispatch(CoolParser::DispatchContext *ctx) {
    print_indent();
    cout << '#' << ctx->getStart()->getLine() << endl;
    print_indent();
    cout << "_dispatch" << endl;
    indent_ += 2;

    visit(ctx->expr(0));

    print_indent();
    cout << ctx->OBJECTID()->getText() << endl;

    print_indent();
    cout << "(" << endl;
    for (size_t i = 1; i < ctx->expr().size(); ++i) {
        visit(ctx->expr(i));
    }
    print_indent();
    cout << ")" << endl;

    indent_ -= 2;
    print_indent();
    cout << ": _no_type" << endl;
    return std::any{};
}

std::any TreePrinter::visitSelfdispatch(CoolParser::SelfdispatchContext *ctx) {
    print_indent();
    cout << '#' << ctx->getStart()->getLine() << endl;
    print_indent();
    cout << "_dispatch" << endl;
    indent_ += 2;

    print_indent();
    cout << '#' << ctx->getStart()->getLine() << endl;
    print_indent();
    cout << "_object" << endl;
    indent_ += 2;
    print_indent();
    cout << "self" << endl;
    indent_ -= 2;
    print_indent();
    cout << ": _no_type" << endl;

    print_indent();
    cout << ctx->OBJECTID()->getText() << endl;

    print_indent();
    cout << "(" << endl;
    for (auto expr_ctx : ctx->expr()) {
        visit(expr_ctx);
    }
    print_indent();
    cout << ")" << endl;

    indent_ -= 2;
    print_indent();
    cout << ": _no_type" << endl;
    return std::any{};
}

// ------------------ maybe these two can be generalized ------------

std::any TreePrinter::visitCond(CoolParser::CondContext *ctx) {
    print_indent();
    cout << '#' << ctx->getStop()->getLine() << endl;
    print_indent();
    cout << "_cond" << endl;
    indent_ += 2;
    
    // visit the condition, then and else expressions
    visit(ctx->expr(0));
    visit(ctx->expr(1));
    visit(ctx->expr(2));

    indent_ -= 2;
    print_indent();
    cout << ": _no_type" << endl;
    return std::any{};
}

std::any TreePrinter::visitLoop(CoolParser::LoopContext *ctx) {
    print_indent();
    cout << '#' << ctx->getStop()->getLine() << endl;
    print_indent();
    cout << "_loop" << endl;
    indent_ += 2;
    
    // visit the condition and body expressions
    visit(ctx->expr(0));
    visit(ctx->expr(1));

    indent_ -= 2;
    print_indent();
    cout << ": _no_type" << endl;
    return std::any{};
}

// ------------------------------------------------------------------

std::any TreePrinter::visitBlock(CoolParser::BlockContext *ctx) {
    print_indent();
    cout << '#' << ctx->getStop()->getLine() << endl;
    print_indent();
    cout << "_block" << endl;
    indent_ += 2;

    for (auto expr_ctx : ctx->expr()) {
        visit(expr_ctx);
    }

    indent_ -= 2;
    print_indent();
    cout << ": _no_type" << endl;
    return std::any{};
}

void TreePrinter::visitLetRecursion(const vector<CoolParser::Let_bindingContext *> &bindings, size_t index, CoolParser::ExprContext *body, size_t let_index) {
    // get the current binding
    auto binding_ctx = bindings[index];

    print_indent();
    cout << '#' << let_index << endl;
    print_indent();
    cout << "_let" << endl;
    indent_ += 2;

    // printing current binding info
    print_indent();
    cout << binding_ctx->OBJECTID()->getText() << endl;
    print_indent();
    cout << binding_ctx->TYPEID()->getText() << endl;

    // visit the initialization expression if exists
    if (binding_ctx->expr()) {
        visit(binding_ctx->expr());
    // no initialization expression
    } else {
        print_indent();
        cout << '#' << binding_ctx->OBJECTID()->getSymbol()->getLine() << endl;
        print_indent();
        cout << "_no_expr" << endl;
        print_indent();
        cout << ": _no_type" << endl;
    }

    // if there are more bindings, recurse
    if (index + 1 < bindings.size()) {
        visitLetRecursion(bindings, index + 1, body, let_index);
    // else visit the body and end recursion
    } else {
        visit(body);
    }

    indent_ -= 2;
    print_indent();
    cout << ": _no_type" << endl;
}

std::any TreePrinter::visitLet(CoolParser::LetContext *ctx) {
    auto bindings = ctx->let_binding();
    auto body = ctx->expr();
    /**
     * @note: seems like a patchwork, revisit later
     * @note could not find a better solution, fix later
     * 
     * @brief get the line of the let, as it
     * should be the same line for all bindings, as per the tests
     */
    auto let_index = ctx->getStop()->getLine();

    // start the recursion to visit bindings
    visitLetRecursion(bindings, 0, body, let_index);
    return std::any{};
}

std::any TreePrinter::visitCase(CoolParser::CaseContext *ctx) {
    print_indent();
    cout << '#' << ctx->getStop()->getLine() << endl;
    print_indent();
    cout << "_typcase" << endl;
    indent_ += 2;

    // visit the case variable
    visit(ctx->expr(0));

    // visit each branch
    for (size_t i = 0; i < ctx->OBJECTID().size(); ++i) {
        print_indent();
        cout << '#' << ctx->OBJECTID(i)->getSymbol()->getLine() << endl;
        print_indent();
        cout << "_branch" << endl;
        indent_ += 2;

        print_indent();
        cout << ctx->OBJECTID(i)->getText() << endl;
        print_indent();
        cout << ctx->TYPEID(i)->getText() << endl;

        // visit branch expression
        visit(ctx->expr(i + 1));

        indent_ -= 2;
    }

    indent_ -= 2;
    print_indent();
    cout << ": _no_type" << endl;
    return std::any{};
}

std::any TreePrinter::visitNew(CoolParser::NewContext *ctx) {
    print_indent();
    cout << '#' << ctx->getStart()->getLine() << endl;
    print_indent();
    cout << "_new" << endl;
    indent_ += 2;

    print_indent();
    cout << ctx->TYPEID()->getText() << endl;

    indent_ -= 2;
    print_indent();
    cout << ": _no_type" << endl;
    return std::any{};
}

std::any TreePrinter::visitChildren(antlr4::tree::ParseTree *node) {
    for (auto child : node->children) {
        child->accept(this);
    }
    return std::any{};
}
