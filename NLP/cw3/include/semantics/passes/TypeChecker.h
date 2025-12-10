#ifndef SEMANTICS_PASSES_TYPE_CHECKER_H_
#define SEMANTICS_PASSES_TYPE_CHECKER_H_

#include <any>
#include <string>
#include <vector>
#include <map>

#include "CoolParser.h"
#include "CoolParserBaseVisitor.h"
#include "semantics/CoolSemantics.h"

class TypeChecker : public CoolParserBaseVisitor {
  private:
    // define any necessary fields
    std::vector<std::string> errors;
    const std::map<std::string, ClassInfo>& classes;
    const std::map<std::string, int>& type_ids;
    const std::vector<std::string>& type_names;
    
    // Symbol table for variables (scopes)
    // Each scope maps variable name to type
    std::vector<std::map<std::string, std::string>> symbol_table;
    
    std::string current_class;
    
    TypedProgram typed_program;

    // override necessary visitor methods
    std::any visitProgram(CoolParser::ProgramContext *ctx) override;
    std::any visitClass(CoolParser::ClassContext *ctx) override;
    std::any visitMethod(CoolParser::MethodContext *ctx) override;
    std::any visitAttr(CoolParser::AttrContext *ctx) override;
    std::any visitFormal(CoolParser::FormalContext *ctx) override;
    std::any visitExpr(CoolParser::ExprContext *ctx) override; // This might need to be split if ANTLR generates specific visit methods for labeled alternatives

    // define helper methods
    void enterScope();
    void exitScope();
    void addSymbol(std::string name, std::string type);
    std::string lookupSymbol(std::string name);
    bool conform(std::string type1, std::string type2); // type1 <= type2
    std::string lub(std::string type1, std::string type2);
    std::string get_parent(std::string type);

  public:
    // TODO: add necessary dependencies to constructor
    TypeChecker(const std::map<std::string, ClassInfo>& classes,
                const std::map<std::string, int>& type_ids,
                const std::vector<std::string>& type_names) 
        : classes(classes), type_ids(type_ids), type_names(type_names) {}

    // Typechecks the AST that the parser produces and returns a list of errors,
    // if any.
    std::vector<std::string> check(CoolParser *parser);
    
    TypedProgram getTypedProgram() { return std::move(typed_program); }
};

#endif
