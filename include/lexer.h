#ifndef LEXER_H
#define LEXER_H
#include <string>
#include <iostream>
#include "token.h"

namespace soto
{

    struct lexer
    {

    public:
        int position;
        unsigned char c_char;
        unsigned char n_char;
        std::string source;
        std::shared_ptr<int> line; // this is for line and col tracking... if you ask me, just for nice error reporting or whatever or maybe it may help linters for our language in the future...
        std::shared_ptr<int> column;

        lexer(std::string);
        lexer() = default;
        ~lexer() = default;

        // important instance methods....
        token lex();
        void next_token();
        void consume_whitespace();
        void skip_comments();
        token make_identifier_token();
        void make_reserved_word_token(token &);
        token make_string_literal_token();
        token make_numeric_literal_token();
        token make_unicode_char_token(const std::string &);
        token new_token(const token_kind &, const std::string &, int);
        token new_token(const token_kind &, const std::string &, int, int); // new_token with int_val for numeric token i suppose
        token new_token(const token_kind &, const std::string &, int, double);
        // std::string to_lowercase(std::string word);
        std::unique_ptr<lexer> clone() const; 

    private:
        static bool is_newline_char(unsigned char);
        bool is_reserved_word(const std::string &);
        bool is_unicode(const char &);
        bool is_char_a_valid_ident_elem(char32_t);
        bool is_type_token(const std::string &);
        std::string canonicalize_source_str(const std::string &);
    };

}

#endif