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

#include <iostream>

using namespace std;

vector<string> TypeChecker::check(CoolParser *parser) {
    visit(parser->program());
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
    
    for (auto attr : ctx->attr()) {
        any res = visit(attr);
        if (res.has_value()) {
            unique_ptr<Attribute> a(any_cast<Attribute*>(res));
            typed_class.attributes.add(move(*a));
        }
    }
    for (auto method : ctx->method()) {
        any res = visit(method);
        if (res.has_value()) {
            unique_ptr<Method> m(any_cast<Method*>(res));
            typed_class.methods.add_method(move(*m));
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
    
    for (auto formal : ctx->formal()) {
        string name = formal->OBJECTID()->getText();
        string type = formal->TYPEID()->getText();
        if (name == "self") {
            errors.push_back("self cannot be the name of a formal parameter");
        }
        if (symbol_table.back().contains(name)) {
            errors.push_back("Formal parameter " + name + " is multiply defined");
        }
        addSymbol(name, type);
        arg_names.push_back(name);
        arg_types.push_back(type);
    }
    
    any body_res = visit(ctx->expr());
    unique_ptr<Expr> body(any_cast<Expr*>(body_res));
    string body_type = type_names[body->get_type()];
    string return_type = ctx->TYPEID()->getText();
    
    if (!conform(body_type, return_type)) {
        errors.push_back("In class " + current_class + " method " + method_name + ": " + body_type + " is not " + return_type + ": type of method body is not a subtype of return type");
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
    
    unique_ptr<Expr> init = nullptr;
    if (ctx->ASSIGN()) {
        any init_result = visit(ctx->expr());
        init.reset(any_cast<Expr*>(init_result));
        string init_type = type_names[init->get_type()];
        if (!conform(init_type, type)) {
            errors.push_back("Inferred type " + init_type + " of initialization of attribute " + name + " does not conform to declared type " + type + ".");
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
        return (Expr*)new IntConstant(stoi(ctx->INT_CONST()->getText()), type_ids.at("Int"));
    }
    if (ctx->STR_CONST()) {
        return (Expr*)new StringConstant(ctx->STR_CONST()->getText(), type_ids.at("String"));
    }
    if (ctx->BOOL_CONST()) {
        return (Expr*)new BoolConstant(ctx->BOOL_CONST()->getText() == "true", type_ids.at("Bool"));
    }
    
    // Variable
    if (!ctx->OBJECTID().empty() && ctx->children.size() == 1) {
        string name = ctx->OBJECTID(0)->getText();
        string type = lookupSymbol(name);
        if (type == "") {
            errors.push_back("Undeclared identifier " + name + ".");
            type = "Object"; 
        }
        return (Expr*)new ObjectReference(name, type_ids.at(type));
    }
    
    // Assignment
    if (ctx->ASSIGN()) {
        string name = ctx->OBJECTID(0)->getText();
        if (name == "self") {
            errors.push_back("Cannot assign to 'self'.");
        }
        
        any val_res = visit(ctx->expr(0));
        unique_ptr<Expr> val(any_cast<Expr*>(val_res));
        string val_type = type_names[val->get_type()];
        
        string var_type = lookupSymbol(name);
        if (var_type == "") {
            errors.push_back("Assignment to undeclared variable " + name + ".");
            var_type = "Object";
        } else {
            if (!conform(val_type, var_type)) {
                errors.push_back("Type " + val_type + " of assigned expression does not conform to declared type " + var_type + " of identifier " + name + ".");
            }
        }
        
        return (Expr*)new Assignment(name, move(val), type_ids.at(val_type));
    }
    
    // New
    if (ctx->NEW()) {
        string type = ctx->TYPEID(0)->getText();
        if (type != "SELF_TYPE" && !classes.contains(type)) {
            errors.push_back("'new' used with undefined class " + type + ".");
            type = "Object";
        }
        return (Expr*)new NewObject(type_ids.at(type));
    }
    
    // If
    if (ctx->IF()) {
        any pred_res = visit(ctx->expr(0));
        unique_ptr<Expr> pred(any_cast<Expr*>(pred_res));
        string pred_type = type_names[pred->get_type()];
        if (pred_type != "Bool") {
            errors.push_back("Predicate of 'if' does not have type Bool.");
        }
        
        any then_res = visit(ctx->expr(1));
        unique_ptr<Expr> then_e(any_cast<Expr*>(then_res));
        string then_type = type_names[then_e->get_type()];
        
        any else_res = visit(ctx->expr(2));
        unique_ptr<Expr> else_e(any_cast<Expr*>(else_res));
        string else_type = type_names[else_e->get_type()];
        
        string join_type = lub(then_type, else_type);
        return (Expr*)new IfThenElseFi(move(pred), move(then_e), move(else_e), type_ids.at(join_type));
    }
    
    // While
    if (ctx->WHILE()) {
        any pred_res = visit(ctx->expr(0));
        unique_ptr<Expr> pred(any_cast<Expr*>(pred_res));
        string pred_type = type_names[pred->get_type()];
        if (pred_type != "Bool") {
            errors.push_back("Loop condition does not have type Bool.");
        }
        
        any body_res = visit(ctx->expr(1));
        unique_ptr<Expr> body(any_cast<Expr*>(body_res));
        
        return (Expr*)new WhileLoopPool(move(pred), move(body), type_ids.at("Object"));
    }
    
    // Block
    if (ctx->children.size() > 2 && ctx->children[0]->getText() == "{") {
        vector<unique_ptr<Expr>> exprs;
        string last_type = "Object"; 
        for (auto e : ctx->expr()) {
            any res = visit(e);
            unique_ptr<Expr> expr(any_cast<Expr*>(res));
            last_type = type_names[expr->get_type()];
            exprs.push_back(move(expr));
        }
        return (Expr*)new Sequence(move(exprs), type_ids.at(last_type));
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
                errors.push_back("Class " + type + " of let-bound identifier " + name + " is undefined.");
                type = "Object";
            }
            
            unique_ptr<Expr> init = nullptr;
            if (v->ASSIGN()) {
                any init_res = visit(v->expr());
                init.reset(any_cast<Expr*>(init_res));
                string init_type = type_names[init->get_type()];
                if (!conform(init_type, type)) {
                    errors.push_back("Inferred type " + init_type + " of initialization of " + name + " does not conform to identifier's declared type " + type + ".");
                }
            }
            
            addSymbol(name, type);
            decls.push_back(unique_ptr<Vardecl>(new Vardecl(name, move(init), type_ids.at(type))));
        }
        
        any body_res = visit(ctx->expr(0)); 
        unique_ptr<Expr> body(any_cast<Expr*>(body_res));
        
        exitScope();
        
        return (Expr*)new LetIn(move(decls), move(body), body->get_type());
    }
    
    // Case
    if (ctx->CASE()) {
        any expr_res = visit(ctx->expr(0));
        unique_ptr<Expr> expr(any_cast<Expr*>(expr_res));
        
        vector<CaseOfEsac::Case> cases;
        string join_type = "";
        
        size_t num_branches = ctx->OBJECTID().size();
        set<string> branch_types;
        
        for (size_t i = 0; i < num_branches; ++i) {
            string name = ctx->OBJECTID(i)->getText();
            string type = ctx->TYPEID(i)->getText();
            
            if (type == "SELF_TYPE") {
                errors.push_back("Identifier " + name + " declared with type SELF_TYPE in case branch.");
            } else if (!classes.contains(type)) {
                errors.push_back("Class " + type + " of case branch is undefined.");
            }
            
            if (branch_types.contains(type)) {
                errors.push_back("Duplicate branch " + type + " in case statement.");
            }
            branch_types.insert(type);
            
            enterScope();
            addSymbol(name, type);
            
            any branch_res = visit(ctx->expr(i+1));
            unique_ptr<Expr> branch_expr(any_cast<Expr*>(branch_res));
            string branch_type = type_names[branch_expr->get_type()];
            
            if (join_type == "") join_type = branch_type;
            else join_type = lub(join_type, branch_type);
            
            cases.emplace_back(name, type_ids.at(type), move(branch_expr));
            
            exitScope();
        }
        
        return (Expr*)new CaseOfEsac(move(expr), move(cases), ctx->getStart()->getLine(), type_ids.at(join_type));
    }
    
    // Dispatch
    if (ctx->DOT()) {
        any target_res = visit(ctx->expr(0));
        unique_ptr<Expr> target(any_cast<Expr*>(target_res));
        string target_type = type_names[target->get_type()];
        
        string method_name = ctx->OBJECTID(0)->getText();
        string static_type = "";
        if (ctx->AT()) {
            static_type = ctx->TYPEID(0)->getText();
            if (static_type == "SELF_TYPE") {
                errors.push_back("Static dispatch to SELF_TYPE.");
                static_type = "Object";
            } else if (!classes.contains(static_type)) {
                errors.push_back("Static dispatch to undefined class " + static_type + ".");
                static_type = "Object";
            } else if (!conform(target_type, static_type)) {
                errors.push_back("Expression type " + target_type + " does not conform to declared static dispatch type " + static_type + ".");
            }
        }
        
        vector<unique_ptr<Expr>> args;
        for (size_t i = 1; i < ctx->expr().size(); ++i) {
            any arg_res = visit(ctx->expr(i));
            args.push_back(unique_ptr<Expr>(any_cast<Expr*>(arg_res)));
        }
        
        string lookup_type = static_type.empty() ? target_type : static_type;
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
            errors.push_back("Dispatch to undefined method " + method_name + ".");
        } else {
            if (args.size() != formal_types.size()) {
                errors.push_back("Method " + method_name + " called with wrong number of arguments.");
            } else {
                for (size_t i = 0; i < args.size(); ++i) {
                    string arg_type = type_names[args[i]->get_type()];
                    if (!conform(arg_type, formal_types[i])) {
                        errors.push_back("In call of method " + method_name + ", type " + arg_type + " of parameter " + "..." + " does not conform to declared type " + formal_types[i] + ".");
                    }
                }
            }
        }
        
        string return_type = actual_method_type;
        if (return_type == "SELF_TYPE") {
            return_type = target_type; 
        }
        
        if (ctx->AT()) {
             return (Expr*)new StaticDispatch(move(target), type_ids.at(static_type), method_name, move(args), type_ids.at(return_type));
        } else {
             return (Expr*)new DynamicDispatch(move(target), method_name, move(args), type_ids.at(return_type));
        }
    }
    
    // Operations
    if (ctx->PLUS() || ctx->MINUS() || ctx->STAR() || ctx->SLASH()) {
        any l_res = visit(ctx->expr(0));
        any r_res = visit(ctx->expr(1));
        unique_ptr<Expr> l(any_cast<Expr*>(l_res));
        unique_ptr<Expr> r(any_cast<Expr*>(r_res));
        
        string l_type = type_names[l->get_type()];
        string r_type = type_names[r->get_type()];
        
        if (l_type != "Int" || r_type != "Int") {
            errors.push_back("non-Int arguments: " + l_type + " + " + r_type);
        }
        
        Arithmetic::Kind op;
        if (ctx->PLUS()) op = Arithmetic::Kind::Addition;
        else if (ctx->MINUS()) op = Arithmetic::Kind::Subtraction;
        else if (ctx->STAR()) op = Arithmetic::Kind::Multiplication;
        else op = Arithmetic::Kind::Division;
        
        return (Expr*)new Arithmetic(move(l), move(r), op, type_ids.at("Int"));
    }
    
    // Comparison
    if (ctx->LT() || ctx->LE() || ctx->EQ()) {
        any l_res = visit(ctx->expr(0));
        any r_res = visit(ctx->expr(1));
        unique_ptr<Expr> l(any_cast<Expr*>(l_res));
        unique_ptr<Expr> r(any_cast<Expr*>(r_res));
        
        string l_type = type_names[l->get_type()];
        string r_type = type_names[r->get_type()];
        
        if (ctx->EQ()) {
            if ((l_type == "Int" || l_type == "String" || l_type == "Bool" ||
                 r_type == "Int" || r_type == "String" || r_type == "Bool") &&
                l_type != r_type) {
                errors.push_back("Illegal comparison with a basic type.");
            }
            return (Expr*)new EqualityComparison(move(l), move(r), type_ids.at("Bool"));
        } else {
            if (l_type != "Int" || r_type != "Int") {
                errors.push_back("non-Int arguments: " + l_type + " < " + r_type);
            }
            if (ctx->LT()) return (Expr*)new IntegerComparison(move(l), move(r), IntegerComparison::Kind::LessThan, type_ids.at("Bool"));
            else return (Expr*)new IntegerComparison(move(l), move(r), IntegerComparison::Kind::LessThanEqual, type_ids.at("Bool"));
        }
    }
    
    // Not
    if (ctx->NOT()) {
        any e_res = visit(ctx->expr(0));
        unique_ptr<Expr> e(any_cast<Expr*>(e_res));
        if (type_names[e->get_type()] != "Bool") {
            errors.push_back("Argument of 'not' has type " + type_names[e->get_type()] + " instead of Bool.");
        }
        return (Expr*)new BooleanNegation(move(e), type_ids.at("Bool"));
    }
    
    // Neg
    if (ctx->TILDE()) {
        any e_res = visit(ctx->expr(0));
        unique_ptr<Expr> e(any_cast<Expr*>(e_res));
        if (type_names[e->get_type()] != "Int") {
            errors.push_back("Argument of '~' has type " + type_names[e->get_type()] + " instead of Int.");
        }
        return (Expr*)new IntegerNegation(move(e), type_ids.at("Int"));
    }
    
    // IsVoid
    if (ctx->ISVOID()) {
        any e_res = visit(ctx->expr(0));
        unique_ptr<Expr> e(any_cast<Expr*>(e_res));
        return (Expr*)new IsVoid(move(e), type_ids.at("Bool"));
    }
    
    // Paren
    if (ctx->children.size() == 3 && ctx->children[0]->getText() == "(") {
        any e_res = visit(ctx->expr(0));
        unique_ptr<Expr> e(any_cast<Expr*>(e_res));
        return (Expr*)new ParenthesizedExpr(move(e), e->get_type());
    }
    
    return (Expr*)new Expr(type_ids.at("Object")); // Fallback
}
