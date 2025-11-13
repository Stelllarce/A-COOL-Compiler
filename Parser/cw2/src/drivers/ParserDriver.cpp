#include <cctype>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "antlr4-runtime/antlr4-runtime.h"

#include "CoolLexer.h"
#include "CoolParser.h"
#include "CoolParserBaseVisitor.h"

using namespace std;
using namespace antlr4;
using namespace antlr4::tree;

namespace fs = filesystem;

/// Converts token to coursework-specific string representation.
string cool_token_to_string(CoolLexer *lexer, Token *token) {
    auto token_type = token->getType();

    // clang-format off
    switch (token_type) {
        case static_cast<size_t>(-1) : return "EOF";

        // Changed some token names to match my lexer
        case CoolLexer::SEMI   : return "';'";
        case CoolLexer::LBRACE : return "'{'";
        case CoolLexer::RBRACE : return "'}'";
        case CoolLexer::LPAREN : return "'('";
        case CoolLexer::COMMA  : return "','";
        case CoolLexer::RPAREN : return "')'";
        case CoolLexer::COLON  : return "':'";
        case CoolLexer::AT     : return "'@'";
        case CoolLexer::DOT    : return "'.'";
        case CoolLexer::PLUS   : return "'+'";
        case CoolLexer::MINUS  : return "'-'";
        case CoolLexer::MULT   : return "'*'";
        case CoolLexer::DIV    : return "'/'";
        case CoolLexer::TILDE  : return "'~'";
        case CoolLexer::LT     : return "'<'";
        case CoolLexer::EQ     : return "'='";

        case CoolLexer::DARROW : return "DARROW";
        case CoolLexer::ASSIGN : return "ASSIGN";
        case CoolLexer::LE     : return "LE";

        case CoolLexer::CLASS  : return "CLASS";
        case CoolLexer::ELSE   : return "ELSE";
        case CoolLexer::FI     : return "FI";
        case CoolLexer::IF     : return "IF";
        case CoolLexer::IN     : return "IN";
        case CoolLexer::INHERITS : return "INHERITS";
        case CoolLexer::ISVOID : return "ISVOID";
        case CoolLexer::LET    : return "LET";
        case CoolLexer::LOOP   : return "LOOP";
        case CoolLexer::POOL   : return "POOL";
        case CoolLexer::THEN   : return "THEN";
        case CoolLexer::WHILE  : return "WHILE";
        case CoolLexer::CASE   : return "CASE";
        case CoolLexer::ESAC   : return "ESAC";
        case CoolLexer::NEW    : return "NEW";
        case CoolLexer::OF     : return "OF";
        case CoolLexer::NOT    : return "NOT";

        case CoolLexer::BOOL_CONST : return "BOOL_CONST";
        case CoolLexer::INT_CONST  : return "INT_CONST = " + token->getText();
        case CoolLexer::STR_CONST  : return "STR_CONST";
        case CoolLexer::TYPEID     : return "TYPEID = " + token->getText();
        case CoolLexer::OBJECTID   : return "OBJECTID = " + token->getText();
        case CoolLexer::ERROR      : return "ERROR";

        default : return "<Invalid Token>: " + token->toString();
    }
    // clang-format on
}


class TreePrinter : public CoolParserBaseVisitor {
  private:
    CoolLexer *lexer_;
    CoolParser *parser_;
    string file_name_;
    int indent_ = 0;

    void print_indent() { cout << string(indent_, ' '); }

  public:
    TreePrinter(CoolLexer *lexer, CoolParser *parser, const string &file_name)
        : lexer_(lexer), parser_(parser), file_name_(file_name) {}

    any visit(ParseTree *tree) override { return tree->accept(this); }

    any visitProgram(CoolParser::ProgramContext *ctx) override {
        cout << '#' << ctx->getStop()->getLine() << endl;
        cout << "_" << parser_->getRuleNames()[ctx->getRuleIndex()] << endl;
        indent_ += 2;
        visitChildren(ctx);

        indent_ -= 2;
        return any{};
    }

    any visitClass(CoolParser::ClassContext *ctx) override {
        print_indent();
        cout << '#' << ctx->getStop()->getLine() << endl;
        print_indent();
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
        return any{};
    }

    any visitAttr(CoolParser::AttrContext *ctx) override {
        print_indent();
        cout << '#' << ctx->getStart()->getLine() << endl;
        print_indent();
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
        return any{};
    }

    any visitFormal(CoolParser::FormalContext *ctx) {
        print_indent();
        cout << '#' << ctx->getStart()->getLine() << endl;
        print_indent();
        cout << "_" << parser_->getRuleNames()[ctx->getRuleIndex()] << endl;
        indent_ += 2;

        print_indent();
        cout << ctx->OBJECTID()->getText() << endl;

        print_indent();
        cout << ctx->TYPEID()->getText() << endl;

        indent_ -= 2;
        return any{};
    }

    any visitAssign(CoolParser::AssignContext *ctx) {
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
        return any{};
    }

    any visitMethod(CoolParser::MethodContext *ctx) override {
        print_indent();
        cout << '#' << ctx->getStop()->getLine() << endl;
        print_indent();
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
        return any{};
    }

    any visitObject(CoolParser::ObjectContext *ctx) override {
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
        return any{};
    }

    any visitInt(CoolParser::IntContext *ctx) override {
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
        return any{};
    }

    any visitString(CoolParser::StringContext *ctx) override {
        print_indent();
        cout << '#' << ctx->getStart()->getLine() << endl;
        print_indent();
        cout << "_string" << endl;
        indent_ += 2;
        print_indent();
        cout << lexer_->get_string_value(ctx->STR_CONST()->getSymbol()->getStartIndex()) << endl;
        indent_ -= 2;
        print_indent();
        cout << ": _no_type" << endl;
        return any{};
    }

    any visitBool(CoolParser::BoolContext *ctx) override {
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
        return any{};
    }

    template<typename T>
    any visitBinaryOp(T *ctx, const string& opName) {
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
        return any{};
    }

    any visitMult(CoolParser::MultContext *ctx) override { return visitBinaryOp(ctx, "_mul"); }
    any visitDiv(CoolParser::DivContext *ctx) override { return visitBinaryOp(ctx, "_divide"); }
    any visitPlus(CoolParser::PlusContext *ctx) override { return visitBinaryOp(ctx, "_plus"); }
    any visitMinus(CoolParser::MinusContext *ctx) override { return visitBinaryOp(ctx, "_sub"); }
    any visitComp(CoolParser::CompContext *ctx) override { return visitBinaryOp(ctx, "_comp"); }

    template<typename T>
    any visitUnaryOp(T *ctx, const string& opName) {
        print_indent();
        cout << '#' << ctx->getStart()->getLine() << endl;
        print_indent();
        cout << opName << endl;
        indent_ += 2;
        visit(ctx->expr());
        indent_ -= 2;
        print_indent();
        cout << ": _no_type" << endl;
        return any{};
    }

    any visitNeg(CoolParser::NegContext *ctx) override { return visitUnaryOp(ctx, "_neg"); }
    any visitIsvoid(CoolParser::IsvoidContext *ctx) override { return visitUnaryOp(ctx, "_isvoid"); }
    any visitNot(CoolParser::NotContext *ctx) override { return visitUnaryOp(ctx, "_comp"); }

    any visitParen(CoolParser::ParenContext *ctx) override {
        return visit(ctx->expr());
    }

    any visitStatic_dispatch(CoolParser::Static_dispatchContext *ctx) override {
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
        return any{};
    }

    any visitDispatch(CoolParser::DispatchContext *ctx) override {
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
        return any{};
    }

    any visitCond(CoolParser::CondContext *ctx) override {
        print_indent();
        cout << '#' << ctx->getStart()->getLine() << endl;
        print_indent();
        cout << "_cond" << endl;
        indent_ += 2;

        visit(ctx->expr(0));
        visit(ctx->expr(1));
        visit(ctx->expr(2));

        indent_ -= 2;
        print_indent();
        cout << ": _no_type" << endl;
        return any{};
    }

    any visitLoop(CoolParser::LoopContext *ctx) override {
        print_indent();
        cout << '#' << ctx->getStart()->getLine() << endl;
        print_indent();
        cout << "_loop" << endl;
        indent_ += 2;

        visit(ctx->expr(0));
        visit(ctx->expr(1));

        indent_ -= 2;
        print_indent();
        cout << ": _no_type" << endl;
        return any{};
    }

    any visitBlock(CoolParser::BlockContext *ctx) override {
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
        return any{};
    }

    void visitLetRecursion(CoolParser::LetContext *ctx, size_t index, const vector<CoolParser::ExprContext*>& init_exprs, size_t& init_idx) {
        auto object_node = ctx->OBJECTID(index);
        
        print_indent();
        cout << '#' << ctx->getStop()->getLine() << endl;
        print_indent();
        cout << "_let" << endl;
        indent_ += 2;

        print_indent();
        cout << object_node->getText() << endl;
        print_indent();
        cout << ctx->TYPEID(index)->getText() << endl;

        if (ctx->ASSIGN(index)) {
            visit(init_exprs[init_idx++]);
        } else {
            print_indent();
            cout << '#' << object_node->getSymbol()->getLine() << endl;
            print_indent();
            cout << "_no_expr" << endl;
            print_indent();
            cout << ": _no_type" << endl;
        }

        if (index + 1 < ctx->OBJECTID().size()) {
            visitLetRecursion(ctx, index + 1, init_exprs, init_idx);
        } else {
            visit(ctx->expr().back());
        }
        indent_ -= 2;
        print_indent();
        cout << ": _no_type" << endl;
    }

    any visitLet(CoolParser::LetContext *ctx) override {
        auto exprs = ctx->expr();
        auto init_exprs = vector<CoolParser::ExprContext*>();
        size_t init_idx = 0;

        visitLetRecursion(ctx, 0, init_exprs, init_idx);
        return any{};
    }

    any visitCase(CoolParser::CaseContext *ctx) override {
        print_indent();
        cout << '#' << ctx->getStop()->getLine() << endl;
        print_indent();
        cout << "_typcase" << endl;
        indent_ += 2;

        // visit the case variable
        visit(ctx->expr(0));

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

            visit(ctx->expr(i + 1));

            indent_ -= 2;
        }

        indent_ -= 2;
        print_indent();
        cout << ": _no_type" << endl;
        return any{};
    }

    any visitChildren(ParseTree *node) override {
        for (auto child : node->children) {
            child->accept(this);
        }
        return any{};
    }
  public:
    void print() { visitProgram(parser_->program()); }
};

class ErrorPrinter : public BaseErrorListener {
  private:
    string file_name_;
    CoolLexer *lexer_;
    bool has_error_ = false;

  public:
    ErrorPrinter(const string &file_name, CoolLexer *lexer)
        : file_name_(file_name), lexer_(lexer) {}

    virtual void syntaxError(Recognizer *recognizer, Token *offendingSymbol,
                             size_t line, size_t charPositionInLine,
                             const std::string &msg,
                             std::exception_ptr e) override {
        has_error_ = true;
        cout << '"' << file_name_ << "\", line " << line
             << ": syntax error at or near "
             << cool_token_to_string(lexer_, offendingSymbol) << endl;
    }

    bool has_error() const { return has_error_; }
};

int main(int argc, const char *argv[]) {
    if (argc != 2) {
        cerr << "Expecting exactly one argument: name of input file" << endl;
        return 1;
    }

    auto file_path = argv[1];
    ifstream fin(file_path);

    auto file_name = fs::path(file_path).filename().string();

    ANTLRInputStream input(fin);
    CoolLexer lexer(&input);

    CommonTokenStream tokenStream(&lexer);

    CoolParser parser(&tokenStream);

    ErrorPrinter error_printer(file_name, &lexer);

    parser.removeErrorListener(&ConsoleErrorListener::INSTANCE);
    parser.addErrorListener(&error_printer);

    // This will trigger the error_printer, in case there are errors.
    parser.program();
    parser.reset();

    if (!error_printer.has_error()) {
        TreePrinter(&lexer, &parser, file_name).print();
    } else {
        cout << "Compilation halted due to lex and parse errors" << endl;
    }

    return 0;
}
