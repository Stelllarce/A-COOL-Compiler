#ifndef SEMANTICS_COOL_SEMANTICS_H_
#define SEMANTICS_COOL_SEMANTICS_H_

#include <expected>
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <set>

#include "CoolLexer.h"
#include "CoolParser.h"
#include "typed-ast/Methods.h"
#include "typed-ast/Attributes.h"

struct MethodInfo {
    std::string return_type;
    std::vector<std::string> arg_types;
    CoolParser::MethodContext* ctx;
};

struct AttributeInfo {
    std::string type;
    CoolParser::AttrContext* ctx;
};

struct ClassInfo {
    std::string name;
    std::string parent;
    std::map<std::string, MethodInfo> methods;
    std::map<std::string, AttributeInfo> attributes;
    CoolParser::ClassContext* ctx;
    int depth = -1;
};

struct TypedClass {
    std::string name;
    std::string parent;
    std::string filename;
    int line;
    Attributes attributes;
    Methods methods;
};

struct TypedProgram {
    std::vector<TypedClass> classes;
};

class CoolSemantics {
  private:
    CoolLexer *lexer_;
    CoolParser *parser_;
    std::map<std::string, ClassInfo> classes_;
    std::map<std::string, int> type_ids_;
    std::vector<std::string> type_names_;

  public:
    CoolSemantics(CoolLexer *lexer, CoolParser *parser)
        : lexer_(lexer), parser_(parser) {}

    // Runs semantic analysis and returns the typed AST generated in the
    // process.
    //
    // In case of errors, a list of error messages is returned.
    std::expected<void *, std::vector<std::string>> run();
};

#endif
