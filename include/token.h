#ifndef TOKEN_H
#define TOKEN_H

#include <memory>
#include <string>
#include <ostream>

namespace soto
{

    enum token_kind
    {
        T_ENDL,
        T_LPAREN,
        T_RPAREN,

        T_TYPE, // Int, Float, String, Task, SomeCustomType

        T_AND,
        T_LOGICAL_AND,
        T_LOGICAL_OR,
        T_LOGICAL_NOT,
        T_XOR,

        T_EQUALITY,
        T_ASSIGN,
        T_LESS_OR_EQUAL,
        T_LESS_THAN,
        T_GREATER_OR_EQUAL,
        T_GREATER_THAN,

        T_AMPERSAND,
        T_SLITERAL,
        T_NLITERAL,
        T_BLITERAL, // boolean literal true/false

        T_PLUS,
        T_MINUS,
        T_STAR,
        T_FSLASH,
        T_BSLASH,
        T_COLON,
        T_COMMA,
        T_LCURLY,
        T_RCURLY,
        T_LSQUARE,
        T_RSQUARE,

        T_NEQ, // not equal !=
        T_NOT, // not !
        T_OR,  // ||
        T_PIPE,
        T_MODULO, // %
        T_DOT,

        T_IDENT,

        // stmt
        T_IF,
        T_DO,
        T_WHILE,
        T_RETURN,
        // T_ELSE_IF,
        T_ELSE,

        // wdl shits
        T_QUESTION, //?
        T_ELLIPSES, //~
        T_INPUT,
        T_OUTPUT,
        T_RUNTIME,
        T_META,
        T_COMMAND,
        T_THEN,
        T_LSHIFT,        //<<
        T_RSHIFT,        //>>
        T_LSHIFT_ASSIGN, //<<<
        T_RSHIFT_ASSIGN, //>>> this to close the T_LSHIFT_ASSIGN
        T_COMMENT,       // until when I can fix/ skipping comments, let's have this T_COMMENT here... for # or //

        T_EOF,
        T_ERROR,

    };

    inline const char *token_kind_to_string(token_kind kind)
    {
        switch (kind)
        {
        case T_ENDL:
            return "T_ENDL";
        case T_LPAREN:
            return "T_LPAREN";
        case T_RPAREN:
            return "T_RPAREN";
        case T_IDENT:
            return "T_IDENT";
        case T_AND:
            return "T_AND";
        case T_LOGICAL_AND:
            return "T_LOGICAL_AND";
        case T_LOGICAL_OR:
            return "T_LOGICAL_OR";
        case T_LOGICAL_NOT:
            return "T_LOGICAL_NOT";
        case T_XOR:
            return "T_XOR";
        case T_EQUALITY:
            return "T_EQUALITY";
        case T_ASSIGN:
            return "T_ASSIGN";
        case T_LESS_OR_EQUAL:
            return "T_LESS_OR_EQUAL";
        case T_LESS_THAN:
            return "T_LESS_THAN";
        case T_GREATER_OR_EQUAL:
            return "T_GREATER_OR_EQUAL";
        case T_GREATER_THAN:
            return "T_GREATER_THAN";
        case T_AMPERSAND:
            return "T_AMPERSAND";
        case T_SLITERAL:
            return "T_SLITERAL";
        case T_NLITERAL:
            return "T_NLITERAL";
        case T_PLUS:
            return "T_PLUS";
        case T_MINUS:
            return "T_MINUS";
        case T_STAR:
            return "T_STAR";
        case T_FSLASH:
            return "T_FSLASH";
        case T_BSLASH:
            return "T_BSLASH";
        case T_COLON:
            return "T_COLON";
        case T_COMMA:
            return "T_COMMA";
        case T_LCURLY:
            return "T_LCURLY";
        case T_RCURLY:
            return "T_RCURLY";
        case T_LSQUARE:
            return "T_LSQUARE";
        case T_RSQUARE:
            return "T_RSQUARE";
        case T_NEQ:
            return "T_NEQ";
        case T_NOT:
            return "T_NOT";
        case T_OR:
            return "T_OR";
        case T_PIPE:
            return "T_PIPE";
        case T_MODULO:
            return "T_MODULO";
        case T_DOT:
            return "T_DOT";
        case T_IF:
            return "T_IF";

        case T_TYPE:
            return "T_TYPE";
        case T_WHILE:
            return "T_WHILE";
        case T_RETURN:
            return "T_RETURN";
        case T_ELSE:
            return "T_ELSE";
        case T_THEN:
            return "T_THEN";

        case T_QUESTION:
            return "T_QUESTION";
        case T_ELLIPSES:
            return "T_ELLIPSES";
        case T_COMMENT:
            return "T_COMMENT";

        case T_BLITERAL:
            return "T_BLITERAL";
        case T_RUNTIME:
            return "T_RUNTIME";
        case T_COMMAND:
            return "T_COMMAND";

        case T_EOF:
            return "T_EOF";
        case T_ERROR:
            return "T_ERROR";
        default:
            return "UNKNOWN";
        }
    }

    // this token object...
    // ref
    struct token
    {
    public:
        token_kind kind;
        std::string lexeme;
        std::shared_ptr<token> next;
        std::shared_ptr<int> int_val;
        std::shared_ptr<double> float_val;
        int line; // current line this token is in..
        int column;

        token(token_kind kind, const std::string &literal) : kind(kind), lexeme(literal), next(nullptr), int_val(nullptr), float_val(nullptr) {}
        token(token_kind kind, const std::string &literal, int int_val) : kind(kind), lexeme(literal), next(nullptr), int_val(std::make_shared<int>(int_val)), float_val(nullptr) {}
        token(token_kind kind, const std::string &literal, double float_val) : kind(kind), lexeme(literal), next(nullptr), int_val(nullptr), float_val(std::make_shared<double>(float_val)) {}

        token() = default;
        ~token() = default;
        token(token &&) = default;
        token &operator=(token &&) = default;
        token(const token &) = delete;
        token &operator=(const token &) = delete;
    };

    inline std::ostream &operator<<(std::ostream &os, const token &tok)
    {
        os << "token(kind=" << token_kind_to_string(tok.kind) << ", lexeme=\"" << tok.lexeme << "\"" << ", line=" << tok.line << ", column=" << tok.column;

        if (tok.int_val)
        {
            os << ", int_val=" << *tok.int_val;
        }
        if (tok.float_val)
        {
            os << ", float_val=" << *tok.float_val;
        }
        os << ")";
        return os;
    }

}

#endif