#include "lexer.h"
#include <string>
#include <cctype>
#include <algorithm>
#include <exception>

#include <codecvt>
#include <locale>
#include <sstream>
#include "string_utils.h"

namespace soto
{

    lexer::lexer(std::string input)
        : source(std::move(input)), position(-2), line(std::make_shared<int>(1)), column(std::make_shared<int>(-1))
    {
        std::cout << "the input source to be tokenized is >>> " << source << std::endl;

        if (!source.empty())
        {
            source = canonicalize_source_str(source);
            c_char = source[0];
            n_char = (source.length() > 1) ? source[1] : '\0';
        }
        else
        {
            c_char = '\0';
            n_char = '\0';
        }

        next_token();
    }
    // this because we need because of peekability (I made this english up) of tokens inside the parser, almost unavoidable...or maybe not...
    std::unique_ptr<lexer> lexer::clone() const
    {
        return std::make_unique<lexer>(*this);
    }
    std::string lexer::canonicalize_source_str(const std::string &source)
    {
        std::string result;
        result.reserve(source.size());

        bool last_was_newline = false;

        for (size_t i = 0; i < source.length(); ++i)
        {
            if (source[i] == '\r')
            {
                if (i + 1 < source.length() && source[i + 1] == '\n')
                {
                    continue;
                }
                continue;
            }

            if (source[i] == '\n')
            {
                if (last_was_newline)
                {
                    continue; // Skip consecutive newlines
                }
                last_was_newline = true;
            }
            else
            {
                last_was_newline = false;
            }

            result += source[i];
        }

        return result;
    }

    token lexer::lex()
    {
        token tok{T_EOF, "\0"};

        next_token();
        // are we on EOF?
        consume_whitespace();
        // std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> converter;
        // std::string l = converter.to_bytes(c_char); // valid UTF-8 string
        // std::cout << "cur char >>> " << l << std::endl;
        std::string l(1, c_char); // valid UTF-8 string
        if (c_char == ';')
        {
            return new_token(T_ENDL, l, position);
        }
        else if (is_newline_char(c_char))
        {
            return new_token(T_ENDL, "\\n", position);
        }
        else if (c_char == '\0')
        {
            return tok;
        }
        else if (c_char == '(')
        {
            return new_token(T_LPAREN, l, position);
        }
        else if (c_char == ')')
        {
            return new_token(T_RPAREN, l, position);
        }
        else if (c_char == '{')
        {
            return new_token(T_LCURLY, l, position);
        }
        else if (c_char == '}')
        {
            return new_token(T_RCURLY, l, position);
        }
        else if (c_char == '[')
        {
            return new_token(T_LSQUARE, l, position);
        }
        else if (c_char == ']')
        {
            return new_token(T_RSQUARE, l, position);
        }
        else if (std::isalpha(c_char))
        {
            tok = make_identifier_token();
            if (is_reserved_word(tok.lexeme))
            {
                make_reserved_word_token(tok);
            }
            return tok;
        }
        else if (c_char == '=')
        {
            if (n_char == '=')
            {
                l += n_char;
                return new_token(T_EQUALITY, l, position);
            }
            return new_token(T_ASSIGN, l, position);
        }
        else if (c_char == '<')
        {
            if (n_char == '=')
            {
                l += n_char;
                next_token();
                return new_token(T_LESS_OR_EQUAL, l, position);
            }
            else if (n_char == '<')
            {
                l += n_char;
                next_token();
                if (c_char == '<')
                {
                    l += c_char;
                    next_token();

                    return new_token(T_LSHIFT_ASSIGN, l, position);
                }

                return new_token(T_LSHIFT, l, position);
            }
            return new_token(T_LESS_THAN, l, position);
        }
        else if (c_char == '>')
        {
            if (n_char == '=')
            {
                l += n_char;
                next_token();
                return new_token(T_GREATER_OR_EQUAL, l, position);
            }
            else if (n_char == '>')
            {
                l += n_char;
                next_token();
                if (c_char == '>')
                {
                    l += c_char;
                    next_token();

                    return new_token(T_RSHIFT_ASSIGN, l, position);
                }

                return new_token(T_RSHIFT, l, position);
            }
            return new_token(T_GREATER_THAN, l, position);
        }
        else if (c_char == '#' || (c_char == '/' && n_char == '/'))
        {
            // skip the comment...
            int start_pos = position;
            // next_token();
            while (c_char != '\0' && !is_newline_char(c_char))
            {
                next_token();
            }
            return lex();
        }
        else if (c_char == '&')
        {
            if (n_char == '&')
            {
                l += n_char;
                next_token();
                return new_token(T_AND, l, position);
            }
            return new_token(T_AMPERSAND, l, position);
        }
        else if (c_char == '\"' || c_char == '\'')
        {
            return make_string_literal_token();
        }
        else if (std::isdigit(c_char))
        {
            return make_numeric_literal_token();
        }
        else if (c_char == '+')
        {
            return new_token(T_PLUS, l, position);
        }
        else if (c_char == '-')
        {
            return new_token(T_MINUS, l, position);
        }
        else if (c_char == '*')
        {
            return new_token(T_STAR, l, position);
        }
        else if (c_char == '?')
        {
            return new_token(T_QUESTION, l, position);
        }
        else if (c_char == '|' && n_char == '|')
        {
            l += n_char;
            next_token();
            return new_token(T_LOGICAL_OR, l, position);
        }
        else if (c_char == '^')
        {
            return new_token(T_XOR, l, position);
        }
        else if (c_char == '/')
        {
            return new_token(T_FSLASH, l, position);
        }
        else if (c_char == ':')
        {
            return new_token(T_COLON, l, position);
        }
        else if (c_char == ',')
        {
            return new_token(T_COMMA, l, position);
        }
        else if (c_char == '!')
        {
            int start_pos = position;
            if (n_char == '=')
            {
                l += n_char;
                next_token();
                return new_token(T_NEQ, l, start_pos);
            }
            return new_token(T_NOT, l, start_pos);
        }
        else if (is_unicode(c_char))
        {
            return make_unicode_char_token(l);
        }
        else if (c_char == '|')
        {
            int start_pos = position;
            if (n_char == '|')
            {
                l += n_char;
                next_token();
                return new_token(T_OR, l, start_pos);
            }
            return new_token(T_PIPE, l, start_pos);
        }
        else if (c_char == '%')
        {
            return new_token(T_MODULO, l, position);
        }
        else if (c_char == '.')
        {
            return new_token(T_DOT, l, position);
        }

        return tok;
    }
    token lexer::new_token(const token_kind &kind, const std::string &lexeme, int start_pos)
    {
        token tok{kind, lexeme};
        tok.line = *line;                              // set it to current line int value on the lexer...
        tok.column = *column - (position - start_pos); // a token's column is it's start index / start index of it's lexeme in the source code...
        return tok;
    }
    token lexer::new_token(const token_kind &kind, const std::string &lexeme, int start_pos, int int_val)
    {
        token tok = new_token(kind, lexeme, start_pos);
        tok.int_val = std::make_unique<int>(std::move(int_val));
        return tok;
    }
    token lexer::new_token(const token_kind &kind, const std::string &lexeme, int start_pos, double float_val)
    {
        token tok = new_token(kind, lexeme, start_pos);
        tok.float_val = std::make_unique<double>(std::move(float_val));
        return tok;
    }
    void lexer::next_token()
    {
        position++;
        c_char = n_char;
        if (position + 1 < source.length())
        {
            n_char = source[position + 1];
        }
        else
        {
            n_char = '\0';
        }
        (*column)++;
        // this is no longer needed...if (position >= 0) //only start incrementing column once we've reach the start of the source.... not while we're still advancing away from EOF...inshort BOM
    }
    token lexer::make_string_literal_token()
    {
        char stop_char = c_char;
        next_token();
        int curr_pos = position;
        while (c_char != '\0' && c_char != stop_char)
        {
            next_token();
        }

        std::string l = source.substr(curr_pos, position - curr_pos);
        // next_token();
        return new_token(T_SLITERAL, l, curr_pos - 1);
    }
    token lexer::make_unicode_char_token(const std::string &l)
    {
        char32_t unicode = static_cast<char32_t>(static_cast<unsigned char>(c_char));
        switch (unicode)
        {
        case U'\u2227':
            return new_token(T_LOGICAL_AND, l, position);
        case U'\u2228':
            return new_token(T_LOGICAL_OR, l, position);

        case U'\u00AC':
            return new_token(T_XOR, l, position);
        default:
            std::ostringstream os;
            os << "[ERROR] Invalid token: " << l;
            return new_token(T_ERROR, os.str(), position);
        }
    }
    token lexer::make_identifier_token()
    {
        int start_pos = position;
        if (!std::isalpha(c_char) && c_char != '_')
        {
            return new_token(T_ERROR, std::string(1, c_char), start_pos);
        }
        next_token();
        while ((std::isalnum(c_char) || c_char == '_') && is_char_a_valid_ident_elem(n_char))
        {
            next_token();
        }
        std::string ident = source.substr(start_pos, position + 1 - start_pos);
        if (util::to_lowercase(ident) == "command")
        {
            token tok = lex(); // This should be T_LSHIFT_ASSIGN (<<<)
            if (tok.kind != T_LSHIFT_ASSIGN)
            {
                return new_token(T_ERROR, ident, start_pos);
            }
            size_t cmd_start = position + 1;
            size_t end_pos = source.find(">>>", cmd_start);
            if (end_pos == std::string::npos)
            {
                return new_token(T_ERROR, "Unterminated command block", start_pos);
            }
            std::string cmd_body = source.substr(cmd_start, end_pos - cmd_start);
            position = end_pos + 3; // we move past the closing >>> i.e T_RSHIFT_ASSIGN
            return new_token(T_COMMAND, cmd_body, start_pos);
        }
        return new_token(T_IDENT, ident, start_pos);
    }
    bool lexer::is_char_a_valid_ident_elem(char32_t n_char)
    {
        // this to verify the next lookahead char is permitted to be in an identifier...
        return std::isalnum(n_char) || n_char == '_';
    }

    // token lexer::make_numeric_literal_token()
    // {
    //     int start_pos = position;
    //     uint8_t dots{0};
    //     if (!std::isdigit(c_char))
    //     {
    //         return token(T_ERROR, std::string(1, c_char));
    //     }
    //     // next_token(); // advance from this current token...
    //     while (std::isdigit(c_char) || c_char == '.')
    //     {
    //         next_token();
    //         if (c_char == '.')
    //             dots++;
    //     }
    //     if (dots > 1)
    //     {
    //         throw std::runtime_error("invalid number format");
    //     }
    //     std::string cleaned = source.substr(start_pos, position - start_pos);
    //     bool is_float = dots > 0;
    //     if (is_float)
    //     {
    //         return new_token(T_NLITERAL, cleaned, start_pos, atof(cleaned.c_str()));
    //     }
    //     return new_token(T_NLITERAL, cleaned, start_pos, atoi(cleaned.c_str()));
    // }
    token lexer::make_numeric_literal_token()
    {
        int start_pos = position;
        uint8_t dots{0};
        while (c_char != '\0' && util::array_contains_char(n_char))
        {
            if (c_char == '.')
                dots++;
            next_token();
        }
        if (dots > 1)
            throw std::runtime_error("invalid number format");
        auto cleaned = source.substr(start_pos, position - start_pos + 1);
        bool is_float = dots > 0;
        if (is_float)
        {
            return new_token(T_NLITERAL, cleaned, start_pos, atof(cleaned.c_str()));
        }
        return new_token(T_NLITERAL, cleaned, start_pos, atoi(cleaned.c_str()));
    }

    void lexer::consume_whitespace()
    {
        for (;;)
        {
            if (is_newline_char(c_char))
            {
                (*line)++; // increment the line...
                (*column) = 1;
                break;
            }
            if (c_char != '\0' && std::isspace(c_char))
            {
                next_token();
            }
            else
            {
                break;
            }
        }
    }
    void lexer::skip_comments()
    {
        if (is_newline_char(c_char))
        {
            (*line)++; // increment the line...
            (*column) = 1;
            return;
        }
        if (c_char != '\0' && (c_char == '#' || (c_char = '/' && n_char == '/')))
        {
            // skip the comment...
            next_token();
            while (c_char != '\0' && !is_newline_char(c_char))
            {
                next_token();
            }
        }
    }
    // static std::string to_lowercase(std::string word)
    // {
    //     std::transform(word.begin(), word.end(), word.begin(),
    //                    [](unsigned char c)
    //                    { return std::tolower(c); });
    //     return word;
    // }
    void lexer::make_reserved_word_token(token &tok)
    {
        std::string l = util::to_lowercase(tok.lexeme);
        if (l == "and")
        {
            tok.kind = T_LOGICAL_AND;
        }
        else if (l == "call")
        {
            tok.kind = T_CALL;
        }
        else if (l == "import")
        {
            tok.kind = T_IMPORT;
        }
        else if (l == "as")
        {
            tok.kind = T_AS;
        }
        else if (l == "or")
        {
            tok.kind = T_LOGICAL_OR;
        }
        else if (l == "not")
        {
            tok.kind = T_LOGICAL_NOT;
        }
        else if (l == "true" || l == "false")
        {
            tok.kind = T_BLITERAL;
        }
        else if (l == "xor")
        {
            tok.kind = T_XOR;
        }
        else if (is_type_token(l))
        {
            tok.kind = T_TYPE;
        }
        //"if", "else", "while", "return"
        else if (l == "if")
        {
            tok.kind = T_IF;
        }
        else if (l == "else")
        {
            tok.kind = T_ELSE;
        }
        else if (l == "do")
        {
            tok.kind = T_DO;
        }
        else if (l == "while")
        {
            tok.kind = T_WHILE;
        }
        else if (l == "return")
        {
            tok.kind = T_RETURN;
        }
        //"input", "output", "runtime", "meta", "command", "then", "array", "file"
        else if (l == "input")
        {
            tok.kind = T_INPUT;
        }
        else if (l == "output")
        {
            tok.kind = T_OUTPUT;
        }
        else if (l == "runtime")
        {
            tok.kind = T_RUNTIME;
        }
        else if (l == "parameter_meta")
        {
            tok.kind = T_META;
        }
        else if (l == "command")
        {
            tok.kind = T_COMMAND;
        }
        else if (l == "then")
        {
            tok.kind = T_THEN;
        }
    }
    bool lexer::is_newline_char(unsigned char chr)
    {
        return chr == '\n';
    }
    bool lexer::is_reserved_word(const std::string &word)
    {
        const std::array<std::string, 33> reserved_words = {"and", "or", "xor", "not", "task", "struct", "int", "float", "string", "bool", "char", "class", "if", "else", "while", "return", "do", "input", "output", "runtime", "parameter_meta", "command", "then", "array", "file", "true", "false", "boolean", "workflow", "call", "import", "as", "map"};
        for (const auto &i : reserved_words)
        {
            if (i == util::to_lowercase(word))
                return true;
        }
        return false;
    }
    bool lexer::is_type_token(const std::string &word)
    {
        const std::array<std::string, 15> reserved_words = {
            "int",
            "float",
            "string",
            "bool",
            "char",
            "struct",
            "task",
            "class",
            "array",
            "file",
            "input",
            "output",
            "boolean",
            "workflow",
            "map", //this is WDL's hashMap...
        };
        for (const auto &i : reserved_words)
        {
            if (i == util::to_lowercase(word))
                return true;
        }
        return false;
    }
    bool lexer::is_unicode(const char &chr)
    {
        char t = chr;
        char32_t unicode = static_cast<char32_t>(static_cast<unsigned char>(chr));

        const std::array<char32_t, 3> unicode_chars = {U'\u2227', U'\u2228', U'\u00AC'};
        for (const auto &i : unicode_chars)
        {
            if (i == unicode)
                return true;
        }
        return false;
    }

}