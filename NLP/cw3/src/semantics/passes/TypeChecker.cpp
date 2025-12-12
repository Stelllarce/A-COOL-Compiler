#include "semantics/passes/TypeChecker.h"

#include "semantics/typed-ast/Arithmetic.h"
#include "semantics/typed-ast/Assignment.h"
#include "semantics/typed-ast/Attribute.h"
#include "semantics/typed-ast/Attributes.h"
#include "semantics/typed-ast/BoolConstant.h"
#include "semantics/typed-ast/BooleanNegation.h"
#include "semantics/typed-ast/CaseOfEsac.h"
#include "semantics/typed-ast/DynamicDispatch.h"
#include "semantics/typed-ast/EqualityComparison.h"
#include "semantics/typed-ast/Expr.h"
#include "semantics/typed-ast/IfThenElseFi.h"
#include "semantics/typed-ast/IntConstant.h"
#include "semantics/typed-ast/IntegerComparison.h"
#include "semantics/typed-ast/IntegerNegation.h"
#include "semantics/typed-ast/IsVoid.h"
#include "semantics/typed-ast/LetIn.h"
#include "semantics/typed-ast/Method.h"
#include "semantics/typed-ast/MethodInvocation.h"
#include "semantics/typed-ast/Methods.h"
#include "semantics/typed-ast/NewObject.h"
#include "semantics/typed-ast/ObjectReference.h"
#include "semantics/typed-ast/ParenthesizedExpr.h"
#include "semantics/typed-ast/Sequence.h"
#include "semantics/typed-ast/StaticDispatch.h"
#include "semantics/typed-ast/StringConstant.h"
#include "semantics/typed-ast/Vardecl.h"
#include "semantics/typed-ast/WhileLoopPool.h"

using namespace std;

vector<string> TypeChecker::check(CoolParser::ProgramContext *ctx) {
    visit(ctx);
    return errors;
}

void TypeChecker::enterScope() {
    symbol_table.push_back({});
}

void TypeChecker::exitScope() {
    symbol_table.pop_back();
}

void TypeChecker::addSymbol(string name, string type) {
    symbol_table.back()[name] = type;
}

string TypeChecker::lookupSymbol(string name) {
    for (auto it = symbol_table.rbegin(); it != symbol_table.rend(); ++it) {
        if (it->contains(name)) {
            return (*it)[name];
        }
    }
    return "";
}

bool TypeChecker::conform(string type1, string type2) {
    if (type1 == type2) return true;
    if (type2 == "Object") return true;
    if (type1 == "Object") return false; 
    
    if (type1 == "SELF_TYPE") {
        if (type2 == "SELF_TYPE") return true;
        return conform(current_class, type2);
    }
    if (type2 == "SELF_TYPE") {
        return false; 
    }
    
    string curr = type1;
    while (curr != "Object" && classes.contains(curr)) {
        if (curr == type2) return true;
        curr = classes.at(curr).parent;
    }
    return false;
}

string TypeChecker::lub(string type1, string type2) {
    if (type1 == type2) return type1;
    if (type1 == "SELF_TYPE") return lub(current_class, type2);
    if (type2 == "SELF_TYPE") return lub(type1, current_class);
    
    if (!classes.contains(type1) || !classes.contains(type2)) return "Object";

    int d1 = classes.at(type1).depth;
    int d2 = classes.at(type2).depth;
    
    string t1 = type1;
    string t2 = type2;
    
    while (d1 > d2) {
        t1 = classes.at(t1).parent;
        d1--;
    }
    while (d2 > d1) {
        t2 = classes.at(t2).parent;
        d2--;
    }
    
    while (t1 != t2) {
        t1 = classes.at(t1).parent;
        t2 = classes.at(t2).parent;
    }
    
    return t1;
}

unique_ptr<Expr> TypeChecker::visitExprAndAssertOk(CoolParser::ExprContext *ctx) {
    visitExpr(ctx);
    if (scratchpad.empty()) {
        assert(false && "ICE: scratchpad is empty after visitExpr, but "
               "caller expects an Expr there");
    }
    auto expr = move(scratchpad.top());
    scratchpad.pop();
    return move(expr);
}

any TypeChecker::visitProgram(CoolParser::ProgramContext *ctx) {
    for (auto class_ctx : ctx->class_()) {
        visit(class_ctx);
    }
    return nullptr;
}

any TypeChecker::visitClass(CoolParser::ClassContext *ctx) {
    current_class = ctx->TYPEID(0)->getText();
    string parent = "Object";
    if (ctx->INHERITS()) parent = ctx->TYPEID(1)->getText();
    
    TypedClass typed_class;
    typed_class.name = current_class;
    typed_class.parent = parent;
    typed_class.line = ctx->getStart()->getLine();
    
    enterScope();
    addSymbol("self", "SELF_TYPE");
    
    // Add inherited attributes
    string curr = parent;
    while (curr != "Object" && classes.contains(curr)) {
        for (auto const& [name, info] : classes.at(curr).attributes) {
             addSymbol(name, info.type);
        }
        curr = classes.at(curr).parent;
    }

    for (auto attr : ctx->attr()) {
        string name = attr->OBJECTID()->getText();
        string type = attr->TYPEID()->getText();
        if (type_ids.contains(type)) {
            addSymbol(name, type);
        }
    }

    for (auto attr : ctx->attr()) {
        any res = visit(attr);
        if (res.has_value()) {
            unique_ptr<Attribute> a(any_cast<Attribute*>(res));
            typed_class.attributes.add(move(*a));
        }
    }
    std::set<string> seen_methods;
    for (auto method : ctx->method()) {
        string method_name = method->OBJECTID()->getText();
        if (seen_methods.contains(method_name)) {
            continue;
        }
        seen_methods.insert(method_name);

        any res = visit(method);
        if (res.has_value()) {
            unique_ptr<Method> m(any_cast<Method*>(res));
            if (!typed_class.methods.contains(m->get_name())) {
                typed_class.methods.add_method(move(*m));
            }
        }
    }
    
    exitScope();
    
    typed_program.classes.push_back(move(typed_class));
    return nullptr;
}

any TypeChecker::visitMethod(CoolParser::MethodContext *ctx) {
    string method_name = ctx->OBJECTID()->getText();
    enterScope();
    
    vector<string> arg_names;
    vector<string> arg_types;
    bool types_ok = true;
    
    for (auto formal : ctx->formal()) {
        string name = formal->OBJECTID()->getText();
        string type = formal->TYPEID()->getText();
        if (name == "self") {
            errors.push_back("self cannot be the name of a formal parameter");
        }
        if (symbol_table.back().contains(name)) {
            errors.push_back("Formal parameter " + name + " is multiply defined");
        }
        if (type == "SELF_TYPE") {
             errors.push_back("Formal argument `" + name + "` declared of type `SELF_TYPE` which is not allowed");
             types_ok = false;
        } else if (!type_ids.contains(type)) {
             errors.push_back("Method `" + method_name + "` in class `" + current_class + "` declared to have an argument of type `" + type + "` which is undefined");
             types_ok = false;
        }
        addSymbol(name, type);
        arg_names.push_back(name);
        arg_types.push_back(type);
    }
    
    string return_type = ctx->TYPEID()->getText();
    if (!type_ids.contains(return_type)) {
        errors.push_back("Method `" + method_name + "` in class `" + current_class + "` declared to have return type `" + return_type + "` which is undefined");
        types_ok = false;
    }

    if (!types_ok) {
        exitScope();
        return {};
    }
    
    size_t errors_before = errors.size();
    auto body = visitExprAndAssertOk(ctx->expr());
    string body_type = type_names[body->get_type()];
    
    if (errors.size() == errors_before || body_type != "Object") {
        if (!conform(body_type, return_type)) {
            errors.push_back("In class `" + current_class + "` method `" + method_name + "`: `" + body_type + "` is not `" + return_type + "`: type of method body is not a subtype of return type");
        }
    }
    
    exitScope();
    
    vector<int> signature;
    for (const auto& t : arg_types) signature.push_back(type_ids.at(t));
    signature.push_back(type_ids.at(return_type));
    
    auto m = make_unique<Method>(method_name, signature);
    m->set_argument_names(arg_names);
    m->set_body(move(body));
    return m.release();
}

any TypeChecker::visitAttr(CoolParser::AttrContext *ctx) {
    string name = ctx->OBJECTID()->getText();
    string type = ctx->TYPEID()->getText();
    
    if (name == "self") {
        errors.push_back("self cannot be the name of an attribute");
    }
    
    if (!type_ids.contains(type)) {
        return {};
    }

    unique_ptr<Expr> init = nullptr;
    if (ctx->ASSIGN()) {
        size_t errors_before = errors.size();
        init = visitExprAndAssertOk(ctx->expr());
        string init_type = type_names[init->get_type()];
        
        if (errors.size() == errors_before) {
            if (!conform(init_type, type)) {
                errors.push_back("In class `" + current_class + "` attribute `" + name + "`: `" + init_type + "` is not `" + type + "`: type of initialization expression is not a subtype of declared type");
            }
        }
    }
    
    auto a = make_unique<Attribute>(name, type_ids.at(type));
    if (init) a->set_initializer(move(init));
    return a.release();
}

any TypeChecker::visitFormal(CoolParser::FormalContext *ctx) {
    return nullptr;
}

any TypeChecker::visitExpr(CoolParser::ExprContext *ctx) {
    // Literals
    if (ctx->INT_CONST()) {
        scratchpad.push(std::make_unique<IntConstant>(stoi(ctx->INT_CONST()->getText()), type_ids.at("Int")));
        return nullptr;
    }
    if (ctx->STR_CONST()) {
        scratchpad.push(std::make_unique<StringConstant>(ctx->STR_CONST()->getText(), type_ids.at("String")));
        return nullptr;
    }
    if (ctx->BOOL_CONST()) {
        scratchpad.push(std::make_unique<BoolConstant>(ctx->BOOL_CONST()->getText() == "true", type_ids.at("Bool")));
        return nullptr;
    }
    
    // Variable
    if (!ctx->OBJECTID().empty() && ctx->children.size() == 1) {
        string name = ctx->OBJECTID(0)->getText();
        string type = lookupSymbol(name);
        if (type == "") {
            errors.push_back("Variable named `" + name + "` not in scope");
            type = "Object"; 
        }
        scratchpad.push(make_unique<ObjectReference>(name, type_ids.at(type)));
        return nullptr;
    }
    
    // Assignment
    if (ctx->ASSIGN()) {
        string name = ctx->OBJECTID(0)->getText();
        if (name == "self") {
            errors.push_back("Cannot assign to 'self'.");
        }
        
        auto val = visitExprAndAssertOk(ctx->expr(0));
        string val_type = type_names[val->get_type()];
        
        string var_type = lookupSymbol(name);
        if (var_type == "") {
            errors.push_back("Assignee named `" + name + "` not in scope");
            var_type = "Object";
        } else {
            // If we are in an attribute initialization, we might be assigning to the attribute itself.
            // But wait, the attribute is already in the symbol table (we added it in visitClass).
            // The issue in 048 is: bar : Int <- bar <- "hello";
            // The inner assignment is bar <- "hello".
            // bar is Int. "hello" is String.
            // So inner assignment fails: String is not Int.
            // The inner assignment returns String (type of "hello").
            // The outer assignment is bar : Int <- (bar <- "hello").
            // So it's initializing bar with the result of the inner assignment.
            // The result of inner assignment is String.
            // So outer initialization fails: String is not Int.
            
            // The expected output only shows ONE error:
            // In class `Foo` assignee `bar`: `String` is not `Int`: type of initialization expression is not a subtype of object type
            
            // This corresponds to the inner assignment failure.
            // The outer initialization failure (attribute init) seems to be suppressed or not reported?
            // Or maybe the inner assignment returns the type of the variable being assigned to?
            // No, assignment returns the value of the expression.
            
            // Let's check the manual.
            // "The value of an assignment is the value of the expression on the right hand side."
            
            // So why is the second error not reported?
            // Maybe because the inner expression already had an error?
            // If we suppress cascading errors?
            
            if (!conform(val_type, var_type)) {
                errors.push_back("In class `" + current_class + "` assignee `" + name + "`: `" + val_type + "` is not `" + var_type + "`: type of initialization expression is not a subtype of object type");
                // On error, return the variable type to avoid cascading errors?
                // In 082, this causes "Foo is not Bar" error because Foo (var type) !<= Bar (return type).
                // In 048, this causes "Int is not Int" (valid) so no second error.
                val_type = var_type;
            }
        }
        
        scratchpad.push(make_unique<Assignment>(name, move(val), type_ids.at(val_type)));
        return nullptr;
    }
    
    // Implicit Dispatch
    if (!ctx->OBJECTID().empty() && ctx->children.size() > 1 && ctx->children[1]->getText() == "(") {
        string method_name = ctx->OBJECTID(0)->getText();
        
        // Target is self
        auto target = make_unique<ObjectReference>("self", type_ids.at("SELF_TYPE"));
        string target_type = "SELF_TYPE";
        
        vector<unique_ptr<Expr>> args;
        for (auto e : ctx->expr()) {
            args.push_back(visitExprAndAssertOk(e));
        }
        
        string lookup_type = current_class;
        
        string actual_method_type = "Object";
        vector<string> formal_types;
        bool method_found = false;
        
        string curr = lookup_type;
        while (curr != "" && classes.contains(curr)) {
            if (classes.at(curr).methods.contains(method_name)) {
                auto& m = classes.at(curr).methods.at(method_name);
                actual_method_type = m.return_type;
                formal_types = m.arg_types;
                method_found = true;
                break;
            }
            curr = classes.at(curr).parent;
        }
        
        if (!method_found) {
            errors.push_back("Method `" + method_name + "` not defined for type `" + lookup_type + "` in dynamic dispatch");
        } else {
            if (args.size() != formal_types.size()) {
                errors.push_back("Method `" + method_name + "` of type `" + lookup_type + "` called with the wrong number of arguments; " + to_string(formal_types.size()) + " arguments expected, but " + to_string(args.size()) + " provided");
            } else {
                for (size_t i = 0; i < args.size(); ++i) {
                    string arg_type = type_names[args[i]->get_type()];
                    if (!conform(arg_type, formal_types[i])) {
                        errors.push_back("Invalid call to method `" + method_name + "` from class `" + lookup_type + "`:");
                        errors.push_back("  `" + arg_type + "` is not a subtype of `" + formal_types[i] + "`: argument at position " + to_string(i) + " (0-indexed) has the wrong type");
                    }
                }
            }
        }
        
        string return_type = actual_method_type;
        if (return_type == "SELF_TYPE") {
            return_type = target_type; 
        }
        
        if (!method_found) {
            return_type = "Object";
        }
        
        scratchpad.push(make_unique<DynamicDispatch>(move(target), method_name, move(args), type_ids.at(return_type)));
        return nullptr;
    }
    
    // New
    if (ctx->NEW()) {
        string type = ctx->TYPEID(0)->getText();
        if (type != "SELF_TYPE" && !classes.contains(type)) {
            errors.push_back("Attempting to instantiate unknown class `" + type + "`");
            type = "Object";
        }
        scratchpad.push(make_unique<NewObject>(type_ids.at(type)));
        return nullptr;
    }
    
    // If
    if (ctx->IF()) {
        auto pred = visitExprAndAssertOk(ctx->expr(0));
        string pred_type = type_names[pred->get_type()];
        if (pred_type != "Bool") {
            errors.push_back("Type `" + pred_type + "` of if-then-else-fi condition is not `Bool`");
        }
        
        auto then_e = visitExprAndAssertOk(ctx->expr(1));
        string then_type = type_names[then_e->get_type()];
        
        auto else_e = visitExprAndAssertOk(ctx->expr(2));
        string else_type = type_names[else_e->get_type()];
        
        string join_type = lub(then_type, else_type);
        scratchpad.push(make_unique<IfThenElseFi>(move(pred), move(then_e), move(else_e), type_ids.at(join_type)));
        return nullptr;
    }
    
    // While
    if (ctx->WHILE()) {
        auto pred = visitExprAndAssertOk(ctx->expr(0));
        string pred_type = type_names[pred->get_type()];
        if (pred_type != "Bool") {
            errors.push_back("Type `" + pred_type + "` of while-loop-pool condition is not `Bool`");
        }
        
        auto body = visitExprAndAssertOk(ctx->expr(1));
        
        scratchpad.push(make_unique<WhileLoopPool>(move(pred), move(body), type_ids.at("Object")));
        return nullptr;
    }
    
    // Block
    if (ctx->children.size() > 2 && ctx->children[0]->getText() == "{") {
        vector<unique_ptr<Expr>> exprs;
        string last_type = "Object"; 
        for (auto e : ctx->expr()) {
            auto expr = visitExprAndAssertOk(e);
            last_type = type_names[expr->get_type()];
            exprs.push_back(move(expr));
        }
        scratchpad.push(make_unique<Sequence>(move(exprs), type_ids.at(last_type)));
        return nullptr;
    }
    
    // Let
    if (ctx->LET()) {
        enterScope();
        vector<unique_ptr<Vardecl>> decls;
        for (auto v : ctx->vardecl()) {
            string name = v->OBJECTID()->getText();
            string type = v->TYPEID()->getText();
            if (name == "self") {
                errors.push_back("'self' cannot be bound in a 'let' expression.");
            }
            if (type != "SELF_TYPE" && !classes.contains(type)) {
                errors.push_back("Class `" + type + "` of let-bound identifier `" + name + "` is undefined");
                type = "Object";
            }
            
            unique_ptr<Expr> init = nullptr;
            if (v->ASSIGN()) {
                size_t errors_before = errors.size();
                init = visitExprAndAssertOk(v->expr());
                bool init_had_error = errors.size() > errors_before;
                string init_type = type_names[init->get_type()];
                if (!init_had_error && !conform(init_type, type)) {
                    errors.push_back("Initializer for variable `" + name + "` in let-in expression is of type `" + init_type + "` which is not a subtype of the declared type `" + type + "`");
                }
            }
            
            addSymbol(name, type);
            decls.push_back(unique_ptr<Vardecl>(new Vardecl(name, move(init), type_ids.at(type))));
        }
        
        auto body = visitExprAndAssertOk(ctx->expr(0));
        
        exitScope();
        
        scratchpad.push(make_unique<LetIn>(move(decls), move(body), body->get_type()));
        return nullptr;
    }
    
    // Case
    if (ctx->CASE()) {
        auto expr = visitExprAndAssertOk(ctx->expr(0));
        
        vector<CaseOfEsac::Case> cases;
        string join_type = "";
        
        size_t num_branches = ctx->OBJECTID().size();
        set<string> branch_types;
        
        for (size_t i = 0; i < num_branches; ++i) {
            string name = ctx->OBJECTID(i)->getText();
            string type = ctx->TYPEID(i)->getText();
            bool type_ok = true;
            
            if (type == "SELF_TYPE") {
                errors.push_back("`" + name + "` in case-of-esac declared to be of type `SELF_TYPE` which is not allowed");
                type_ok = false;
            } else if (!classes.contains(type)) {
                errors.push_back("Option `" + name + "` in case-of-esac declared to have unknown type `" + type + "`");
                type_ok = false;
            }
            
            if (branch_types.contains(type)) {
                errors.push_back("Multiple options match on type `" + type + "`");
            }
            branch_types.insert(type);
            
            enterScope();
            if (type_ok) {
                addSymbol(name, type);
            }
            
            auto branch_expr = visitExprAndAssertOk(ctx->expr(i+1));
            string branch_type = type_names[branch_expr->get_type()];
            
            if (join_type == "") join_type = branch_type;
            else join_type = lub(join_type, branch_type);
            
            int type_id = type_ok ? type_ids.at(type) : type_ids.at("Object");
            cases.emplace_back(name, type_id, move(branch_expr));
            
            exitScope();
        }
        
        scratchpad.push(make_unique<CaseOfEsac>(move(expr), move(cases), ctx->getStart()->getLine(), type_ids.at(join_type)));
        return nullptr;
    }
    
    // Dispatch
    if (ctx->DOT()) {
        size_t errors_before = errors.size();
        auto target = visitExprAndAssertOk(ctx->expr(0));
        bool target_had_error = errors.size() > errors_before;
        
        string target_type = type_names[target->get_type()];
        
        string method_name = ctx->OBJECTID(0)->getText();
        string static_type = "";
        bool static_type_error = false;
        if (ctx->AT()) {
            static_type = ctx->TYPEID(0)->getText();
            if (static_type == "SELF_TYPE") {
                errors.push_back("Static dispatch to SELF_TYPE.");
                static_type = "Object";
                static_type_error = true;
            } else if (!classes.contains(static_type)) {
                errors.push_back("Undefined type `" + static_type + "` in static method dispatch");
                static_type = "Object";
                static_type_error = true;
            } else if (!conform(target_type, static_type)) {
                errors.push_back("`" + target_type + "` is not a subtype of `" + static_type + "`");
            }
        }
        
        vector<unique_ptr<Expr>> args;
        for (size_t i = 1; i < ctx->expr().size(); ++i) {
            args.push_back(visitExprAndAssertOk(ctx->expr(i)));
        }
        
        string lookup_type = static_type.empty() ? target_type : static_type;
        if (static_type_error) lookup_type = target_type;
        if (lookup_type == "SELF_TYPE") lookup_type = current_class;
        
        string actual_method_type = "Object";
        vector<string> formal_types;
        bool method_found = false;
        
        string curr = lookup_type;
        while (curr != "" && classes.contains(curr)) {
            if (classes.at(curr).methods.contains(method_name)) {
                auto& m = classes.at(curr).methods.at(method_name);
                actual_method_type = m.return_type;
                formal_types = m.arg_types;
                method_found = true;
                break;
            }
            curr = classes.at(curr).parent;
        }
        
        if (!method_found) {
            if (!target_had_error) {
                if (ctx->AT()) {
                    errors.push_back("Method `" + method_name + "` not defined for type `" + lookup_type + "` in static dispatch");
                } else {
                    errors.push_back("Method `" + method_name + "` not defined for type `" + lookup_type + "` in dynamic dispatch");
                }
            }
        } else {
            if (args.size() != formal_types.size()) {
                errors.push_back("Method `" + method_name + "` of type `" + lookup_type + "` called with the wrong number of arguments; " + to_string(formal_types.size()) + " arguments expected, but " + to_string(args.size()) + " provided");
            } else {
                for (size_t i = 0; i < args.size(); ++i) {
                    string arg_type = type_names[args[i]->get_type()];
                    if (!conform(arg_type, formal_types[i])) {
                        errors.push_back("Invalid call to method `" + method_name + "` from class `" + lookup_type + "`:");
                        errors.push_back("  `" + arg_type + "` is not a subtype of `" + formal_types[i] + "`: argument at position " + to_string(i) + " (0-indexed) has the wrong type");
                    }
                }
            }
        }
        
        string return_type = actual_method_type;
        if (return_type == "SELF_TYPE") {
            return_type = target_type; 
        }
        
        if (!method_found) {
            return_type = "Object";
        }
        
        if (ctx->AT()) {
             scratchpad.push(make_unique<StaticDispatch>(move(target), type_ids.at(static_type), method_name, move(args), type_ids.at(return_type)));
        } else {
             scratchpad.push(make_unique<DynamicDispatch>(move(target), method_name, move(args), type_ids.at(return_type)));
        }
        return nullptr;
    }
    
    // Operations
    if (ctx->PLUS() || ctx->MINUS() || ctx->STAR() || ctx->SLASH()) {
        auto l = visitExprAndAssertOk(ctx->expr(0));
        auto r = visitExprAndAssertOk(ctx->expr(1));
        
        string l_type = type_names[l->get_type()];
        string r_type = type_names[r->get_type()];
        
        if (l_type != "Int") {
            errors.push_back("Left-hand-side of arithmetic expression is not of type `Int`, but of type `" + l_type + "`");
        }
        if (r_type != "Int") {
            errors.push_back("Right-hand-side of arithmetic expression is not of type `Int`, but of type `" + r_type + "`");
        }
        
        Arithmetic::Kind op;
        if (ctx->PLUS()) op = Arithmetic::Kind::Addition;
        else if (ctx->MINUS()) op = Arithmetic::Kind::Subtraction;
        else if (ctx->STAR()) op = Arithmetic::Kind::Multiplication;
        else op = Arithmetic::Kind::Division;
        
        scratchpad.push(make_unique<Arithmetic>(move(l), move(r), op, type_ids.at("Int")));
        return nullptr;
    }
    
    // Comparison
    if (ctx->LT() || ctx->LE() || ctx->EQ()) {
        auto l = visitExprAndAssertOk(ctx->expr(0));
        auto r = visitExprAndAssertOk(ctx->expr(1));
        
        string l_type = type_names[l->get_type()];
        string r_type = type_names[r->get_type()];
        
        if (ctx->EQ()) {
            if ((l_type == "Int" || l_type == "String" || l_type == "Bool" ||
                 r_type == "Int" || r_type == "String" || r_type == "Bool") &&
                l_type != r_type) {
                errors.push_back("A `" + l_type + "` can only be compared to another `" + l_type + "` and not to a `" + r_type + "`");
            }
            scratchpad.push(make_unique<EqualityComparison>(move(l), move(r), type_ids.at("Bool")));
        } else {
            if (l_type != "Int") {
                errors.push_back("Left-hand-side of integer comparison is not of type `Int`, but of type `" + l_type + "`");
            }
            if (r_type != "Int") {
                errors.push_back("Right-hand-side of integer comparison is not of type `Int`, but of type `" + r_type + "`");
            }
            if (ctx->LT()) {
                scratchpad.push(make_unique<IntegerComparison>(move(l), move(r), IntegerComparison::Kind::LessThan, type_ids.at("Bool")));
            } else {
                scratchpad.push(make_unique<IntegerComparison>(move(l), move(r), IntegerComparison::Kind::LessThanEqual, type_ids.at("Bool")));
            }
        }
        return nullptr;
    }
    
    // Not
    if (ctx->NOT()) {
        auto e = visitExprAndAssertOk(ctx->expr(0));
        if (type_names[e->get_type()] != "Bool") {
            errors.push_back("Argument of boolean negation is not of type `Bool`, but of type `" + type_names[e->get_type()] + "`");
        }
        scratchpad.push(make_unique<BooleanNegation>(move(e), type_ids.at("Bool")));
        return nullptr;
    }
    
    // Neg
    if (ctx->TILDE()) {
        auto e = visitExprAndAssertOk(ctx->expr(0));
        if (type_names[e->get_type()] != "Int") {
            errors.push_back("Argument of integer negation is not of type `Int`, but of type `" + type_names[e->get_type()] + "`");
        }
        scratchpad.push(make_unique<IntegerNegation>(move(e), type_ids.at("Int")));
        return nullptr;
    }
    
    // IsVoid
    if (ctx->ISVOID()) {
        auto e = visitExprAndAssertOk(ctx->expr(0));
        scratchpad.push(make_unique<IsVoid>(move(e), type_ids.at("Bool")));
        return nullptr;
    }
    
    // Paren
    if (ctx->children.size() == 3 && ctx->children[0]->getText() == "(") {
        auto e = visitExprAndAssertOk(ctx->expr(0));
        int type = e->get_type();
        scratchpad.push(make_unique<ParenthesizedExpr>(move(e), type));
        return nullptr;
    }
    
    scratchpad.push(make_unique<Expr>(type_ids.at("Object")));
    return nullptr;
}
