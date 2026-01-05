#ifndef CODEGEN_CONSTANTS_H_
#define CODEGEN_CONSTANTS_H_

#include <string>
#include <map>
#include <ostream>

class Constants {
public:
    Constants();

    // Adds an integer constant and returns its label
    std::string add_int_constant(int value);
    
    // Adds a string constant and returns its label
    std::string add_string_constant(const std::string &value);
    
    // Adds a bool constant and returns its label
    std::string add_bool_constant(bool value);

    void set_tags(int int_tag, int bool_tag, int string_tag) {
        int_tag_ = int_tag;
        bool_tag_ = bool_tag;
        string_tag_ = string_tag;
    }

    // Emits all constants to the output stream
    void emit(std::ostream &out);

private:
    std::map<int, std::string> int_constants_;
    std::map<std::string, std::string> string_constants_;
    std::map<bool, std::string> bool_constants_;
    
    int int_count_ = 0;
    int string_count_ = 0;
    
    int int_tag_ = 2;
    int bool_tag_ = 3;
    int string_tag_ = 4;
};

#endif // CODEGEN_CONSTANTS_H_
