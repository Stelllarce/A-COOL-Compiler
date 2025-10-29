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
            register_error(ErrorCode::STRING_TOO_LONG);
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

    std::string get_string_value(int token_start_char_index) {
        return std::string(string_buffer.begin(), string_buffer.end());
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

    enum class ErrorCode {
        UNMATCHED_COMMENT,
        EOF_IN_STRING,
        EOF_IN_COMMENT,
        STRING_TOO_LONG,
        INVALID_ESCAPE_SEQUENCE,
        INVALID_SYMBOL
    };

    std::map<int, ErrorCode> err_codes;

    void register_error(ErrorCode code) {
        err_codes[tokenStartCharIndex] = code;
    }

    ErrorCode get_error_code(int token_start_char_index) {
        return err_codes.at(token_start_char_index);
    }

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

LINE_COMMENT : '--' ~[\r\n]* -> skip;
ML_COMMENT_START : '(*' ->pushMode(ML_COMMENT_MODE), skip;
ML_COMMENT_UNMATCHED_END : '*)' { 
    register_error(ErrorCode::UNMATCHED_COMMENT);
    setType(ERROR); 
};

// --------------- интервали -------------------

WS : [ \t\r\n\f]+ -> skip;

// --------------- текстови низове -------------------
STR_CONST : ;
STR_START : '"' { clear_string_buffer(); } -> pushMode(STR_MODE), skip;

NULL_CHAR: '\u0000' {
    register_error(ErrorCode::INVALID_SYMBOL);
    setType(ERROR);
};

ERROR : . { register_error(ErrorCode::INVALID_SYMBOL); };

// --------------- допълнителна обработка на коментари -------------------
mode ML_COMMENT_MODE;
NESTED_ML_COMMENT_START : '(*' -> pushMode(ML_COMMENT_MODE), skip;
ML_COMMENT_END : '*)' -> popMode, skip;
ML_COMMENT_ANY : . -> skip;
ML_COMMENT_EOF : . EOF { 
    register_error(ErrorCode::EOF_IN_COMMENT);
    setType(ERROR); 
};


mode STR_MODE;

STR_END : '"' { setType(STR_CONST); } -> popMode;

STR_UNESCAPED_NEWLINE : '\n' {
    register_error(ErrorCode::INVALID_ESCAPE_SEQUENCE);
    setType(ERROR);
} -> popMode;

STR_ESCAPED_NEWLINE : '\\\n' {
    add_char_to_string_buffer('\n');
} -> skip;

STR_NULL_CHAR : '\u0000' {
    register_error(ErrorCode::INVALID_ESCAPE_SEQUENCE);
    setType(ERROR);
} -> popMode;

STR_EOF : EOF {
    register_error(ErrorCode::EOF_IN_STRING);
    setType(ERROR);
} -> popMode;

STR_ESCAPED_CHAR : '\\' . { 
    add_char_to_string_buffer(getText()[1]); 
} -> skip;

STR_SINGLE_CHAR : ~["\\\n] { 
    add_char_to_string_buffer(getText()[0]); 
} -> skip;

