lexer grammar CoolLexer;

options { language = Cpp; }

// Тази част позволява дефинирането на допълнителен код, който да се запише в
// генерирания CoolLexer.h.
//
// Коментарите вътре са на английски, понеже ANTLR4 иначе ги омазва.
@lexer::members {
    // Maximum length of a constant string literal (CSL) excluding the implicit
    // null character that marks the end of the string.
    const unsigned MAX_STR_CONST = 1024;
    // Stores the current CSL as it's being built.
    std::vector<char> string_buffer;

    void clear_string_buffer() {
        string_buffer.clear();
    }

    void add_char_to_string_buffer(char c) {
        if (string_buffer.size() >= MAX_STR_CONST) {
        } else {
            string_buffer.push_back(convert_escape_sequence(c));
        }
    }

    char convert_escape_sequence(char esc_char) {
        switch (esc_char) {
            case 'n': return '\n';
            case 't': return '\t';
            case 'b': return '\b';
            case 'f': return '\f';
            case 'r': return '\r';
            case '\\': return '\\';
            case '"': return '"';
            default: return esc_char;
        }
    }

    // ----------------------- booleans -------------------------

    // A map from token ids to boolean values
    std::map<int, bool> bool_values;

    void assoc_bool_with_token(bool value) {
        bool_values[tokenStartCharIndex] = value;
    }

    bool get_bool_value(int token_start_char_index) {
        return bool_values.at(token_start_char_index);
    }

    // Add your own custom code to be copied to CoolLexer.h here.


}


// --------------- ключови думи -------------------

CLASS : [cC] [lL] [aA] [sS] [sS];
ELSE  : [eE] [lL] [sS] [eE];
ESAC  : [eE] [sS] [aA] [cC];
FI    : [fF] [iI];
IF    : [iI] [fF];
IN    : [iI] [nN];
INHERITS : [iI] [nN] [hH] [eE] [rR] [iI] [tT] [sS];
LET   : [lL] [eE] [tT];
LOOP  : [lL] [oO] [oO] [pP];
POOL  : [pP] [oO] [oO] [lL];
THEN  : [tT] [hH] [eE] [nN];
WHILE : [wW] [hH] [iI] [lL] [eE];
CASE  : [cC] [aA] [sS] [eE];
OF    : [oO] [fF];
NEW   : [nN] [eE] [wW];
ISVOID : [iI] [sS] [vV] [oO] [iI] [dD];
NOT   : [nN] [oO] [tT];

// --------------- булеви константи -------------------

BOOL_CONST : 't' [rR] [uU] [eE]  { assoc_bool_with_token(true); }
           | 'f' [aA] [lL] [sS] [eE] { assoc_bool_with_token(false); };

// --------------- числови константи -------------------

INT_CONST : '0' | [1-9] [0-9]*;

// --------------- идентификатори -------------------

TYPEID : [A-Z] [a-zA-Z0-9_]*;
OBJECTID : [a-z] [a-zA-Z0-9_]*;

// --------------- прости жетони -------------------

// Добавете тук останалите жетони, които представляват просто текст.
SEMI   : ';';
DARROW : '=>';
COLON : ':';
COMMA  : ',';
LPAREN : '(';
RPAREN : ')';
LBRACE : '{';
RBRACE : '}';
PLUS   : '+';
MINUS  : '-';
MULT   : '*';
DIV    : '/';
LT     : '<';
LE     : '<=';
EQ     : '=';
ASSIGN : '<-';
DOT    : '.';
AT     : '@';
TILDE  : '~';

// --------------- коментари -------------------

LINE_COMMENT : '--' .*? '\n' -> skip;
ML_COMMENT_START : '(*' ->pushMode(COMMENT_MODE), skip;

// --------------- интервали -------------------

WS : [ \t\r\n\f]+ -> skip;

// --------------- текстови низове -------------------


mode COMMENT_MODE;
NESTED_ML_COMMENT_START : '(*' -> pushMode(COMMENT_MODE), skip;
ML_COMMENT_END : '*)' -> popMode, skip;

// ML_COMMENT_ERROR : EOF -> { semantic_value = "EOF in comment"; }, type(ERROR), popMode;

ML_COMMENT_ANY : . -> skip;


// --------------- грешки -------------------

    //   | BAD_ML_COMMENT { notifyListeners("EOF in comment"); };
