#include "CoolCodegen.h"
#include "CodeEmitter.h"
#include "ExpressionGenerator.h"
#include "Location.h"
#include "Register.h"

#include <map>
#include <sstream>
#include <vector>

using namespace std;
using namespace riscv_emit;

// ============================================================================
// Code Generator Main Class
// ============================================================================

void CoolCodegen::generate(ostream &out) {
    // Reset static state
    reset_string_constants();
    if_then_else_fi_label_count = 0;
    while_loop_pool_label_count = 0;
    case_of_esac_count = 0;
    
    // Register empty string constant
    register_string_constant("");
    
    // ========================================================================
    // Text Section
    // ========================================================================
    emit_text_segment_tag(out);
    emit_empty_line(out);
    
    // Infinite loop label (for errors)
    emit_label(out, "_inf_loop");
    emit_jump(out, "_inf_loop");
    emit_empty_line(out);
    
    // Generate method implementations
    const auto& class_names = class_table_->get_class_names();
    
    for (int i = 0; i < (int)class_names.size(); i++) {
        const string& class_name = class_names[i];
        
        // Skip built-in classes - their methods are provided by the runtime
        if (class_name == "Object" || class_name == "IO" || 
            class_name == "Int" || class_name == "Bool" || class_name == "String") {
            continue;
        }
        
        // Get methods defined in this class
        auto method_names = class_table_->get_method_names(i);
        
        for (const auto& method_name : method_names) {
            const Expr* body = class_table_->get_method_body(i, method_name);
            if (!body) continue;
            
            auto arg_names = class_table_->get_argument_names(i, method_name);
            
            // Emit method label
            emit_globl(out, class_name + "." + method_name);
            emit_label(out, class_name + "." + method_name);
            
            // Method prologue: set up frame
            emit_move(out, FramePointer{}, StackPointer{});
            emit_store_word(out, ReturnAddress{}, MemoryLocation{0, StackPointer{}});
            emit_grow_stack(out, 1);
            
            // Set up local variable tracking
            // Arguments are on the stack: arg0 at fp+8, arg1 at fp+12, etc.
            map<string, int> local_var_offsets;
            int next_local_offset = -8; // After ra on stack
            
            for (int j = 0; j < (int)arg_names.size(); j++) {
                // Arguments pushed by caller, after control link
                // Stack: [Old FP] [ArgN] ... [Arg0] [RA] (FP points to RA)
                // So Arg0 is at 4(FP), Arg1 at 8(FP) etc.
                local_var_offsets[arg_names[j]] = 4 + j * WORD_SIZE;
            }
            
            // Generate body code
            ExpressionGenerator expr_gen(class_table_.get(), i, &local_var_offsets, &next_local_offset);
            expr_gen.emit_expr(out, body);
            
            // Method epilogue: restore state and return
            emit_load_word(out, ReturnAddress{}, MemoryLocation{0, FramePointer{}});
            // Pop: ra slot (4) + control link (4) + arguments
            emit_add_immediate(out, StackPointer{}, StackPointer{}, 4 + 4 + (int)arg_names.size() * WORD_SIZE);
            emit_load_word(out, FramePointer{}, MemoryLocation{0, StackPointer{}});
            emit_ident(out);
            out << "ret" << endl;
            emit_empty_line(out);
        }
    }
    
    // ========================================================================
    // Data Section
    // ========================================================================
    emit_data_segment_tag(out);
    
    // Class name table
    emit_header_comment(out, "Name table of classes");
    emit_p2align(out, 2);
    emit_globl(out, "class_nameTab");
    emit_label(out, "class_nameTab");
    
    for (const auto& class_name : class_names) {
        emit_word(out, class_name + "_className");
    }
    emit_empty_line(out);
    
    // Class name strings (Int object for length + String object for name)
    for (const auto& class_name : class_names) {
        // Length Int object
        emit_gc_tag(out);
        emit_label(out, class_name + "_classNameLength");
        emit_word(out, class_table_->get_index("Int"), "class tag for Int");
        emit_word(out, 4, "object size");
        emit_word(out, 0, "dispatch table");
        emit_word(out, (int)class_name.length(), "value");
        emit_empty_line(out);
        
        // String object
        emit_gc_tag(out);
        emit_label(out, class_name + "_className");
        emit_word(out, class_table_->get_index("String"), "class tag for String");
        // Size: header (3 words) + length pointer (1) + string content rounded up to word
        int str_words = 5 + ((int)class_name.length() + 4) / 4;
        emit_word(out, str_words, "object size");
        emit_word(out, "String_dispTab");
        emit_word(out, class_name + "_classNameLength");
        emit_ident(out);
        out << ".string " << escape_string(class_name) << endl;
        // Padding to word boundary
        int padding = (4 - ((int)class_name.length() + 1) % 4) % 4;
        for (int j = 0; j < padding; j++) {
            emit_ident(out);
            out << ".byte 0" << endl;
        }
        emit_empty_line(out);
    }
    
    // Prototype objects
    emit_header_comment(out, "Prototype objects");
    emit_p2align(out, 2);
    
    for (int i = 0; i < (int)class_names.size(); i++) {
        const string& class_name = class_names[i];
        auto attrs = class_table_->get_all_attributes(i);
        int obj_size = 3 + (int)attrs.size(); 
        
        emit_gc_tag(out);
        emit_globl(out, class_name + "_protObj");
        emit_label(out, class_name + "_protObj");
        emit_word(out, i, "class tag");
        emit_word(out, obj_size, "object size");
        
        // Int and Bool have no dispatch table pointer
        if (class_name == "Int" || class_name == "Bool") {
            emit_word(out, 0, "dispatch table");
        } else {
            emit_word(out, class_name + "_dispTab");
        }
        
        // Attributes with default values (0)
        for (size_t j = 0; j < attrs.size(); j++) {
            emit_word(out, 0, "attribute: " + attrs[j]);
        }
        emit_empty_line(out);
    }
    
    // Dispatch tables
    emit_header_comment(out, "Dispatch tables");
    
    for (int i = 0; i < (int)class_names.size(); i++) {
        const string& class_name = class_names[i];
        auto methods = class_table_->get_all_methods(i);
        
        emit_globl(out, class_name + "_dispTab");
        emit_label(out, class_name + "_dispTab");
        
        for (const auto& [method_name, defining_class] : methods) {
            string defining_class_name(class_table_->get_name(defining_class));
            emit_word(out, defining_class_name + "." + method_name);
        }
        emit_empty_line(out);
    }
    
    // Init methods
    emit_header_comment(out, "Init methods");
    
    for (int i = 0; i < (int)class_names.size(); i++) {
        const string& class_name = class_names[i];
        
        emit_globl(out, class_name + "_init");
        emit_label(out, class_name + "_init");

        // if (class_name == "String") {
        //     // String_init special case
        //     emit_move(out, FramePointer{}, StackPointer{});
        //     emit_store_word(out, ReturnAddress{}, MemoryLocation{0, StackPointer{}});
        //     emit_grow_stack(out, 1);
            
        //     emit_store_word(out, SavedRegister{1}, MemoryLocation{0, StackPointer{}});
        //     emit_grow_stack(out, 1);
        //     emit_add(out, SavedRegister{1}, ArgumentRegister{0}, ZeroRegister{});
            
        //     emit_load_address(out, ArgumentRegister{0}, "Int_protObj");
        //     emit_store_word(out, FramePointer{}, MemoryLocation{0, StackPointer{}});
        //     emit_grow_stack(out, 1);
            
        //     emit_call(out, "Object.copy");
            
        //     emit_store_word(out, ArgumentRegister{0}, MemoryLocation{12, SavedRegister{1}});
            
        //     emit_add(out, ArgumentRegister{0}, SavedRegister{1}, ZeroRegister{});
            
        //     emit_grow_stack(out, -1);
        //     emit_load_word(out, SavedRegister{1}, MemoryLocation{0, StackPointer{}});
        //     emit_load_word(out, ReturnAddress{}, MemoryLocation{0, FramePointer{}});
        //     emit_add_immediate(out, StackPointer{}, StackPointer{}, 8);
        //     emit_load_word(out, FramePointer{}, MemoryLocation{0, StackPointer{}});
        //     emit_ident(out);
        //     out << "ret" << endl;
        //     emit_empty_line(out);
        //     continue;
        // }
        
        emit_move(out, FramePointer{}, StackPointer{});
        emit_store_word(out, ReturnAddress{}, MemoryLocation{0, StackPointer{}});
        emit_grow_stack(out, 1);
        
        // For most built-in types, just return
        if (class_name == "Object" || class_name == "IO" || 
            class_name == "Int" || class_name == "Bool") {
            emit_load_word(out, ReturnAddress{}, MemoryLocation{0, FramePointer{}});
            emit_add_immediate(out, StackPointer{}, StackPointer{}, 8);
            emit_load_word(out, FramePointer{}, MemoryLocation{0, StackPointer{}});
            emit_ident(out);
            out << "ret" << endl;
            emit_empty_line(out);
            continue;
        }
        
        // String_init needs special handling
        if (class_name == "String") {
            emit_push_register(out, SavedRegister{1});
            emit_move(out, SavedRegister{1}, ArgumentRegister{0});
            
            emit_load_address(out, ArgumentRegister{0}, "Int_protObj");
            emit_push_register(out, FramePointer{});
            emit_call(out, "Object.copy");
            emit_store_word(out, ArgumentRegister{0}, MemoryLocation{FIRST_ATTRIBUTE_OFFSET, SavedRegister{1}});
            emit_move(out, ArgumentRegister{0}, SavedRegister{1});
            emit_add_immediate(out, StackPointer{}, StackPointer{}, 4);
            emit_pop_into_register(out, SavedRegister{1});
            emit_load_word(out, ReturnAddress{}, MemoryLocation{0, FramePointer{}});
            emit_add_immediate(out, StackPointer{}, StackPointer{}, 8);
            emit_load_word(out, FramePointer{}, MemoryLocation{0, StackPointer{}});
            emit_ident(out);
            out << "ret" << endl;
            emit_empty_line(out);
            continue;
        }
        
        // User-defined classes - initialize attributes
        emit_push_register(out, SavedRegister{1});
        emit_move(out, SavedRegister{1}, ArgumentRegister{0});
        
        // Call parent init
        int parent = class_table_->get_parent_index(i);
        if (parent >= 0) {
            string parent_name(class_table_->get_name(parent));
            emit_push_register(out, FramePointer{});
            emit_call(out, parent_name + "_init");
            // Callee pops Control Link (FP)
        }
        
        // Initialize attributes with their initializers
        auto attrs = class_table_->get_attributes(i);  // Only this class's attrs
        auto all_attrs = class_table_->get_all_attributes(i);  // Including inherited
        
        for (const auto& attr_name : attrs) {
            const Expr* init = class_table_->transitive_get_attribute_initializer(class_name, attr_name);
            if (init) {
                map<string, int> local_var_offsets;
                int next_local_offset = -12;
                ExpressionGenerator expr_gen(class_table_.get(), i, &local_var_offsets, &next_local_offset);
                expr_gen.emit_expr(out, init);
                
                // Find attribute offset in object
                for (size_t j = 0; j < all_attrs.size(); j++) {
                    if (all_attrs[j] == attr_name) {
                        int offset = FIRST_ATTRIBUTE_OFFSET + (int)j * WORD_SIZE;
                        emit_store_word(out, ArgumentRegister{0}, MemoryLocation{offset, SavedRegister{1}});
                        break;
                    }
                }
            }
        }
        
        emit_move(out, ArgumentRegister{0}, SavedRegister{1});
        emit_pop_into_register(out, SavedRegister{1});
        emit_load_word(out, ReturnAddress{}, MemoryLocation{0, FramePointer{}});
        emit_add_immediate(out, StackPointer{}, StackPointer{}, 8);
        emit_load_word(out, FramePointer{}, MemoryLocation{0, StackPointer{}});
        emit_ident(out);
        out << "ret" << endl;
        emit_empty_line(out);
    }
    
    // Class object table
    emit_header_comment(out, "Class object table");
    emit_label(out, "class_objTab");
    
    for (const auto& class_name : class_names) {
        emit_word(out, class_name + "_protObj");
        emit_word(out, class_name + "_init");
    }
    emit_empty_line(out);
    
    // String constants
    emit_header_comment(out, "String constants");
    
    for (const auto& [str, id] : get_string_constants()) {
        string label = "_string" + to_string(id);
        int actual_len = compute_string_length(str);
        
        // Length Int object
        emit_gc_tag(out);
        emit_label(out, label + ".length");
        emit_word(out, class_table_->get_index("Int"), "class tag for Int");
        emit_word(out, 4, "object size");
        emit_word(out, 0, "dispatch table");
        emit_word(out, actual_len, "value");
        emit_empty_line(out);
        
        // String object
        emit_gc_tag(out);
        emit_label(out, label + ".content");
        emit_word(out, class_table_->get_index("String"), "class tag for String");
        int str_words = 5 + (actual_len + 4) / 4;
        emit_word(out, str_words, "object size");
        emit_word(out, "String_dispTab");
        emit_word(out, label + ".length");
        emit_ident(out);
        out << ".string " << escape_string(str) << endl;
        // Padding to word boundary
        int padding = (4 - (actual_len + 1) % 4) % 4;
        for (int j = 0; j < padding; j++) {
            emit_ident(out);
            out << ".byte 0" << endl;
        }
        emit_empty_line(out);
    }
    
    // Type tags
    emit_globl(out, "_bool_tag");
    emit_label(out, "_bool_tag");
    emit_word(out, class_table_->get_index("Bool"));
    
    emit_globl(out, "_int_tag");
    emit_label(out, "_int_tag");
    emit_word(out, class_table_->get_index("Int"));
    
    emit_globl(out, "_string_tag");
    emit_label(out, "_string_tag");
    emit_word(out, class_table_->get_index("String"));
}
