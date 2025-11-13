parser grammar CoolParser;

options { tokenVocab=CoolLexer; }

program: (class SEMI)+ ;

class  : CLASS TYPEID (INHERITS TYPEID)? LBRACE (feature SEMI)* RBRACE ;

method : OBJECTID LPAREN (formal (COMMA formal)*)? RPAREN COLON TYPEID LBRACE expr RBRACE ;

attr   : OBJECTID COLON TYPEID (ASSIGN expr)? ;

feature : method | attr ;

formal : OBJECTID COLON TYPEID ;

let_binding : OBJECTID COLON TYPEID (ASSIGN expr)? ;

expr   : NEW TYPEID                                                                # new
       | IF expr THEN expr ELSE expr FI                                            # cond
       | WHILE expr LOOP expr POOL                                                 # loop
       | LBRACE (expr SEMI)+ RBRACE                                                # block
       | CASE expr OF (OBJECTID COLON TYPEID DARROW expr SEMI)+ ESAC               # case
       | LET let_binding (COMMA let_binding)* IN expr                              # let
       | LPAREN expr RPAREN                                                        # paren
       | OBJECTID                                                                  # object
       | INT_CONST                                                                 # int
       | STR_CONST                                                                 # string
       | BOOL_CONST                                                                # bool
       | OBJECTID LPAREN (expr (COMMA expr)*)? RPAREN                              # dispatch
       | expr (AT TYPEID)? DOT OBJECTID LPAREN (expr (COMMA expr)*)? RPAREN        # static_dispatch
       | TILDE expr                                                                # neg
       | ISVOID expr                                                               # isvoid
       | expr MULT expr                                                            # mult
       | expr DIV expr                                                             # div
       | expr PLUS expr                                                            # plus
       | expr MINUS expr                                                           # minus
       | expr (LE | EQ | LT) expr                                                  # comp
       | NOT expr                                                                  # not
       | <assoc=right> OBJECTID ASSIGN expr                                        # assign
       ;
