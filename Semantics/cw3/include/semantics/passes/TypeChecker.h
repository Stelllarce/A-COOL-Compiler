#ifndef SEMANTICS_PASSES_TYPE_CHECKER_H_
#define SEMANTICS_PASSES_TYPE_CHECKER_H_

#include <any>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <variant>

#include "CoolParser.h"
#include "CoolParserBaseVisitor.h"
#include "semantics/typed-ast/Expr.h"
#include "semantics/CoolSemantics.h"

struct ErrorMessagePrinter {
  enum class MethodError {
    SELF_PARAMETER_NAME,
    MULTIPLE_DEF,
    SELF_ARGUMENT_TYPE,
    UNDEFINED_ARGUMENT_TYPE,
    UNDEFINED_RETURN_TYPE,
    BODY_TYPE_MISMATCH
  };

  enum class AttrError {
    SELF_ATTR_NAME,
    BAD_SUBTYPE
  };
  enum class ExprError {
    OUT_OF_SCOPE,
    NO_SELF_ASSIGN,
    ASSIGNEE_OUT_SCOPE,
    ASSIGNEE_NOT_SUBTYPE,
    METHOD_NOT_DEFINED,
    METHOD_BAD_ARGS_NUMBER,
    METHOD_INVALID_CALL,
    ARGUMENT_HAS_WRONG_TYPE,
    INSTANTIATE_UKNOWN_CLASS,
    IF_ELSE_NOT_BOOL,
    WHILE_NOT_BOOL,
    LET_NO_SELF_ASSIGN,
    LET_NOT_SUBTYPE,
    LET_BAD_TYPE,
    CASE_SELF_TYPE,
    CASE_UKNOWN_TYPE,
    CASE_MULTIPLE_OPTIONS_TYPE,
    STATIC_TO_SELF,
    STATIC_UNDEFINED_TYPE,
    DYN_DISPATCH_BAD_TYPE,
    STAT_DISPATCH_BAD_TYPE,
    DISPATCH_BAD_ARGS_NUMBER,
    DISPATCH_INVALID_CALL,
    DISPATCH_NOT_SUBTYPE,
    OP_BAD_LEFT,
    OP_BAD_RIGHT,
    OP_BAD_COMPARE,
    CMP_BAD_LEFT,
    CMP_BAD_RIGHT,
    NOT_BAD_TYPE,
    TILDE_BAD_TYPE
  };

  std::variant<MethodError, AttrError, ExprError> error;
  std::vector<std::string> args;

  ErrorMessagePrinter(MethodError err, std::vector<std::string> args = {}) : error(err), args(args) {}
  ErrorMessagePrinter(AttrError err, std::vector<std::string> args = {}) : error(err), args(args) {}
  ErrorMessagePrinter(ExprError err, std::vector<std::string> args = {}) : error(err), args(args) {}

  std::string to_string() const;
};

class TypeChecker : public CoolParserBaseVisitor {
  private:
    // all errors
    std::vector<ErrorMessagePrinter> errors;

    const std::map<std::string, ClassInfo>& classes;
    const std::map<std::string, int>& type_ids;
    const std::vector<std::string>& type_names;
    
    // symbol table for every scope
    std::vector<std::map<std::string, std::string>> symbol_table;

    // to bypass any
    std::stack<std::unique_ptr<Expr>> scratchpad;
    
    // track current class
    std::string current_class;
    
    TypedProgram typed_program;

    // override necessary visitor methods
    std::any visitProgram(CoolParser::ProgramContext *ctx) override;
    std::any visitClass(CoolParser::ClassContext *ctx) override;
    std::any visitMethod(CoolParser::MethodContext *ctx) override;
    std::any visitAttr(CoolParser::AttrContext *ctx) override;
    std::any visitFormal(CoolParser::FormalContext *ctx) override;
    std::any visitExpr(CoolParser::ExprContext *ctx) override;

    // helper methods
    void enterScope();
    void exitScope();
    void addSymbol(std::string name, std::string type);
    std::string lookupSymbol(std::string name);
    bool conform(std::string type1, std::string type2);
    std::string lub(std::string type1, std::string type2);
    std::string get_parent(std::string type);
    
    // method for scratchpad
    std::unique_ptr<Expr> visitExprAndAssertOk(CoolParser::ExprContext *ctx);

  public:
    TypeChecker(const std::map<std::string, ClassInfo>& classes,
                const std::map<std::string, int>& type_ids,
                const std::vector<std::string>& type_names) 
        : classes(classes), type_ids(type_ids), type_names(type_names) {}

    // Typechecks the AST that the parser produces and returns a list of errors,
    // if any
    std::vector<std::string> check(CoolParser::ProgramContext *ctx);
    
    TypedProgram getTypedProgram() { return std::move(typed_program); }
};

#endif
