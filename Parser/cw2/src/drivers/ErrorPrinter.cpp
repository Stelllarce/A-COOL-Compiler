#include "ErrorPrinter.h"
#include <iostream>

namespace
{
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
}
ErrorPrinter::ErrorPrinter(const string &file_name, CoolLexer *lexer, CoolParser *parser)
: file_name_(file_name), lexer_(lexer), parser_(parser) {}

void ErrorPrinter::syntaxError(Recognizer *recognizer, Token *offendingSymbol,
    size_t line, size_t charPositionInLine,
    const std::string &msg,
    std::exception_ptr e) {
    // responsible for leaving only one error message per line
    if (lines_with_errors_.count(line))
    return;

    has_error_ = true;
    lines_with_errors_.insert(line);
    cout << '"' << file_name_ << "\", line " << line
    << ": syntax error at or near "
    << cool_token_to_string(lexer_, offendingSymbol) << endl;
}

bool ErrorPrinter::has_error() const { return has_error_; }