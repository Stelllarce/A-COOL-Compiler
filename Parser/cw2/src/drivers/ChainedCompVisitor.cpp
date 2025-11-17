#include "ChainedCompVisitor.h"

ChainedCompVisitor::ChainedCompVisitor(ErrorPrinter &error_printer)
    : error_printer_(error_printer) {}

std::any ChainedCompVisitor::visitComp(CoolParser::CompContext *ctx) {
    // Get the operator
    auto *left_expr = ctx->expr(0);

    if (auto *left_comp_ctx = dynamic_cast<CoolParser::CompContext *>(left_expr)) {
        antlr4::Token *offender_token = nullptr;
        if (ctx->LT()) {
            offender_token = ctx->LT()->getSymbol();
        } else if (ctx->LE()) {
            offender_token = ctx->LE()->getSymbol();
        } else if (ctx->EQ()) {
            offender_token = ctx->EQ()->getSymbol();
        }

        if (offender_token) {
            error_printer_.syntaxError(nullptr, offender_token, offender_token->getLine(), offender_token->getCharPositionInLine(), "syntax error", nullptr);
        }
    }

    return visitChildren(ctx);
}
