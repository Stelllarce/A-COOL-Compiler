#ifndef CODEGEN_EXPRESSION_GENERATOR_H_
#define CODEGEN_EXPRESSION_GENERATOR_H_

#include <iostream>
#include "semantics/typed-ast/Expr.h"
#include "semantics/typed-ast/IntConstant.h"
#include "semantics/typed-ast/BoolConstant.h"
#include "semantics/typed-ast/StringConstant.h"
#include "semantics/typed-ast/StaticDispatch.h"
#include "semantics/typed-ast/DynamicDispatch.h"
#include "semantics/typed-ast/MethodInvocation.h"
#include "semantics/typed-ast/ObjectReference.h"
#include "codegen/Constants.h"
#include "semantics/ClassTable.h"

class ExpressionGenerator {
public:
    ExpressionGenerator(Constants &constants, ClassTable &class_table) 
        : constants_(constants), class_table_(class_table) {}

    void set_class_index(int index) { current_class_index_ = index; }
    void emit_expr(std::ostream &out, const Expr *expr);

private:
    Constants &constants_;
    ClassTable &class_table_;
    int current_class_index_ = -1;

    void emit_int_constant(std::ostream &out, const IntConstant *expr);
    void emit_bool_constant(std::ostream &out, const BoolConstant *expr);
    void emit_string_constant(std::ostream &out, const StringConstant *expr);
    void emit_static_dispatch(std::ostream &out, const StaticDispatch *expr);
    void emit_dynamic_dispatch(std::ostream &out, const DynamicDispatch *expr);
    void emit_method_invocation(std::ostream &out, const MethodInvocation *expr);
    void emit_object_reference(std::ostream &out, const ObjectReference *expr);
};

#endif // CODEGEN_EXPRESSION_GENERATOR_H_
