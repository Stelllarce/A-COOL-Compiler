#ifndef CODEGEN_EXPRESSION_GENERATOR_H_
#define CODEGEN_EXPRESSION_GENERATOR_H_

#include <map>
#include <ostream>
#include <string>

#include "semantics/ClassTable.h"
#include "semantics/typed-ast/Expr.h"

// Forward declarations
class IntConstant;
class StringConstant;
class BoolConstant;
class ObjectReference;
class StaticDispatch;
class DynamicDispatch;
class Sequence;
class Assignment;
class NewObject;
class IfThenElseFi;
class WhileLoopPool;
class LetIn;
class Arithmetic;
class IntegerNegation;
class IntegerComparison;
class EqualityComparison;
class BooleanNegation;
class IsVoid;
class ParenthesizedExpr;
class MethodInvocation;
class CaseOfEsac;

// Constants for object layout
constexpr int WORD_SIZE = 4;
constexpr int OBJECT_TAG_OFFSET = 0;
constexpr int OBJECT_SIZE_OFFSET = 4;
constexpr int DISPATCH_TABLE_OFFSET = 8;
constexpr int FIRST_ATTRIBUTE_OFFSET = 12;

// Class tags for built-in types
constexpr int OBJECT_TAG = 0;
constexpr int IO_TAG = 1;
constexpr int INT_TAG = 2;
constexpr int BOOL_TAG = 3;
constexpr int STRING_TAG = 4;

// String constant management
int register_string_constant(const std::string& value);
std::string escape_string(const std::string& s);
int compute_string_length(const std::string& s);
void reset_string_constants();
const std::map<std::string, int>& get_string_constants();

const std::map<int, int>& get_int_constants();

class ExpressionGenerator {
private:
    ClassTable* class_table_;
    int current_class_index_;
    std::map<std::string, int> local_var_offsets_;
    int next_local_offset_;

public:
    ExpressionGenerator(ClassTable* class_table, int current_class_index,
                        std::map<std::string, int> local_var_offsets, int next_local_offset)
        : class_table_(class_table), current_class_index_(current_class_index),
          local_var_offsets_(std::move(local_var_offsets)), next_local_offset_(next_local_offset) {}

    void emit_expr(std::ostream& out, const Expr* expr);

private:
    void emit_int_constant(std::ostream& out, const IntConstant* expr);
    void emit_string_constant(std::ostream& out, const StringConstant* expr);
    void emit_bool_constant(std::ostream& out, const BoolConstant* expr);
    void emit_object_reference(std::ostream& out, const ObjectReference* expr);
    void emit_static_dispatch(std::ostream& out, const StaticDispatch* expr);
    void emit_dynamic_dispatch(std::ostream& out, const DynamicDispatch* expr);
    void emit_sequence(std::ostream& out, const Sequence* expr);
    void emit_assignment(std::ostream& out, const Assignment* expr);
    void emit_new_object(std::ostream& out, const NewObject* expr);
    void emit_if_then_else(std::ostream& out, const IfThenElseFi* expr);
    void emit_while_loop(std::ostream& out, const WhileLoopPool* expr);
    void emit_let_in(std::ostream& out, const LetIn* expr);
    void emit_arithmetic(std::ostream& out, const Arithmetic* expr);
    void emit_integer_negation(std::ostream& out, const IntegerNegation* expr);
    void emit_integer_comparison(std::ostream& out, const IntegerComparison* expr);
    void emit_equality_comparison(std::ostream& out, const EqualityComparison* expr);
    void emit_boolean_negation(std::ostream& out, const BooleanNegation* expr);
    void emit_is_void(std::ostream& out, const IsVoid* expr);
    void emit_parenthesized(std::ostream& out, const ParenthesizedExpr* expr);
    void emit_method_invocation(std::ostream& out, const MethodInvocation* expr);
    void emit_case_of_esac(std::ostream& out, const CaseOfEsac* expr);
};

#endif
