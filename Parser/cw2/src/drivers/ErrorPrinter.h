#pragma once

#include <string>
#include <set>

#include "antlr4-runtime/antlr4-runtime.h"

#include "CoolLexer.h"
#include "CoolParser.h"

using namespace std;
using namespace antlr4;
using namespace antlr4::tree;

class ErrorPrinter : public BaseErrorListener {
  private:
    string file_name_;
    CoolLexer *lexer_;
    CoolParser *parser_;
    bool has_error_ = false;
    // To avoid reporting multiple errors on the same line
    std::set<size_t> lines_with_errors_ = {};
  public:
    ErrorPrinter(const string &file_name, CoolLexer *lexer, CoolParser *parser = nullptr);

    /**
     * @brief Print error message to stdout
     */
    virtual void syntaxError(Recognizer *recognizer, Token *offendingSymbol,
                             size_t line, size_t charPositionInLine,
                             const std::string &msg,
                             std::exception_ptr e) override;

    bool has_error() const;
};
