parser grammar CoolParser;

options { tokenVocab=CoolLexer; }

program: (class SEMI)+ ;

class  : CLASS TYPEID (INHERITS TYPEID)? LBRACE (feature SEMI)* RBRACE ;

feature: OBJECTID LPAREN (formal (COMMA formal)*)? COLON TYPEID LBRACE expr RBRACE
       | OBJECTID COLON TYPEID (ASSIGN expr)?
       ;

formal : OBJECTID COLON TYPEID ;

expr   :   LPAREN expr RPAREN
       |   IF expr THEN expr ELSE expr FI
       |   WHILE expr LOOP expr POOL
       |   LBRACE (expr SEMI)+ RBRACE
       |   CASE expr OF (OBJECTID COLON TYPEID DARROW expr SEMI)+ ESAC
       |   LET OBJECTID COLON TYPEID (ASSIGN expr)? (COMMA OBJECTID COLON TYPEID (ASSIGN expr)?)* IN expr
       |   expr (AT TYPEID)? DOT OBJECTID LPAREN (expr (COMMA expr)*)? RPAREN
       |   NEW TYPEID
       |   ISVOID expr
       |   TILDE expr
       |   expr MULT expr
       |   expr DIV expr
       |   expr PLUS expr
       |   expr MINUS expr
       |   expr LT expr
       |   expr LE expr
       |   expr EQ expr
       |   NOT expr
       |   <assoc=right> OBJECTID ASSIGN expr
       |   OBJECTID
       |   INT_CONST
       |   STR_CONST
       |   BOOL_CONST
       ;
