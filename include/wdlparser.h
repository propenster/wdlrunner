
#ifndef WDL_PARSER_H
#define WDL_PARSER_H
#include "parser.h"

namespace soto
{
    enum wdl_ast_node_type
    {
        N_WDL_PROGRAM,
        N_WDL_FUNC_DECL,
        N_WDL_TASK_DECL,
        N_WDL_FUNC_CALL,
        N_WDL_VAR_DECL,
        N_WDL_TYPE,
        N_WDL_IDENT,
        N_WDL_BLOCK,
        N_WDL_IF_STMT,
        N_WDL_INPUT,
        N_WDL_OUTPUT,
        N_WDL_RUNTIME,
        N_WDL_META,
        N_WDL_COMMAND,

        N_WDL_DO_WHILE_STMT,
        N_WDL_WHILE_STMT,
        N_WDL_RET_STMT,
        N_WDL_EXPR_STMT,
        N_WDL_BINARY_EXPR,
        N_WDL_UNARY,
        N_WDL_PRIMARY,
        N_WDL_ASSIGNMENT,
        N_WDL_LITERAL,
    };

    using wdl_ast_node_ptr = std::unique_ptr<ast_node>;

    // struct program
    // {
    //     std::vector<wdl_ast_node_ptr> declarations;
    // };

    // struct wdl_ast_node : public ast_node
    // {
    //     wdl_ast_node_type type;
    //     std::shared_ptr<token> tok;
    //     std::variant<program,
    //                  func_decl,
    //                  class_decl,
    //                  var_decl,
    //                  block,
    //                  if_stmt,
    //                  while_stmt,
    //                  do_while_stmt,
    //                  ret_stmt,
    //                  expr_stmt,
    //                  binary_expr,
    //                  unary_expr,
    //                  literal_expr,
    //                  assign_expr>
    //         node;
    // };

    // WDLParser is a parser for the WDL (Workflow Description Language).
    // It extends the base parser class and provides specific parsing for WDL alone...
    struct wdlparser : public parser
    {
        using parser::parser;

    public:
        wdlparser(std::unique_ptr<lexer> lex);
        virtual ~wdlparser() = default;
        ast_node_ptr parse_program();
        ast_node_ptr parse_decl();
        ast_node_ptr parse_class_decl();

        using parser::parse_class_decl;
        using parser::parse_decl;
        using parser::parse_program;
    };

}

#endif // WDL_PARSER_H