#include "ExpressionGenerator.h"
#include "CodeEmitter.h"
#include "Location.h"
#include "Register.h"

#include "semantics/typed-ast/Arithmetic.h"
#include "semantics/typed-ast/Assignment.h"
#include "semantics/typed-ast/BoolConstant.h"
#include "semantics/typed-ast/BooleanNegation.h"
#include "semantics/typed-ast/CaseOfEsac.h"
#include "semantics/typed-ast/DynamicDispatch.h"
#include "semantics/typed-ast/EqualityComparison.h"
#include "semantics/typed-ast/IfThenElseFi.h"
#include "semantics/typed-ast/IntConstant.h"
#include "semantics/typed-ast/IntegerComparison.h"
#include "semantics/typed-ast/IntegerNegation.h"
#include "semantics/typed-ast/IsVoid.h"
#include "semantics/typed-ast/LetIn.h"
#include "semantics/typed-ast/MethodInvocation.h"
#include "semantics/typed-ast/NewObject.h"
#include "semantics/typed-ast/ObjectReference.h"
#include "semantics/typed-ast/ParenthesizedExpr.h"
#include "semantics/typed-ast/Sequence.h"
#include "semantics/typed-ast/StaticDispatch.h"
#include "semantics/typed-ast/StringConstant.h"
#include "semantics/typed-ast/Vardecl.h"
#include "semantics/typed-ast/WhileLoopPool.h"

#include <iostream>
#include <sstream>
#include <vector>

using namespace std;
using namespace riscv_emit;

// ============================================================================
// String constant management
// ============================================================================

static map<string, int> string_constants;
static int string_constant_counter = 0;

static map<int, int> int_constants;
static int int_constant_counter = 0;

void reset_string_constants() {
    string_constants.clear();
    string_constant_counter = 0;
    int_constants.clear();
    int_constant_counter = 0;
}

const map<string, int>& get_string_constants() {
    return string_constants;
}

const map<int, int>& get_int_constants() {
    return int_constants;
}

int register_string_constant(const string& value) {
    auto it = string_constants.find(value);
    if (it != string_constants.end()) {
        return it->second;
    }
    int id = ++string_constant_counter;
    string_constants[value] = id;
    return id;
}

int register_int_constant(int value) {
    auto it = int_constants.find(value);
    if (it != int_constants.end()) {
        return it->second;
    }
    int id = ++int_constant_counter;
    int_constants[value] = id;
    return id;
}

// Helper to escape strings for assembly output
// The input string may include surrounding quotes from the source - strip them
string escape_string(const string& s) {
    ostringstream result;
    result << "\"";
    
    string input = s;
    // Strip surrounding quotes if present (from source parsing)
    if (input.size() >= 2 && input.front() == '"' && input.back() == '"') {
        input = input.substr(1, input.size() - 2);
    }
    
    for (size_t i = 0; i < input.size(); i++) {
        char c = input[i];
        // Handle escape sequences from the source
        if (c == '\\' && i + 1 < input.size()) {
            char next = input[i + 1];
            switch (next) {
                case 'n':
                    result << "\\n";
                    i++;
                    break;
                case 't':
                    result << "\\t";
                    i++;
                    break;
                case '\\':
                    result << "\\\\";
                    i++;
                    break;
                case '"':
                    result << "\\\"";
                    i++;
                    break;
                default:
                    result << c;
                    break;
            }
        } else {
            switch (c) {
                case '\n': result << "\\n"; break;
                case '\t': result << "\\t"; break;
                case '\\': result << "\\\\"; break;
                case '"': result << "\\\""; break;
                default: result << c; break;
            }
        }
    }
    result << "\"";
    return result.str();
}

// Compute the actual string length (after processing escape sequences)
int compute_string_length(const string& s) {
    string input = s;
    // Strip surrounding quotes if present
    if (input.size() >= 2 && input.front() == '"' && input.back() == '"') {
        input = input.substr(1, input.size() - 2);
    }
    
    int len = 0;
    for (size_t i = 0; i < input.size(); i++) {
        char c = input[i];
        if (c == '\\' && i + 1 < input.size()) {
            char next = input[i + 1];
            if (next == 'n' || next == 't' || next == '\\' || next == '"') {
                i++;  // Skip the escape character
            }
        }
        len++;
    }
    return len;
}

// ============================================================================
// Expression Generator Implementation
// ============================================================================

void ExpressionGenerator::emit_expr(ostream& out, const Expr* expr) {
    if (auto ic = dynamic_cast<const IntConstant*>(expr)) {
        return emit_int_constant(out, ic);
    }
    if (auto sc = dynamic_cast<const StringConstant*>(expr)) {
        return emit_string_constant(out, sc);
    }
    if (auto bc = dynamic_cast<const BoolConstant*>(expr)) {
        return emit_bool_constant(out, bc);
    }
    if (auto or_ = dynamic_cast<const ObjectReference*>(expr)) {
        return emit_object_reference(out, or_);
    }
    if (auto sd = dynamic_cast<const StaticDispatch*>(expr)) {
        return emit_static_dispatch(out, sd);
    }
    if (auto dd = dynamic_cast<const DynamicDispatch*>(expr)) {
        return emit_dynamic_dispatch(out, dd);
    }
    if (auto seq = dynamic_cast<const Sequence*>(expr)) {
        return emit_sequence(out, seq);
    }
    if (auto assign = dynamic_cast<const Assignment*>(expr)) {
        return emit_assignment(out, assign);
    }
    if (auto newobj = dynamic_cast<const NewObject*>(expr)) {
        return emit_new_object(out, newobj);
    }
    if (auto ite = dynamic_cast<const IfThenElseFi*>(expr)) {
        return emit_if_then_else(out, ite);
    }
    if (auto wlp = dynamic_cast<const WhileLoopPool*>(expr)) {
        return emit_while_loop(out, wlp);
    }
    if (auto li = dynamic_cast<const LetIn*>(expr)) {
        return emit_let_in(out, li);
    }
    if (auto ar = dynamic_cast<const Arithmetic*>(expr)) {
        return emit_arithmetic(out, ar);
    }
    if (auto neg = dynamic_cast<const IntegerNegation*>(expr)) {
        return emit_integer_negation(out, neg);
    }
    if (auto ic = dynamic_cast<const IntegerComparison*>(expr)) {
        return emit_integer_comparison(out, ic);
    }
    if (auto eq = dynamic_cast<const EqualityComparison*>(expr)) {
        return emit_equality_comparison(out, eq);
    }
    if (auto bn = dynamic_cast<const BooleanNegation*>(expr)) {
        return emit_boolean_negation(out, bn);
    }
    if (auto iv = dynamic_cast<const IsVoid*>(expr)) {
        return emit_is_void(out, iv);
    }
    if (auto pe = dynamic_cast<const ParenthesizedExpr*>(expr)) {
        return emit_parenthesized(out, pe);
    }
    if (auto coe = dynamic_cast<const CaseOfEsac*>(expr)) {
        return emit_case_of_esac(out, coe);
    }
    if (auto mi = dynamic_cast<const MethodInvocation*>(expr)) {
        return emit_method_invocation(out, mi);
    }
    
    cerr << "ICE: unhandled expression type: " << typeid(*expr).name() << endl;
    abort();
}

void ExpressionGenerator::emit_int_constant(ostream& out, const IntConstant* expr) {
    int id = register_int_constant(expr->get_value());
    emit_load_address(out, TempRegister{0}, "_int" + to_string(id));
    emit_push_register(out, TempRegister{0});
    emit_pop_into_register(out, ArgumentRegister{0});
}

void ExpressionGenerator::emit_string_constant(ostream& out, const StringConstant* expr) {
    int id = register_string_constant(expr->get_value());
    emit_load_address(out, TempRegister{0}, "_string" + to_string(id) + ".content");
    emit_move(out, ArgumentRegister{0}, TempRegister{0});
}

void ExpressionGenerator::emit_bool_constant(ostream& out, const BoolConstant* expr) {
    emit_ident(out);
    out << "li a0, " << (expr->get_value() ? 1 : 0) << endl;
}

void ExpressionGenerator::emit_object_reference(ostream& out, const ObjectReference* expr) {
    const string& name = expr->get_name();
    
    if (name == "self") {
        emit_load_word(out, TempRegister{0}, MemoryLocation{-4, FramePointer{}});
        emit_push_register(out, TempRegister{0});
        emit_pop_into_register(out, ArgumentRegister{0});
        return;
    }
    
    // Check if it's a local variable or argument
    auto it = local_var_offsets_.find(name);
    if (it != local_var_offsets_.end()) {
        emit_load_word(out, ArgumentRegister{0}, MemoryLocation{it->second, FramePointer{}});
        return;
    }
    
    // Check if it's an attribute
    auto attrs = class_table_->get_all_attributes(current_class_index_);
    for (int i = 0; i < (int)attrs.size(); i++) {
        if (attrs[i] == name) {
            // Load from self object (a0)
            int offset = FIRST_ATTRIBUTE_OFFSET + i * WORD_SIZE;
            emit_load_word(out, ArgumentRegister{0}, MemoryLocation{offset, ArgumentRegister{0}});
            return;
        }
    }
    
    cerr << "ICE: unknown object reference: " << name << endl;
    abort();
}

void ExpressionGenerator::emit_static_dispatch(ostream& out, const StaticDispatch* expr) {
    auto args = expr->get_arguments();
    
    // Evaluate target object first
    emit_expr(out, expr->get_target());
    
    // Push control link (fp) FIRST (caller convention)
    emit_push_register(out, FramePointer{});
    
    // Evaluate and push arguments in reverse order
    for (int i = (int)args.size() - 1; i >= 0; i--) {
        if (auto sc = dynamic_cast<const StringConstant*>(args[i])) {
            int id = register_string_constant(sc->get_value());
            emit_load_address(out, TempRegister{0}, "_string" + to_string(id) + ".content");
            emit_push_register(out, TempRegister{0});
            continue;
        }
        if (auto ic = dynamic_cast<const IntConstant*>(args[i])) {
            int id = register_int_constant(ic->get_value());
            emit_load_address(out, TempRegister{0}, "_int" + to_string(id));
            emit_push_register(out, TempRegister{0});
            continue;
        }

        // Save target (a0) temporarily if we have args
        emit_push_register(out, ArgumentRegister{0});
        emit_expr(out, args[i]);
        emit_move(out, TempRegister{0}, ArgumentRegister{0});
        emit_pop_into_register(out, ArgumentRegister{0});
        emit_push_register(out, TempRegister{0});
    }
        
    // Call the method using static dispatch
    int dispatch_type = expr->get_static_dispatch_type();
    string class_name(class_table_->get_name(dispatch_type));
    string method_name = expr->get_method_name();
    
    emit_jump_and_link(out, class_name + "." + method_name);
}

void ExpressionGenerator::emit_dynamic_dispatch(ostream& out, const DynamicDispatch* expr) {
    // Push control link (fp) (caller convention)
    emit_push_register(out, FramePointer{});

    // Evaluate and push arguments in reverse order
    auto args = expr->get_arguments();
    for (int i = (int)args.size() - 1; i >= 0; i--) {
        emit_expr(out, args[i]);
        emit_push_register(out, ArgumentRegister{0});
    }

    emit_expr(out, expr->get_target());
    
    // Check for void dispatch (a0 still has target)
    emit_branch_equal_zero(out, ArgumentRegister{0}, "_inf_loop");
    
    // Get dispatch table from object
    emit_load_word(out, TempRegister{0}, MemoryLocation{DISPATCH_TABLE_OFFSET, ArgumentRegister{0}});
    
    // Get method index from target type
    int target_type = expr->get_target()->get_type();
    int method_index = class_table_->get_method_index(target_type, expr->get_method_name());
    int method_offset = method_index * WORD_SIZE;
    
    // Load method address and jump
    emit_load_word(out, TempRegister{0}, MemoryLocation{method_offset, TempRegister{0}});
    emit_jump_and_link_register(out, TempRegister{0});
}

void ExpressionGenerator::emit_sequence(ostream& out, const Sequence* expr) {
    auto seq = expr->get_sequence();
    for (auto* e : seq) {
        emit_expr(out, e);
    }
    // Result is in a0 from last expression
}

void ExpressionGenerator::emit_assignment(ostream& out, const Assignment* expr) {
    const string& name = expr->get_assignee_name();
    
    // Evaluate the expression
    emit_expr(out, expr->get_value());
    
    // Check if it's a local variable
    auto it = local_var_offsets_.find(name);
    if (it != local_var_offsets_.end()) {
        emit_store_word(out, ArgumentRegister{0}, MemoryLocation{it->second, FramePointer{}});
        return;
    }
    
    // Check if it's an attribute
    auto attrs = class_table_->get_all_attributes(current_class_index_);
    for (int i = 0; i < (int)attrs.size(); i++) {
        if (attrs[i] == name) {
            int offset = FIRST_ATTRIBUTE_OFFSET + i * WORD_SIZE;
            emit_store_word(out, ArgumentRegister{0}, MemoryLocation{offset, SavedRegister{1}});
            return;
        }
    }
    
    cerr << "ICE: unknown assignment target: " << name << endl;
    abort();
}

void ExpressionGenerator::emit_new_object(ostream& out, const NewObject* expr) {
    int type_index = expr->get_type();
    string type_name(class_table_->get_name(type_index));
    
    // Load prototype object and copy it
    emit_load_address(out, ArgumentRegister{0}, type_name + "_protObj");
    
    emit_push_register(out, FramePointer{});
    emit_call(out, "Object.copy");
    
    // Call the init method
    emit_push_register(out, FramePointer{}); // Push FP before calling init
    emit_call(out, type_name + "_init");
}

void ExpressionGenerator::emit_if_then_else(ostream& out, const IfThenElseFi* expr) {
    int label_id = if_then_else_fi_label_count++;
    string else_label = "_if_else_" + to_string(label_id);
    string end_label = "_if_end_" + to_string(label_id);
    
    // Evaluate condition
    emit_expr(out, expr->get_condition());
    emit_branch_equal_zero(out, ArgumentRegister{0}, else_label);
    
    // Then branch
    emit_expr(out, expr->get_then_expr());
    emit_jump(out, end_label);
    
    // Else branch
    emit_label(out, else_label);
    emit_expr(out, expr->get_else_expr());
    
    emit_label(out, end_label);
}

void ExpressionGenerator::emit_while_loop(ostream& out, const WhileLoopPool* expr) {
    int label_id = while_loop_pool_label_count++;
    string loop_label = "_while_loop_" + to_string(label_id);
    string end_label = "_while_end_" + to_string(label_id);
    
    emit_label(out, loop_label);
    
    // Evaluate condition
    emit_expr(out, expr->get_condition());
    emit_branch_equal_zero(out, ArgumentRegister{0}, end_label);
    
    // Loop body
    emit_expr(out, expr->get_body());
    emit_jump(out, loop_label);
    
    emit_label(out, end_label);
    
    // While loop returns void (0)
    emit_ident(out);
    out << "li a0, 0" << endl;
}

void ExpressionGenerator::emit_let_in(ostream& out, const LetIn* expr) {
    auto vardecls = expr->get_vardecls();
    vector<string> var_names;
    vector<int> var_offsets;
    
    // Process each vardecl
    for (auto* vardecl : vardecls) {
        const string& var_name = vardecl->get_name();
        int var_type = vardecl->get_type();
        
        // Evaluate initializer if present
        if (vardecl->has_initializer()) {
            emit_expr(out, vardecl->get_initializer());
        } else {
            // Default initialization based on type
            string type_name(class_table_->get_name(var_type));
            if (type_name == "Int" || type_name == "Bool" || type_name == "String") {
                emit_load_address(out, ArgumentRegister{0}, type_name + "_protObj");
                emit_push_register(out, FramePointer{});
                emit_call(out, "Object.copy");
                emit_push_register(out, FramePointer{});
                emit_call(out, type_name + "_init");
            } else {
                // void/null for reference types
                emit_ident(out);
                emit_load_immediate(out, ArgumentRegister{0}, 0);
            }
        }
        
        // Push the variable value on stack
        emit_push_register(out, ArgumentRegister{0});
        
        // Record the variable offset
        int offset = next_local_offset_;
        next_local_offset_ -= WORD_SIZE;
        local_var_offsets_[var_name] = offset;
        var_names.push_back(var_name);
        var_offsets.push_back(offset);
    }
    
    // Evaluate body
    emit_expr(out, expr->get_body());
    
    // Clean up variables
    for (const auto& name : var_names) {
        local_var_offsets_.erase(name);
    }
    
    // Restore stack and preserve result (a0)
    // Push result
    emit_push_register(out, ArgumentRegister{0});
    // Pop result into t0
    emit_pop_into_register(out, TempRegister{0});
    
    // Pop variables
    int num_vars = (int)vardecls.size();
    emit_add_immediate(out, StackPointer{}, StackPointer{}, num_vars * WORD_SIZE);
    next_local_offset_ += num_vars * WORD_SIZE;
    
    // Push result (t0)
    emit_push_register(out, TempRegister{0});
    // Pop result into a0
    emit_pop_into_register(out, ArgumentRegister{0});
}

void ExpressionGenerator::emit_arithmetic(ostream& out, const Arithmetic* expr) {
    // Evaluate left operand
    emit_expr(out, expr->get_lhs());
    emit_push_register(out, ArgumentRegister{0});
    
    // Evaluate right operand  
    emit_expr(out, expr->get_rhs());
    emit_move(out, TempRegister{1}, ArgumentRegister{0});
    
    // Pop left operand
    emit_pop_into_register(out, TempRegister{0});
    
    // Perform operation
    switch (expr->get_kind()) {
        case Arithmetic::Kind::Addition:
            emit_add(out, ArgumentRegister{0}, TempRegister{0}, TempRegister{1});
            break;
        case Arithmetic::Kind::Subtraction:
            emit_subtract(out, ArgumentRegister{0}, TempRegister{0}, TempRegister{1});
            break;
        case Arithmetic::Kind::Multiplication:
            emit_multiply(out, ArgumentRegister{0}, TempRegister{0}, TempRegister{1});
            break;
        case Arithmetic::Kind::Division:
            emit_divide(out, ArgumentRegister{0}, TempRegister{0}, TempRegister{1});
            break;
    }
}

void ExpressionGenerator::emit_integer_negation(ostream& out, const IntegerNegation* expr) {
    emit_expr(out, expr->get_argument());
    emit_subtract(out, ArgumentRegister{0}, ZeroRegister{}, ArgumentRegister{0});
}

void ExpressionGenerator::emit_integer_comparison(ostream& out, const IntegerComparison* expr) {
    int label_id = if_then_else_fi_label_count++;
    string true_label = "_cmp_true_" + to_string(label_id);
    string end_label = "_cmp_end_" + to_string(label_id);
    
    // Evaluate left operand
    emit_expr(out, expr->get_lhs());
    emit_push_register(out, ArgumentRegister{0});
    
    // Evaluate right operand
    emit_expr(out, expr->get_rhs());
    emit_move(out, TempRegister{1}, ArgumentRegister{0});
    
    // Pop left operand
    emit_pop_into_register(out, TempRegister{0});
    
    switch (expr->get_kind()) {
        case IntegerComparison::Kind::LessThan:
            emit_set_less_than(out, ArgumentRegister{0}, TempRegister{0}, TempRegister{1});
            break;
        case IntegerComparison::Kind::LessThanEqual:
            emit_set_less_than(out, ArgumentRegister{0}, TempRegister{1}, TempRegister{0});
            emit_xor_immediate(out, ArgumentRegister{0}, ArgumentRegister{0}, 1);
            break;
    }
}

void ExpressionGenerator::emit_equality_comparison(ostream& out, const EqualityComparison* expr) {
    // Evaluate left operand
    emit_expr(out, expr->get_lhs());
    emit_push_register(out, ArgumentRegister{0});
    
    // Evaluate right operand
    emit_expr(out, expr->get_rhs());
    emit_move(out, TempRegister{1}, ArgumentRegister{0});
    
    // Pop left operand
    emit_pop_into_register(out, TempRegister{0});
    
    // Compare: subtract and check if zero
    emit_subtract(out, TempRegister{2}, TempRegister{0}, TempRegister{1});
    emit_set_equal_zero(out, ArgumentRegister{0}, TempRegister{2});
}

void ExpressionGenerator::emit_boolean_negation(ostream& out, const BooleanNegation* expr) {
    emit_expr(out, expr->get_argument());
    emit_xor_immediate(out, ArgumentRegister{0}, ArgumentRegister{0}, 1);
}

void ExpressionGenerator::emit_is_void(ostream& out, const IsVoid* expr) {
    emit_expr(out, expr->get_subject());
    emit_set_equal_zero(out, ArgumentRegister{0}, ArgumentRegister{0});
}

void ExpressionGenerator::emit_parenthesized(ostream& out, const ParenthesizedExpr* expr) {
    emit_expr(out, expr->get_contents());
}

void ExpressionGenerator::emit_case_of_esac(ostream& out, const CaseOfEsac* expr) {
    int label_id = case_of_esac_count++;
    string end_label = "_case_end_" + to_string(label_id);
    string no_match_label = "_case_no_match_" + to_string(label_id);
    
    // Evaluate the expression
    emit_expr(out, expr->get_multiplex());
    
    // Check for void
    emit_branch_equal_zero(out, ArgumentRegister{0}, "_case_abort2");
    
    // Get class tag
    emit_load_word(out, TempRegister{0}, MemoryLocation{OBJECT_TAG_OFFSET, ArgumentRegister{0}});
    
    // Save expression result
    emit_push_register(out, ArgumentRegister{0});
    
    // Try each branch
    const auto& cases = expr->get_cases();
    for (size_t i = 0; i < cases.size(); i++) {
        string branch_label = "_case_branch_" + to_string(label_id) + "_" + to_string(i);
        string next_label = (i + 1 < cases.size()) ? 
            "_case_branch_" + to_string(label_id) + "_" + to_string(i + 1) : no_match_label;
        
        emit_label(out, branch_label);
        
        // Check if type matches
        int branch_type = cases[i].get_type();
        emit_ident(out);
        out << "li t1, " << branch_type << endl;
        emit_subtract(out, TempRegister{2}, TempRegister{0}, TempRegister{1});
        emit_branch_not_equal_zero(out, TempRegister{2}, next_label);
        
        // Match found - bind variable and evaluate body
        const string& var_name = cases[i].get_name();
        int offset = next_local_offset_;
        next_local_offset_ -= WORD_SIZE;
        
        emit_load_word(out, ArgumentRegister{0}, MemoryLocation{0, StackPointer{}});
        emit_push_register(out, ArgumentRegister{0});
        local_var_offsets_[var_name] = offset;
        
        emit_expr(out, cases[i].get_expr());
        
        local_var_offsets_.erase(var_name);
        emit_add_immediate(out, StackPointer{}, StackPointer{}, WORD_SIZE);
        next_local_offset_ += WORD_SIZE;
        
        emit_jump(out, end_label);
    }
    
    emit_label(out, no_match_label);
    emit_call(out, "_case_abort");
    
    emit_label(out, end_label);
    // Clean up saved expression
    emit_add_immediate(out, StackPointer{}, StackPointer{}, WORD_SIZE);
}
void ExpressionGenerator::emit_method_invocation(ostream& out, const MethodInvocation* expr) {
    // Push control link (fp) (caller convention)
    emit_push_register(out, FramePointer{});

    // Evaluate and push arguments in reverse order
    auto args = expr->get_arguments();
    for (int i = (int)args.size() - 1; i >= 0; i--) {
        if (auto sc = dynamic_cast<const StringConstant*>(args[i])) {
            int id = register_string_constant(sc->get_value());
            emit_load_address(out, TempRegister{0}, "_string" + to_string(id) + ".content");
            emit_push_register(out, TempRegister{0});
            continue;
        }
        if (auto ic = dynamic_cast<const IntConstant*>(args[i])) {
            int id = register_int_constant(ic->get_value());
            emit_load_address(out, TempRegister{0}, "_int" + to_string(id));
            emit_push_register(out, TempRegister{0});
            continue;
        }

        emit_expr(out, args[i]);
        emit_push_register(out, ArgumentRegister{0});
    }

    // Target is self, which is available in a0 or we load it from where we saved it.
    // In method prologue, we saved a0 (self) at offsets from fp.
    // We need to load self into a0 for the dispatch.
    // self is stored at -4(fp)
    emit_load_word(out, ArgumentRegister{0}, MemoryLocation{-4, FramePointer{}});
    
    // Check for void dispatch (self shouldn't be void, but for consistency)
    emit_branch_equal_zero(out, ArgumentRegister{0}, "_inf_loop");
    
    // Get dispatch table from object
    emit_load_word(out, TempRegister{0}, MemoryLocation{DISPATCH_TABLE_OFFSET, ArgumentRegister{0}});
    
    int method_index = class_table_->get_method_index(current_class_index_, expr->get_method_name());
    int method_offset = method_index * WORD_SIZE;
    
    // Load method address and jump
    emit_load_word(out, TempRegister{0}, MemoryLocation{method_offset, TempRegister{0}});
    emit_jump_and_link_register(out, TempRegister{0});
}
