lexer grammar CoolLexer;

@header { 
    #include <iomanip>
}

options { language = Cpp; }

@lexer::members {
    // Maximum length of a constant string literal (CSL) excluding the implicit
    // null character that marks the end of the string.
    const unsigned MAX_STR_CONST = 1024;
    // Stores the current CSL as it's being built.
    std::vector<char> string_buffer;

    void clear_string_buffer() {
        string_buffer.clear();
    }

    std::string convert_non_printable_to_hex(char c) {
        std::stringstream ss;
        ss << "<0x"
            << std::hex << std::setw(2)
            << std::setfill('0')
            << (static_cast<int>(c) & 0xFF)
            << ">";
        return ss.str();
    }

    void add_non_printable_char_to_buffer(char c) {
        std::string hex_representation = convert_non_printable_to_hex(c);
        for (char ch : hex_representation) {
            add_char_to_string_buffer(ch);
        }
    }

    void add_char_to_string_buffer(char c) {
        if (string_buffer.size() >= MAX_STR_CONST) {
            register_error(ErrorCode::STRING_TOO_LONG);
            setType(ERROR);
        } else {
            string_buffer.push_back(c);
        }
    }

    void add_escaped_char_to_string_buffer(char c) {
        if (string_buffer.size() >= MAX_STR_CONST) {
            register_error(ErrorCode::STRING_TOO_LONG);
            setType(ERROR);
        } else {
            switch (c) {
                case '\n': {
                    string_buffer.push_back('\\');
                    string_buffer.push_back('n');
                    break;
                }
                case '\t': {
                    string_buffer.push_back('\\');
                    string_buffer.push_back('t');
                    break;
                }
                case '\b': {
                    string_buffer.push_back('\\');
                    string_buffer.push_back('b');
                    break;
                }
                case '\f': {
                    string_buffer.push_back('\\');
                    string_buffer.push_back('f');
                    break;
                }
                case '\r': {    
                    string_buffer.push_back('\\');
                    string_buffer.push_back('r');
                    break;
                }
                case 'n':
                case 't':
                case 'b':
                case 'f':
                case 'r':
                case '\\':
                case '"': {
                    string_buffer.push_back('\\');
                    string_buffer.push_back(c);
                    break;
                }
                default:
                    string_buffer.push_back(c);
                    break;
            }
        }   
    }

    std::map<int, std::string> string_values;

    void assoc_string_with_token() {
        string_values[tokenStartCharIndex] = std::string(string_buffer.begin(), string_buffer.end());
    }

    std::string get_string_value(int token_start_char_index) {
        return string_values.at(token_start_char_index);
    }

    std::map<int, bool> bool_values;

    void assoc_bool_with_token(bool value) {
        bool_values[tokenStartCharIndex] = value;
    }

    bool get_bool_value(int token_start_char_index) {
        return bool_values.at(token_start_char_index);
    }


    enum class ErrorCode {
        UNMATCHED_COMMENT,
        EOF_IN_STRING,
        EOF_IN_COMMENT,
        STRING_TOO_LONG,
        INVALID_ESCAPE_SEQUENCE,
        INVALID_SYMBOL,
        ESCAPED_NULL,
        NULL_INSIDE_STRING,
        INVALID_SYMBOL_NON_PRINTABLE,
    };

    std::map<int, ErrorCode> err_codes;

    void register_error(ErrorCode code) {
        err_codes[tokenStartCharIndex] = code;
    }

    ErrorCode get_error_code(int token_start_char_index) {
        return err_codes.at(token_start_char_index);
    }

}


// --------------- keywords -------------------

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

// --------------- boolean constants -------------------

BOOL_CONST : 't' [rR] [uU] [eE]  { assoc_bool_with_token(true); }
           | 'f' [aA] [lL] [sS] [eE] { assoc_bool_with_token(false); };

// --------------- integer constants -------------------

INT_CONST : [0-9]+;

// I think this is more correct to do, but I must follow the tests
// INCORRECT_INT_CONST : '0' [0-9]+ {
//     register_error(ErrorCode::INVALID_INT_LITERAL);
//     setType(ERROR);
// };

// --------------- identifiers -------------------
TYPEID : [A-Z] [a-zA-Z0-9_]*;
OBJECTID : [a-z] [a-zA-Z0-9_]*;

// --------------- simple tokens -------------------
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

// --------------- comments -------------------

LINE_COMMENT : '--' ~[\r\n]* -> skip;
ML_COMMENT_START : '(*' -> pushMode(ML_COMMENT_MODE), skip;
ML_COMMENT_UNMATCHED_END : '*)' { 
    register_error(ErrorCode::UNMATCHED_COMMENT);
    setType(ERROR); 
};

// --------------- whitespaces -------------------

WS : [ \t\r\n\f]+ -> skip;
NON_PRINTABLES : ~[\u0020-\u007E\t\b\n\f] {
    register_error(ErrorCode::INVALID_SYMBOL_NON_PRINTABLE);
    setType(ERROR);
};

// --------------- strings -------------------
STR_CONST :;
STR_START : '"' { clear_string_buffer(); } -> more, pushMode(STR_MODE);

// --------------- all errors -------------------
ERROR : . { register_error(ErrorCode::INVALID_SYMBOL); };


// --------------- modes -------------------
mode ML_COMMENT_MODE;
NESTED_ML_COMMENT_START : '(*' -> pushMode(ML_COMMENT_MODE), skip;
ML_COMMENT_END : '*)' -> popMode, skip;
ML_COMMENT_ANY : . -> skip;
ML_COMMENT_EOF : . EOF { 
    register_error(ErrorCode::EOF_IN_COMMENT);
    setType(ERROR); 
};

mode STR_MODE;
STR_END : '"' { 
    if (string_buffer.size() > MAX_STR_CONST) {
        clear_string_buffer();
        register_error(ErrorCode::STRING_TOO_LONG);
        setType(ERROR);
    } else {
        setType(STR_CONST);
        assoc_string_with_token();
    }
} -> popMode;
NULL_CHAR : '\u0000' {
    clear_string_buffer();
    register_error(ErrorCode::NULL_INSIDE_STRING);
    setType(ERROR);
} -> popMode, mode(EXIT_STR_MODE);
ESCAPED_NULL_CHAR : '\\' '\u0000' {
    register_error(ErrorCode::ESCAPED_NULL);
    setType(ERROR);
} -> popMode, mode(EXIT_STR_MODE);
NEWLINE_INSIDE : [\n] {
    clear_string_buffer();
    register_error(ErrorCode::INVALID_ESCAPE_SEQUENCE);
    setType(ERROR);
} -> popMode;
VALID_ESCAPE_SEQUENCE_INSIDE : [\t\b] { add_escaped_char_to_string_buffer(getText()[0]); } -> skip;
NON_PRINTABLES_INSIDE_STR : '\\'? ~[\u0020-\u007E\t\b\n] {
    if (getText()[0] == '\\')
        add_non_printable_char_to_buffer(getText()[1]);
    else
        add_non_printable_char_to_buffer(getText()[0]);
} -> skip;
ESCAPED_CHAR : '\\' . { add_escaped_char_to_string_buffer(getText()[1]); } -> skip;
UNTERMINATED_ESCAPED_CHAR : '\\' EOF {
    clear_string_buffer();
    register_error(ErrorCode::EOF_IN_STRING);
    setType(ERROR); 
};
TEXT : ~["\\\n\r\f\b\t] {
    if (string_buffer.size() > MAX_STR_CONST) {
        clear_string_buffer();
        register_error(ErrorCode::STRING_TOO_LONG);
        setType(ERROR);
    } else {
        add_char_to_string_buffer(getText()[0]);
    }
} -> skip;
EOF_INSIDE : ~["] EOF { 
    clear_string_buffer();
    register_error(ErrorCode::EOF_IN_STRING);
    setType(ERROR); 
};

mode EXIT_STR_MODE;
TEXT_AFTER_ERROR : .*? '"' -> skip, mode(DEFAULT_MODE);
EOF_AFTER_ERROR: . EOF -> skip;


