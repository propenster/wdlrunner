#include <memory>
#include <vector>
#include <string>
#include "token.h"
#include <variant>
#include "lexer.h"

#include <optional>

#ifndef PARSER_H
#define PARSER_H

namespace soto
{

    enum ast_node_type
    {
        N_PROGRAM,
        N_FUNC_DECL,
        N_CLASS_DECL,
        N_TASK_DECL,
        N_INPUT_DECL,
        N_OUTPUT_DECL,
        N_RUNTIME_DECL,
        N_META_DECL,
        N_META_MEMBER_DECL,
        N_COMMAND_DECL,
        N_VERSION_DECL,

        N_FUNC_CALL,
        N_ARRAY,

        // others
        N_VAR_DECL,
        N_TYPE,
        N_TYPE_NULLABLE,
        N_IDENT,
        N_BLOCK,
        N_IF_STMT,
        N_DO_WHILE_STMT,
        N_WHILE_STMT,
        N_RET_STMT,
        N_EXPR_STMT,
        N_BINARY_EXPR,
        N_UNARY,
        N_PRIMARY,
        N_ASSIGNMENT,
        N_LITERAL,
        N_STRUCT,

        N_WTCALL,        // Node Workflow Task->Call...
        N_MEMBER_ACCESS, // Node for member access (e.g., obj.member)
        // N_OBJECT,     // Node for object (e.g., obj)
        N_IMPORT_DECL,
    };

    inline const char *ast_node_type_to_string(ast_node_type type)
    {
        switch (type)
        {
        case N_PROGRAM:
            return "N_PROGRAM";
        case N_FUNC_DECL:
            return "N_FUNC_DECL";
        case N_CLASS_DECL:
            return "N_CLASS_DECL";
        case N_TASK_DECL:
            return "N_TASK_DECL";
        case N_INPUT_DECL:
            return "N_INPUT_DECL";
        case N_OUTPUT_DECL:
            return "N_OUTPUT_DECL";
        case N_RUNTIME_DECL:
            return "N_RUNTIME_DECL";
        case N_META_DECL:
            return "N_META_DECL";
        case N_COMMAND_DECL:
            return "N_COMMAND_DECL";

        case N_VAR_DECL:
            return "N_VAR_DECL";

        case N_TYPE:
            return "N_TYPE";
        case N_TYPE_NULLABLE:
            return "N_TYPE_NULLABLE";
        case N_IDENT:
            return "N_IDENT";
        case N_BLOCK:
            return "N_BLOCK";
        case N_IF_STMT:
            return "N_IF_STMT";
        case N_DO_WHILE_STMT:
            return "N_DO_WHILE_STMT";
        case N_WHILE_STMT:
            return "N_WHILE_STMT";
        case N_RET_STMT:
            return "N_RET_STMT";
        case N_EXPR_STMT:
            return "N_EXPR_STMT";
        case N_BINARY_EXPR:
            return "N_BINARY_EXPR";
        case N_UNARY:
            return "N_UNARY";
        case N_PRIMARY:
            return "N_PRIMARY";
        case N_ASSIGNMENT:
            return "N_ASSIGNMENT";
        case N_LITERAL:
            return "N_LITERAL"; // for all kinds of literals...Number, String, Boolean, etc...
        case N_STRUCT:
            return "N_STRUCT";
        case N_VERSION_DECL:
            return "N_VERSION_DECL";
        case N_FUNC_CALL:
            return "N_FUNC_CALL";
        case N_ARRAY:
            return "N_ARRAY";
        case N_META_MEMBER_DECL:
            return "N_META_MEMBER_DECL";

        case N_WTCALL:
            return "N_WTCALL";
        case N_MEMBER_ACCESS:
            return "N_MEMBER_ACCESS";
        case N_IMPORT_DECL:
            return "N_IMPORT_DECL";

        default:
            return "UNKNOWN AST_NODE_TYPE";
        }
    }

    struct ast_node;
    using ast_node_ptr = std::unique_ptr<ast_node>;

    // All the different AST_NODE variants here...
    // program node...
    struct program
    {
        ast_node_ptr version;
        std::vector<ast_node_ptr> imports;
        std::vector<ast_node_ptr> declarations;
    };

    struct input_decl
    {
        ast_node_ptr body;
        std::vector<ast_node_ptr> members;
    };
    struct command_decl
    {
        ast_node_ptr body;                   // body text...a stringLiteral...of the command String...
        std::vector<ast_node_ptr> arguments; // arguments to the command ~{nameOfVariable}...
    };
    struct output_decl
    {
        ast_node_ptr body;
        std::vector<ast_node_ptr> members;
    };
    struct func_call
    {
        ast_node_ptr identifier;
        std::vector<ast_node_ptr> arguments;
    };
    struct array_expr
    {
        ast_node_ptr identifier;
        std::vector<ast_node_ptr> elements;
    };
    struct runtime_decl
    {
        std::vector<std::tuple<ast_node_ptr, ast_node_ptr>> members; // runtime members (identifier, value (expr))
    };
    struct meta_decl
    {
        ast_node_ptr identifier;
        std::vector<std::tuple<ast_node_ptr, ast_node_ptr>> members;
    };

    struct call_decl
    {
        ast_node_ptr identifier;
        std::vector<std::tuple<ast_node_ptr, ast_node_ptr>> arguments;
    };
    struct member_access
    {
        ast_node_ptr object;
        ast_node_ptr member;
    };

    struct import_decl
    {
        ast_node_ptr path;
        ast_node_ptr alias;
    };
    struct func_decl
    {
        ast_node_ptr type;
        ast_node_ptr identifier;
        std::vector<ast_node_ptr> parameters;
        ast_node_ptr body;
    };
    struct class_decl
    {
        ast_node_ptr identifier;
        std::vector<ast_node_ptr> members; // instance fields (var_decl) or methods (func_decl)
    };
    struct var_decl
    {
        ast_node_ptr type;
        ast_node_ptr identifier;
        ast_node_ptr initializer;
    };
    struct block
    {
        std::vector<ast_node_ptr> statements;
    };
    struct if_stmt
    {
        ast_node_ptr condition;
        ast_node_ptr then_;
        ast_node_ptr else_if;
        ast_node_ptr else_;
    };
    struct while_stmt
    {
        ast_node_ptr condition;
        ast_node_ptr body;
    };
    struct do_while_stmt
    {
        ast_node_ptr body;
        ast_node_ptr condition;
    };

    struct ret_stmt
    {
        ast_node_ptr expr;
    };

    struct expr_stmt
    {
        ast_node_ptr expr;
    };

    struct binary_expr
    {
        ast_node_ptr left;
        std::shared_ptr<token> op; // is this necessary?
        ast_node_ptr right;
    };

    struct unary_expr
    {
        ast_node_ptr operand;
    };
    struct literal_expr
    {
        token value;
    };
    struct version_decl
    {
        ast_node_ptr version;
        std::shared_ptr<token> version_number;
    };
    struct assign_expr
    {
        ast_node_ptr left;
        ast_node_ptr right;
    };

    struct ast_node
    {
        bool is_nullable;
        ast_node_type type;
        std::shared_ptr<token> tok;
        std::variant<program,
                     func_decl,
                     class_decl,
                     input_decl,
                     output_decl,
                     runtime_decl,
                     meta_decl,
                     version_decl,

                     var_decl,
                     block,
                     if_stmt,
                     while_stmt,
                     do_while_stmt,
                     ret_stmt,
                     expr_stmt,
                     binary_expr,
                     unary_expr,
                     literal_expr,
                     assign_expr,
                     func_call,
                     array_expr,
                     command_decl,
                     call_decl,
                     member_access,
                     import_decl

                     >
            node;
    };

    struct parser
    {
    public:
        std::unique_ptr<lexer> m_lexer;
        std::shared_ptr<token> curr_tok;
        std::shared_ptr<token> prev_tok;
        std::optional<token> next_tok;
        bool error_state;

        // constructor
        parser(std::unique_ptr<lexer>);

        ast_node_ptr parse_program();
        void print_ast_node(const ast_node_ptr &, int indent);

        // helper methods...
    private:
        void read_token_or_emit_error();
        token next_token();
        bool peek_token(const token_kind &);
        void emit_error(const std::string &, const token &);
        void expect_token_or_emit_error(token_kind, const std::string &);
        bool expect_token(const token_kind &);
        bool expect_token_and_read(const token_kind &);

        ast_node_ptr new_node(const ast_node_type &);

        ast_node_ptr parse_decl();
        ast_node_ptr parse_func_decl();
        ast_node_ptr parse_class_decl();
        ast_node_ptr parse_struct_decl();
        ast_node_ptr parse_var_decl();
        ast_node_ptr parse_block();
        ast_node_ptr parse_stmt();
        ast_node_ptr parse_if_stmt();
        ast_node_ptr parse_ternary_if_stmt();
        ast_node_ptr parse_do_while_stmt();
        ast_node_ptr parse_while_stmt();
        ast_node_ptr parse_return_stmt();
        ast_node_ptr parse_expr_stmt();
        ast_node_ptr parse_expr();
        ast_node_ptr parse_assign_expr();

        //....
        ast_node_ptr parse_equality_expr();
        ast_node_ptr parse_comparison_expr();
        ast_node_ptr parse_unary_expr();
        ast_node_ptr parse_logor_expr();
        ast_node_ptr parse_logand_expr();
        ast_node_ptr parse_term_expr();
        ast_node_ptr parse_factor_expr();

        ast_node_ptr parse_primary_expr();
    };

}

#endif