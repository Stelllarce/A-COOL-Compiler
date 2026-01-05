#include "CoolCodegen.h"
#include "codegen/Constants.h"
#include "codegen/ExpressionGenerator.h"
#include "codegen/CodeEmitter.h"

using namespace std;
using namespace riscv_emit;

void CoolCodegen::generate(ostream &out) {
    Constants constants;
    constants.set_tags(
        class_table_->get_index("Int"),
        class_table_->get_index("Bool"),
        class_table_->get_index("String")
    );
    ExpressionGenerator expr_gen(constants, *class_table_);

    emit_text_segment_tag(out);

    // 3. emit code for method bodies
    for (const auto &class_name : class_table_->get_class_names()) {
        int class_index = class_table_->get_index(class_name);
        expr_gen.set_class_index(class_index);
        
        if (class_name == "Object" || class_name == "IO" || class_name == "Int" || class_name == "Bool" || class_name == "String") {
            continue;
        }

        for (const auto &method_name : class_table_->get_method_names(class_index)) {
             std::string label = class_name + "." + method_name;
             emit_globl(out, label);
             emit_label(out, label);
             
             // Prologue
             emit_move(out, Register(FramePointer{}), Register(StackPointer{}));
             emit_store_word(out, Register(ReturnAddress{}), MemoryLocation{0, Register(StackPointer{})});
             emit_add_immediate(out, Register(StackPointer{}), Register(StackPointer{}), -4);
             
             // Save s1
             emit_store_word(out, Register(SavedRegister{1}), MemoryLocation{0, Register(StackPointer{})});
             emit_add_immediate(out, Register(StackPointer{}), Register(StackPointer{}), -4);
             
             // Move self (a0) to s1
             emit_move(out, Register(SavedRegister{1}), Register(ArgumentRegister{0}));
             
             // Body
             const Expr *body = class_table_->get_method_body(class_index, method_name);
             if (body) {
                 expr_gen.emit_expr(out, body);
             }
             
             // Epilogue
             // Restore s1
             emit_add_immediate(out, Register(StackPointer{}), Register(StackPointer{}), 4);
             emit_load_word(out, Register(SavedRegister{1}), MemoryLocation{0, Register(StackPointer{})});
             
             // Restore sp, ra
             emit_add_immediate(out, Register(StackPointer{}), Register(StackPointer{}), 4);
             emit_load_word(out, Register(ReturnAddress{}), MemoryLocation{0, Register(StackPointer{})});
             
             out << "    ret\n";
        }
    }
    
    // 6. emit initialization methods for classes
    emit_object_init_methods(out, expr_gen);

    emit_data_segment_tag(out);
    
    // Emit tags for runtime
    emit_globl(out, "_int_tag");
    emit_label(out, "_int_tag");
    emit_word(out, class_table_->get_index("Int"));
    
    emit_globl(out, "_bool_tag");
    emit_label(out, "_bool_tag");
    emit_word(out, class_table_->get_index("Bool"));
    
    emit_globl(out, "_string_tag");
    emit_label(out, "_string_tag");
    emit_word(out, class_table_->get_index("String"));
    
    // 7. emit class name table
    emit_class_name_table(out, constants);
    
    // 5. emit dispatch tables
    emit_dispatch_tables(out);
    
    // 4. emit prototype objects
    emit_prototype_objects(out, constants);
    
    // 8. emit static constants
    constants.emit(out);
}

void CoolCodegen::emit_class_name_table(std::ostream &out, Constants &constants) {
    emit_p2align(out, 2);
    emit_globl(out, "class_nameTab");
    emit_label(out, "class_nameTab");
    
    int num_classes = class_table_->get_num_of_classes();
    // Assuming class indices are 0 to num_classes-1 and correspond to tags
    for (int i = 0; i < num_classes; ++i) {
        std::string name(class_table_->get_name(i));
        std::string label = constants.add_string_constant(name);
        emit_word(out, label);
    }
}

void CoolCodegen::emit_dispatch_tables(std::ostream &out) {
    int num_classes = class_table_->get_num_of_classes();
    for (int i = 0; i < num_classes; ++i) {
        std::string class_name(class_table_->get_name(i));
        std::string label = class_name + "_dispTab";
        emit_globl(out, label);
        emit_label(out, label);
        
        auto methods = class_table_->get_all_methods(i);
        for (const auto &method : methods) {
            std::string method_name = method.first;
            int impl_class_index = method.second;
            std::string impl_class_name(class_table_->get_name(impl_class_index));
            emit_word(out, impl_class_name + "." + method_name);
        }
    }
}

void CoolCodegen::emit_prototype_objects(std::ostream &out, Constants &constants) {
    int num_classes = class_table_->get_num_of_classes();
    for (int i = 0; i < num_classes; ++i) {
        std::string class_name(class_table_->get_name(i));
        std::string label = class_name + "_protObj";
        emit_globl(out, label);
        emit_word(out, -1); // GC tag
        emit_label(out, label);
        
        emit_word(out, i); // Class tag
        
        // Calculate size
        auto attributes = class_table_->get_all_attributes(i);
        int size = 3 + attributes.size(); // Header (3) + attributes
        emit_word(out, size);
        
        emit_word(out, class_name + "_dispTab");
        
        // Attributes
        for (const auto &attr_name : attributes) {
            // Determine type of attribute to set default value
            int attr_type_index = class_table_->transitive_get_attribute_type(i, attr_name).value();
            std::string attr_type_name(class_table_->get_name(attr_type_index));
            
            if (attr_type_name == "Int") {
                emit_word(out, constants.add_int_constant(0));
            } else if (attr_type_name == "Bool") {
                emit_word(out, constants.add_bool_constant(false));
            } else if (attr_type_name == "String") {
                emit_word(out, constants.add_string_constant(""));
            } else {
                emit_word(out, 0); // void
            }
        }
    }
}

void CoolCodegen::emit_object_init_methods(std::ostream &out, ExpressionGenerator &expr_gen) {
    int num_classes = class_table_->get_num_of_classes();
    for (int i = 0; i < num_classes; ++i) {
        std::string class_name(class_table_->get_name(i));
        std::string label = class_name + "_init";
        emit_globl(out, label);
        emit_label(out, label);
        
        // Prologue
        emit_move(out, Register(FramePointer{}), Register(StackPointer{}));
        emit_store_word(out, Register(ReturnAddress{}), MemoryLocation{0, Register(StackPointer{})});
        emit_add_immediate(out, Register(StackPointer{}), Register(StackPointer{}), -4);
        
        // Save s1 (self)
        emit_store_word(out, Register(SavedRegister{1}), MemoryLocation{0, Register(StackPointer{})});
        emit_add_immediate(out, Register(StackPointer{}), Register(StackPointer{}), -4);
        
        // Move self (a0) to s1
        emit_move(out, Register(SavedRegister{1}), Register(ArgumentRegister{0}));
        
        // Call parent init
        int parent_index = class_table_->get_parent_index(i);
        if (parent_index != -1) {
            std::string parent_name(class_table_->get_name(parent_index));
            out << "    jal " << parent_name << "_init\n";
        }
        
        // Initialize attributes
        // We only initialize attributes defined in THIS class.
        // But we need to know their offsets in the object.
        // Offsets depend on total number of attributes including inherited ones.
        
        // Get all attributes to calculate offsets
        auto all_attributes = class_table_->get_all_attributes(i);
        // Get local attributes to know which ones to init
        auto local_attributes = class_table_->get_attributes(i);
        
        // Map attribute name to offset
        std::map<std::string, int> attr_offsets;
        for (size_t j = 0; j < all_attributes.size(); ++j) {
            attr_offsets[all_attributes[j]] = 12 + j * 4; // 12 is header size (3 words * 4 bytes)
        }
        
        expr_gen.set_class_index(i);
        
        for (const auto &attr_name : local_attributes) {
            const Expr *init_expr = class_table_->transitive_get_attribute_initializer(class_name, attr_name);
            if (init_expr) {
                expr_gen.emit_expr(out, init_expr);
                // Result in a0. Store to attribute offset.
                int offset = attr_offsets[attr_name];
                emit_store_word(out, Register(ArgumentRegister{0}), MemoryLocation{offset, Register(SavedRegister{1})});
            }
        }
        
        // Restore self to a0
        emit_move(out, Register(ArgumentRegister{0}), Register(SavedRegister{1}));
        
        // Epilogue
        emit_add_immediate(out, Register(StackPointer{}), Register(StackPointer{}), 4);
        emit_load_word(out, Register(SavedRegister{1}), MemoryLocation{0, Register(StackPointer{})});
        
        emit_add_immediate(out, Register(StackPointer{}), Register(StackPointer{}), 4);
        emit_load_word(out, Register(ReturnAddress{}), MemoryLocation{0, Register(StackPointer{})});
        
        out << "    ret\n";
    }
}
