#include "codegen/ExpressionGenerator.h"
#include "codegen/CodeEmitter.h"

using namespace riscv_emit;

void ExpressionGenerator::emit_expr(std::ostream &out, const Expr *expr) {
    if (auto int_const = dynamic_cast<const IntConstant *>(expr)) {
        emit_int_constant(out, int_const);
    } else if (auto bool_const = dynamic_cast<const BoolConstant *>(expr)) {
        emit_bool_constant(out, bool_const);
    } else if (auto string_const = dynamic_cast<const StringConstant *>(expr)) {
        emit_string_constant(out, string_const);
    } else if (auto static_dispatch = dynamic_cast<const StaticDispatch *>(expr)) {
        emit_static_dispatch(out, static_dispatch);
    } else if (auto dynamic_dispatch = dynamic_cast<const DynamicDispatch *>(expr)) {
        emit_dynamic_dispatch(out, dynamic_dispatch);
    } else if (auto method_invocation = dynamic_cast<const MethodInvocation *>(expr)) {
        emit_method_invocation(out, method_invocation);
    } else if (auto object_ref = dynamic_cast<const ObjectReference *>(expr)) {
        emit_object_reference(out, object_ref);
    } else {
        // Fallback for unimplemented expressions
        emit_comment(out, "Expression not implemented yet");
    }
}

void ExpressionGenerator::emit_int_constant(std::ostream &out, const IntConstant *expr) {
    std::string label = constants_.add_int_constant(expr->get_value());
    emit_load_address(out, Register(ArgumentRegister{0}), label);
}

void ExpressionGenerator::emit_bool_constant(std::ostream &out, const BoolConstant *expr) {
    emit_comment(out, "BoolConstant: " + std::string(expr->get_value() ? "true" : "false"));
}

void ExpressionGenerator::emit_string_constant(std::ostream &out, const StringConstant *expr) {
    std::string value = expr->get_value();
    if (value.length() >= 2 && value.front() == '"' && value.back() == '"') {
        value = value.substr(1, value.length() - 2);
    }
    std::string label = constants_.add_string_constant(value);
    emit_load_address(out, Register(ArgumentRegister{0}), label);
}

void ExpressionGenerator::emit_static_dispatch(std::ostream &out, const StaticDispatch *expr) {
    emit_comment(out, "StaticDispatch: " + expr->get_method_name());
    
    // 1. Evaluate arguments in reverse order and push to stack
    auto args = expr->get_arguments();
    for (auto it = args.rbegin(); it != args.rend(); ++it) {
        emit_expr(out, *it);
        // Result is in a0. Push to stack.
        emit_store_word(out, Register(ArgumentRegister{0}), MemoryLocation{0, Register(StackPointer{})});
        emit_add_immediate(out, Register(StackPointer{}), Register(StackPointer{}), -4);
    }
    
    // 2. Evaluate target (object)
    emit_expr(out, expr->get_target());
    
    // 3. Check for void (TODO)
    
    // 4. Call method
    std::string class_name(class_table_.get_name(expr->get_static_dispatch_type()));
    std::string method_name = expr->get_method_name();
    std::string label = class_name + "." + method_name;
    
    out << "    jal " << label << "\n";
}

void ExpressionGenerator::emit_dynamic_dispatch(std::ostream &out, const DynamicDispatch *expr) {
    emit_comment(out, "DynamicDispatch: " + expr->get_method_name());
    
    // 1. Evaluate arguments in reverse order
    auto args = expr->get_arguments();
    for (auto it = args.rbegin(); it != args.rend(); ++it) {
        emit_expr(out, *it);
        emit_store_word(out, Register(ArgumentRegister{0}), MemoryLocation{0, Register(StackPointer{})});
        emit_add_immediate(out, Register(StackPointer{}), Register(StackPointer{}), -4);
    }
    
    // 2. Evaluate target
    emit_expr(out, expr->get_target());
    
    // 3. Check for void (TODO: emit check)
    
    // 4. Load dispatch table
    // Dispatch table is at offset 8 of the object (a0).
    emit_load_word(out, Register(TempRegister{0}), MemoryLocation{8, Register(ArgumentRegister{0})});
    
    // 5. Load method address
    int target_type = expr->get_target()->get_type();
    if (target_type == SELF_TYPE_INDEX) {
        target_type = current_class_index_;
    }
    
    int method_index = class_table_.get_method_index(target_type, expr->get_method_name());
    int offset = method_index * 4;
    
    // lw t0, offset(t0)
    emit_load_word(out, Register(TempRegister{0}), MemoryLocation{offset, Register(TempRegister{0})});
    
    // 6. Jump to method
    out << "    jalr t0\n";
}

void ExpressionGenerator::emit_method_invocation(std::ostream &out, const MethodInvocation *expr) {
    emit_comment(out, "MethodInvocation: " + expr->get_method_name());
    
    // 1. Evaluate arguments in reverse order
    auto args = expr->get_arguments();
    for (auto it = args.rbegin(); it != args.rend(); ++it) {
        emit_expr(out, *it);
        emit_store_word(out, Register(ArgumentRegister{0}), MemoryLocation{0, Register(StackPointer{})});
        emit_add_immediate(out, Register(StackPointer{}), Register(StackPointer{}), -4);
    }
    
    // 2. Target is self (s1)
    emit_move(out, Register(ArgumentRegister{0}), Register(SavedRegister{1}));
    
    // 3. No void check needed for self
    
    // 4. Load dispatch table
    emit_load_word(out, Register(TempRegister{0}), MemoryLocation{8, Register(ArgumentRegister{0})});
    
    // 5. Load method address
    int method_index = class_table_.get_method_index(current_class_index_, expr->get_method_name());
    int offset = method_index * 4;
    
    emit_load_word(out, Register(TempRegister{0}), MemoryLocation{offset, Register(TempRegister{0})});
    
    // 6. Jump
    out << "    jalr t0\n";
}

void ExpressionGenerator::emit_object_reference(std::ostream &out, const ObjectReference *expr) {
    std::string name = expr->get_name();
    if (name == "self") {
        emit_move(out, Register(ArgumentRegister{0}), Register(SavedRegister{1}));
    } else {
        emit_comment(out, "ObjectReference: " + name + " not implemented yet");
    }
}

