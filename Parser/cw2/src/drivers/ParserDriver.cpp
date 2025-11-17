#include <cctype>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "antlr4-runtime/antlr4-runtime.h"

#include "CoolLexer.h"
#include "CoolParser.h"
#include "CoolParserBaseVisitor.h"
#include "ErrorPrinter.h"
#include "ChainedCompVisitor.h"
#include "TreePrinter.h"

using namespace std;
using namespace antlr4;
using namespace antlr4::tree;

namespace fs = filesystem;

int main(int argc, const char *argv[]) {
    if (argc != 2) {
        cerr << "Expecting exactly one argument: name of input file" << endl;
        return 1;
    }

    auto file_path = argv[1];
    ifstream fin(file_path);

    auto file_name = fs::path(file_path).filename().string();

    ANTLRInputStream input(fin);
    CoolLexer lexer(&input);

    CommonTokenStream tokenStream(&lexer);

    CoolParser parser(&tokenStream);

    ErrorPrinter error_printer(file_name, &lexer, &parser);

    parser.removeErrorListener(&ConsoleErrorListener::INSTANCE);
    parser.addErrorListener(&error_printer);

    // This will trigger the error_printer, in case there are errors.
    auto *program_tree = parser.program();

    if (!error_printer.has_error()) {
        ChainedCompVisitor chained_comp_visitor(error_printer);
        chained_comp_visitor.visit(program_tree);
    }

    parser.reset();

    if (!error_printer.has_error()) {
        TreePrinter(&lexer, &parser, file_name).print();
    } else {
        cout << "Compilation halted due to lex and parse errors" << endl;
    }

    return 0;
}
