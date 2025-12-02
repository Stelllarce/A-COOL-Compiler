#pragma once

#include "CoolParserBaseVisitor.h"
#include "ErrorPrinter.h"
#include <any>

using namespace std;
using namespace antlr4;
using namespace antlr4::tree;

class ChainedCompVisitor : public CoolParserBaseVisitor {
  private:
    ErrorPrinter &error_printer_;

  public:
    /**
     * Dependency injection of ErrorPrinter to report errors.
     */
    ChainedCompVisitor(ErrorPrinter &error_printer);
    /**
     * @brief Check and report an error caused by chained comparison operators
     */
    std::any visitComp(CoolParser::CompContext *ctx) override;
};
