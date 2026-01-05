#ifndef CODEGEN_COOL_CODEGEN_H_
#define CODEGEN_COOL_CODEGEN_H_

#include <memory>
#include <ostream>

#include "CoolParser.h"
#include "semantics/ClassTable.h"
#include "codegen/Constants.h"
#include "codegen/ExpressionGenerator.h"

class CoolCodegen {
  private:
    std::string file_name_;
    std::unique_ptr<ClassTable> class_table_;

    void emit_class_name_table(std::ostream &out, Constants &constants);
    void emit_dispatch_tables(std::ostream &out);
    void emit_prototype_objects(std::ostream &out, Constants &constants);
    void emit_object_init_methods(std::ostream &out, ExpressionGenerator &expr_gen);

  public:
    CoolCodegen(std::string file_name, std::unique_ptr<ClassTable> class_table)
        : file_name_(std::move(file_name)),
          class_table_(std::move(class_table)) {}

    void generate(std::ostream &out);
};

#endif
