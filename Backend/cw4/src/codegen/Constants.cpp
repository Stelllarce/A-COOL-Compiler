#include "codegen/Constants.h"
#include "codegen/CodeEmitter.h"

using namespace riscv_emit;

Constants::Constants() {
    // Ensure default constants exist
    add_int_constant(0);
    add_string_constant("");
    add_bool_constant(false);
    add_bool_constant(true);
}

std::string Constants::add_int_constant(int value) {
    if (int_constants_.find(value) == int_constants_.end()) {
        std::string label = "int_const" + std::to_string(int_count_++);
        int_constants_[value] = label;
    }
    return int_constants_[value];
}

std::string Constants::add_string_constant(const std::string &value) {
    if (string_constants_.find(value) == string_constants_.end()) {
        std::string label = "str_const" + std::to_string(string_count_++);
        string_constants_[value] = label;
        // Ensure length int constant exists
        add_int_constant(value.length());
    }
    return string_constants_[value];
}

std::string Constants::add_bool_constant(bool value) {
    if (bool_constants_.find(value) == bool_constants_.end()) {
        std::string label = "bool_const" + std::to_string(value ? 1 : 0);
        bool_constants_[value] = label;
    }
    return bool_constants_[value];
}

void Constants::emit(std::ostream &out) {
    // Emit string constants
    for (const auto &[value, label] : string_constants_) {
        emit_label(out, label);
        emit_word(out, string_tag_); // Tag for String
        
        int len = value.length();
        int size = 4 + (len + 1 + 3) / 4; 
        emit_word(out, size);
        emit_word(out, "String_dispTab");
        
        std::string len_label = add_int_constant(len); 
        emit_word(out, len_label);
        
        out << "    .string \"" << value << "\"\n";
        emit_p2align(out, 2);
    }

    // Emit int constants
    for (const auto &[value, label] : int_constants_) {
        emit_label(out, label);
        emit_word(out, int_tag_); // Tag for Int
        emit_word(out, 4); // Size
        emit_word(out, "Int_dispTab");
        emit_word(out, value);
    }

    // Emit bool constants
    for (const auto &[value, label] : bool_constants_) {
        emit_label(out, label);
        emit_word(out, bool_tag_); // Tag for Bool
        emit_word(out, 4); // Size
        emit_word(out, "Bool_dispTab");
        emit_word(out, value ? 1 : 0);
    }
}
