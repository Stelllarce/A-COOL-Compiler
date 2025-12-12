#include "CoolSemantics.h"

#include <expected>
#include <string>
#include <vector>
#include <set>
#include <algorithm>
#include <sstream>

#include "passes/TypeChecker.h"

using namespace std;

string print_inheritance_loops_error(vector<vector<string>> inheritance_loops);

// Runs semantic analysis and returns a list of errors, if any.
expected<TypedProgram, vector<string>> CoolSemantics::run() {
    vector<string> errors;

    // collect classes
    classes_.clear();
    vector<string> processing_order;
    bool fatal_error = false;

    // Add basic classes
    classes_["Object"] = {"Object", "", {}, {}, nullptr, 0};
    processing_order.push_back("Object");
    classes_["IO"] = {"IO", "Object", {}, {}, nullptr};
    processing_order.push_back("IO");
    classes_["Int"] = {"Int", "Object", {}, {}, nullptr};
    processing_order.push_back("Int");
    classes_["String"] = {"String", "Object", {}, {}, nullptr};
    processing_order.push_back("String");
    classes_["Bool"] = {"Bool", "Object", {}, {}, nullptr};
    processing_order.push_back("Bool");

    auto program = parser_->program();
    for (auto class_ctx : program->class_()) {
        string name = class_ctx->TYPEID(0)->getText();
        string parent = "Object";
        if (class_ctx->INHERITS()) {
            parent = class_ctx->TYPEID(1)->getText();
        }

        if (classes_.contains(name)) {
            errors.push_back("Type `" + name + "` already defined");
            fatal_error = true;
            continue;
        }
        
        if (name == "SELF_TYPE") {
             errors.push_back("Redefinition of basic class SELF_TYPE.");
             fatal_error = true;
             continue;
        }

        classes_[name] = {name, parent, {}, {}, class_ctx};
        processing_order.push_back(name);
    }

    // build inheritance graph
    // Check for undefined parents and inheritance from basic classes
    for (const auto& name : processing_order) {
        if (name == "Object") continue;
        auto& info = classes_[name];

        if (!classes_.contains(info.parent)) {
            errors.push_back(name + " inherits from undefined class " + info.parent);
            fatal_error = true;
            continue;
        }

        if (info.parent == "Int" || info.parent == "String" || info.parent == "Bool" || info.parent == "SELF_TYPE") {
            errors.push_back("`" + name + "` inherits from `" + info.parent + "` which is an error");
            fatal_error = true;
        }
    }

    // check inheritance graph is a tree
    vector<vector<string>> inheritance_loops;
    set<string> checked;
    
    for (const auto& name : processing_order) {
        if (checked.contains(name)) continue;
        
        vector<string> path;
        string curr = name;
        while (curr != "Object" && classes_.contains(curr)) {
            // Check if curr is already in path
            auto it = find(path.begin(), path.end(), curr);
            if (it != path.end()) {
                // Cycle detected
                vector<string> loop;
                for (auto k = it; k != path.end(); ++k) {
                    loop.push_back(*k);
                }
                inheritance_loops.push_back(loop);
                break;
            }
            
            if (checked.contains(curr)) {
                break;
            }
            
            path.push_back(curr);
            curr = classes_[curr].parent;
        }
        
        for (const auto& p : path) {
            checked.insert(p);
        }
    }

    if (!inheritance_loops.empty()) {
        errors.push_back(print_inheritance_loops_error(inheritance_loops));
        return unexpected(errors);
    }

    if (fatal_error) {
        return unexpected(errors);
    }

    // collect features
    // Add built-in methods
    // Object
    classes_["Object"].methods["abort"] = {"Object", {}, nullptr};
    classes_["Object"].methods["type_name"] = {"String", {}, nullptr};
    classes_["Object"].methods["copy"] = {"SELF_TYPE", {}, nullptr};
    
    // IO
    classes_["IO"].methods["out_string"] = {"SELF_TYPE", {"String"}, nullptr};
    classes_["IO"].methods["out_int"] = {"SELF_TYPE", {"Int"}, nullptr};
    classes_["IO"].methods["in_string"] = {"String", {}, nullptr};
    classes_["IO"].methods["in_int"] = {"Int", {}, nullptr};
    
    // String
    classes_["String"].methods["length"] = {"Int", {}, nullptr};
    classes_["String"].methods["concat"] = {"String", {"String"}, nullptr};
    classes_["String"].methods["substr"] = {"String", {"Int", "Int"}, nullptr};

    for (const auto& name : processing_order) {
        auto& info = classes_[name];
        if (info.ctx == nullptr) continue;

        for (auto method : info.ctx->method()) {
            string mname = method->OBJECTID()->getText();
            if (info.methods.contains(mname)) {
                errors.push_back("Method `" + mname + "` already defined for class `" + name + "`");
                continue;
            }
            
            vector<string> arg_types;
            for (auto formal : method->formal()) {
                 arg_types.push_back(formal->TYPEID()->getText());
            }
            string return_type = method->TYPEID()->getText();
            
            info.methods[mname] = {return_type, arg_types, method};
        }

        for (auto attr : info.ctx->attr()) {
            string aname = attr->OBJECTID()->getText();
            string type = attr->TYPEID()->getText();

            bool type_exists = classes_.contains(type) || type == "SELF_TYPE";
            if (!type_exists) {
                 errors.push_back("Attribute `" + aname + "` in class `" + name + "` declared to have type `" + type + "` which is undefined");
                 continue;
            }

            if (info.attributes.contains(aname)) {
                errors.push_back("Attribute `" + aname + "` already defined for class `" + name + "`");
                continue;
            }
            
            info.attributes[aname] = {type, attr};
        }
    }

    // check methods are overridden correctly
    for (const auto& name : processing_order) {
        auto& info = classes_[name];
        if (info.ctx == nullptr) continue;

        // Check attributes
        for (auto& [aname, ainfo] : info.attributes) {
            string curr = info.parent;
            while (curr != "" && classes_.contains(curr)) {
                if (classes_[curr].attributes.contains(aname)) {
                    errors.push_back("Attribute `" + aname + "` in class `" + name + "` redefines attribute with the same name in ancestor `" + curr + "` (earliest ancestor that defines this attribute)");
                    break;
                }
                curr = classes_[curr].parent;
            }
        }

        // Check methods
        for (auto& [mname, minfo] : info.methods) {
            string curr = info.parent;
            string earliest_mismatch_ancestor = "";

            while (curr != "" && classes_.contains(curr)) {
                if (classes_[curr].methods.contains(mname)) {
                    // Check signature
                    auto& parent_method = classes_[curr].methods[mname];
                    
                    if (parent_method.error) {
                        curr = classes_[curr].parent;
                        continue;
                    }

                    bool match = true;
                    if (minfo.return_type != parent_method.return_type) match = false;
                    if (minfo.arg_types.size() != parent_method.arg_types.size()) match = false;
                    else {
                        for (size_t i = 0; i < minfo.arg_types.size(); ++i) {
                            if (minfo.arg_types[i] != parent_method.arg_types[i]) {
                                match = false;
                                break;
                            }
                        }
                    }
                    
                    if (!match) {
                        earliest_mismatch_ancestor = curr;
                    }
                }
                curr = classes_[curr].parent;
            }

            if (!earliest_mismatch_ancestor.empty()) {
                errors.push_back("Override for method " + mname + " in class " + name + " has different signature than method in ancestor " + earliest_mismatch_ancestor + " (earliest ancestor that mismatches)");
                minfo.error = true;
            }
        }
    }

    // Assign type IDs
    type_ids_.clear();
    type_names_.clear();
    int id_counter = 0;
    for (auto& [name, info] : classes_) {
        type_ids_[name] = id_counter++;
        type_names_.push_back(name);
    }
    // Add SELF_TYPE
    type_ids_["SELF_TYPE"] = id_counter;
    type_names_.push_back("SELF_TYPE");

    // Compute depths
    for (auto& [name, info] : classes_) {
        int d = 0;
        string curr = name;
        while (curr != "Object" && classes_.contains(curr)) {
            d++;
            curr = classes_[curr].parent;
        }
        info.depth = d;
    }

    TypeChecker checker(classes_, type_ids_, type_names_);
    for (const auto &error : checker.check(program)) {
        errors.push_back(error);
    }

    if (!errors.empty()) {
        return unexpected(errors);
    }

    // return the typed AST
    return checker.getTypedProgram();
}

string print_inheritance_loops_error(vector<vector<string>> inheritance_loops) {
    stringstream eout;
    eout << "Detected " << inheritance_loops.size()
         << " loops in the type hierarchy:" << endl;
    for (int i = 0; i < inheritance_loops.size(); ++i) {
        eout << i + 1 << ") ";
        auto &loop = inheritance_loops[i];
        for (string name : loop) {
            eout << name << " <- ";
        }
        eout << endl;
    }

    return eout.str();
}
