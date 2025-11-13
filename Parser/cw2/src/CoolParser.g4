parser grammar CoolParser;

options { tokenVocab=CoolLexer; }

program: (class SEMI)+ ;

class  : CLASS TYPEID (INHERITS TYPEID)? LBRACE (feature SEMI)* RBRACE ;

method : OBJECTID LPAREN (formal (COMMA formal)*)? RPAREN COLON TYPEID LBRACE expr RBRACE ;

attr   : OBJECTID COLON TYPEID (ASSIGN expr)? ;

feature : method | attr ;

formal : OBJECTID COLON TYPEID ;

let_binding : OBJECTID COLON TYPEID (ASSIGN expr)? ;

expr   : NEW TYPEID                                                         # new
       | IF expr THEN expr ELSE expr FI                                     # cond
       | WHILE expr LOOP expr POOL                                          # loop
       | LBRACE (expr SEMI)+ RBRACE                                         # block
       | CASE expr OF (OBJECTID COLON TYPEID DARROW expr SEMI)+ ESAC        # case
       | LPAREN expr RPAREN                                                 # paren
       | OBJECTID                                                           # object
       | INT_CONST                                                          # int
       | STR_CONST                                                          # string
       | BOOL_CONST                                                         # bool
       | expr AT TYPEID DOT OBJECTID LPAREN (expr (COMMA expr)*)? RPAREN    # statdispatch
       | expr DOT OBJECTID LPAREN (expr (COMMA expr)*)? RPAREN              # dispatch
       | OBJECTID LPAREN (expr (COMMA expr)*)? RPAREN                       # selfdispatch
       | TILDE expr                                                         # neg
       | ISVOID expr                                                        # isvoid
       | expr (MULT | DIV) expr                                             # multdiv
       | expr (PLUS | MINUS) expr                                           # subadd
       | expr (LE | EQ | LT) expr                                           # comp
       | NOT expr                                                           # not
       | LET let_binding (COMMA let_binding)* IN expr                       # let
       | <assoc=right> OBJECTID ASSIGN expr                                 # assign
       ;
