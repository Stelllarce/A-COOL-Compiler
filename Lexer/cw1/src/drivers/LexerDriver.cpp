#include <cctype>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "antlr4-runtime/antlr4-runtime.h"

#include "CoolLexer.h"

using namespace std;
using namespace antlr4;

string cool_token_to_string(Token *token) {
    auto token_type = token->getType();

    switch (token_type) {
        case static_cast<size_t>(-1) : return "EOF";

        case CoolLexer::SEMI: return "';'";
        case CoolLexer::COLON: return "':'";
        case CoolLexer::COMMA: return "','";
        case CoolLexer::LPAREN: return "'('";
        case CoolLexer::RPAREN: return "')'";
        case CoolLexer::LBRACE: return "'{'";
        case CoolLexer::RBRACE: return "'}'";
        case CoolLexer::PLUS: return "'+'";
        case CoolLexer::MINUS: return "'-'";
        case CoolLexer::MULT: return "'*'";
        case CoolLexer::DIV: return "'/'";
        case CoolLexer::LT: return "'<'";
        case CoolLexer::EQ: return "'='";
        case CoolLexer::DOT: return "'.'";
        case CoolLexer::AT: return "'@'";
        case CoolLexer::TILDE: return "'~'";
        
        case CoolLexer::ASSIGN: return "ASSIGN";
        case CoolLexer::LE: return "LE";
        case CoolLexer::DARROW: return "DARROW";
        case CoolLexer::BOOL_CONST: return "BOOL_CONST";
        case CoolLexer::INT_CONST: return "INT_CONST";
        case CoolLexer::TYPEID: return "TYPEID";
        case CoolLexer::OBJECTID: return "OBJECTID";

        case CoolLexer::CLASS: return "CLASS";
        case CoolLexer::ELSE: return "ELSE";
        case CoolLexer::ESAC: return "ESAC";
        case CoolLexer::FI: return "FI";
        case CoolLexer::IF: return "IF";
        case CoolLexer::IN: return "IN";
        case CoolLexer::INHERITS: return "INHERITS";
        case CoolLexer::LET: return "LET";
        case CoolLexer::LOOP: return "LOOP";
        case CoolLexer::POOL: return "POOL";
        case CoolLexer::THEN: return "THEN";
        case CoolLexer::WHILE: return "WHILE";
        case CoolLexer::CASE: return "CASE";
        case CoolLexer::OF: return "OF";
        case CoolLexer::NEW: return "NEW";
        case CoolLexer::ISVOID: return "ISVOID";
        case CoolLexer::NOT: return "NOT";
        case CoolLexer::ERROR: return "ERROR";
        case CoolLexer::STR_CONST: return "STR_CONST";

        default : return "<Invalid Token>: " + token->toString();
    }
}

string cool_error_code_to_string(CoolLexer::ErrorCode code) {
    switch (code) {
        case CoolLexer::ErrorCode::UNMATCHED_COMMENT:
            return "Unmatched";
        case CoolLexer::ErrorCode::EOF_IN_STRING:
            return "Unterminated string at EOF";
        case CoolLexer::ErrorCode::EOF_IN_COMMENT:
            return "Unmatched (*";
        case CoolLexer::ErrorCode::STRING_TOO_LONG:
            return "String constant too long";
        case CoolLexer::ErrorCode::INVALID_ESCAPE_SEQUENCE:
            return "String contains unescaped new line";
        case CoolLexer::ErrorCode::INVALID_SYMBOL:
            return "Invalid symbol";
        case CoolLexer::ErrorCode::ESCAPED_NULL:
            return "String contains escaped null character";
        case CoolLexer::ErrorCode::NULL_INSIDE_STRING:
            return "String contains null character";
        default:
            return "Unknown lexer error";
    }
}

void dump_cool_token(CoolLexer *lexer, ostream &out, Token *token) {
    if (token->getType() == static_cast<size_t>(-1)) {
        return;
    }

    out << "#" << token->getLine() << " " << cool_token_to_string(token);

    auto token_type = token->getType();
    auto token_start_char_index = token->getStartIndex();
    switch (token_type) {
    case CoolLexer::BOOL_CONST:
        out << " "
            << (lexer->get_bool_value(token_start_char_index) ? "true"
                                                              : "false");
        break;
    case CoolLexer::TYPEID:
    case CoolLexer::OBJECTID:
        out << " " << token->getText();
        break;
    case CoolLexer::INT_CONST:
        out << " " << token->getText();
        break;
    case CoolLexer::ERROR: {
        CoolLexer::ErrorCode error_code = lexer->get_error_code(token_start_char_index);
        if (error_code == CoolLexer::ErrorCode::INVALID_SYMBOL_NON_PRINTABLE) {
            out << ": " << cool_error_code_to_string(CoolLexer::ErrorCode::INVALID_SYMBOL) << " \"" << lexer->convert_non_printable_to_hex(token->getText()[0]) << "\"";
        }
        else if (error_code == CoolLexer::ErrorCode::INVALID_SYMBOL) {
            if (token->getText()[0] == '\\')
                out << ": " << cool_error_code_to_string(error_code) << " \"\\" << token->getText() << "\"";
            else
                out << ": " << cool_error_code_to_string(error_code) << " \"" << token->getText() << "\"";
        }
        else if (error_code == CoolLexer::ErrorCode::EOF_IN_COMMENT ||
                 error_code == CoolLexer::ErrorCode::EOF_IN_STRING ||
                 error_code == CoolLexer::ErrorCode::INVALID_ESCAPE_SEQUENCE ||
                 error_code == CoolLexer::ErrorCode::NULL_INSIDE_STRING ||
                 error_code == CoolLexer::ErrorCode::ESCAPED_NULL ||
                 error_code == CoolLexer::ErrorCode::STRING_TOO_LONG) {
            out << ": " << cool_error_code_to_string(error_code);
        }
        else {
            out << ": " << cool_error_code_to_string(error_code) << " " << token->getText();
        }
        break;
    }
    case CoolLexer::STR_CONST:
        out << " " << "\"" << lexer->get_string_value(token_start_char_index) << "\"";
        break;
    }
    
    out << endl;
}

int main(int argc, const char *argv[]) {
    ANTLRInputStream input(cin);
    CoolLexer lexer(&input);

    // За временно скриване на грешките:
    // lexer.removeErrorListener(&ConsoleErrorListener::INSTANCE);

    CommonTokenStream tokenStream(&lexer);

    tokenStream.fill(); // Изчитане на всички жетони.

    vector<Token *> tokens = tokenStream.getTokens();
    for (Token *token : tokens) {
        dump_cool_token(&lexer, cout, token);
    };

    return 0;
}
