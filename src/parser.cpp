
#include "parser.h"
#include <sstream>
#include <string>
#include "string_utils.h"
#include <regex>

namespace soto
{

    parser::parser(std::unique_ptr<lexer> lex) : m_lexer(std::move(lex)), curr_tok(std::make_shared<token>()), prev_tok(std::make_shared<token>()), error_state(false)
    {

        read_token_or_emit_error();
    }

    // this EATS token... and also emits ERROR if we hit a T_ERROR anywhere...
    void parser::read_token_or_emit_error()
    {
        prev_tok = std::move(curr_tok);
        for (;;)
        {
            token n_tok = next_token();
            // std::cout << "next token emitted in parser is " << n_tok << std::endl;
            curr_tok = std::make_shared<token>(std::move(n_tok));
            if (curr_tok->kind != T_ERROR)
                break;
            emit_error(curr_tok->lexeme, *curr_tok); // this is bada bad
        }
    }
    token parser::next_token()
    {
        return m_lexer->lex();
    }
    bool parser::peek_token(const token_kind &kind)
    {
        std::unique_ptr<lexer> copy = std::make_unique<lexer>(*m_lexer); // deep copy
        token t = copy->lex();
        return t.kind == kind;
    }

    // THIS EATS tokens....consumes them doesn't return anything...
    // but emits diagnostic errors when necessary...
    void parser::expect_token_or_emit_error(token_kind kind, const std::string &error)
    {
        if (curr_tok->kind != kind)
        {
            emit_error(error, *curr_tok);
            return;
        }
        read_token_or_emit_error();
    }
    // this CHECKS token kind is equal to current token kind on the parser table...
    bool parser::expect_token(const token_kind &kind)
    {
        return curr_tok->kind == kind;
    }
    // This MATCHES token kind with current token on the parser table and returns false if not equal but reads it and advances and returns TRUE if EQUAL
    bool parser::expect_token_and_read(const token_kind &kind)
    {
        if (!expect_token(kind))
            return false;
        read_token_or_emit_error();
        return true;
    }

    void parser::emit_error(const std::string &error_msg, const token &tok)
    {
        error_state = true;
        std::ostringstream os;
        os << "[ERROR] [line " << tok.line << "]";

        if (tok.kind == T_EOF)
        {
            os << " at end";
        }
        else if (tok.kind != T_ERROR)
        {
            os << " at '" << tok.lexeme << "'";
        }

        os << ": " << error_msg << std::endl;
        std::cerr << os.str();
        return;
    }
    ast_node_ptr parser::parse_program()
    {
        ast_node_ptr prog = new_node(N_PROGRAM);
        program prow; // your variant type holding declarations

        while (!expect_token_and_read(T_EOF))
        {
            // first parse version if it's present...
            // it's probably a version tag...
            if (expect_token_and_read(T_IDENT) && util::to_lowercase(prev_tok->lexeme) == "version")
            {
                ast_node_ptr version = new_node(N_VERSION_DECL);
                version->tok = prev_tok; // just capture the token in front of it straigh...
                version_decl version_decl{};

                if (expect_token_and_read(T_NLITERAL))
                {
                    version_decl.version = new_node(N_LITERAL);
                    version_decl.version_number = prev_tok;
                    version_decl.version->tok = prev_tok;
                }
                else
                {
                    emit_error("Expect a version number after 'version' keyword.", *prev_tok);
                }
                version->node = std::move(version_decl);
                prow.version = std::move(version);
                if (expect_token(T_ENDL))
                    expect_token_or_emit_error(T_ENDL, "Expect ';' or newline after version declaration.");
            }

            ast_node_ptr decl = parse_decl();
            if (decl)
            {
                prow.declarations.push_back(std::move(decl));
                if (expect_token(T_ENDL))
                    expect_token_or_emit_error(T_ENDL, "Expect ';' or newline after declaration.");
            }
        }

        prog->node = std::move(prow); // set the variant in the ast_node
        return prog;                  // <-- you forgot to return the node
    }

    ast_node_ptr parser::parse_decl()
    {
        ast_node_ptr result = nullptr;
        if (expect_token_and_read(T_TYPE))
        {
            // check that the next token is an Identifier like int nameOfVar
            if (expect_token(T_IDENT) && peek_token(T_LPAREN)) // there's gotta be abetter way to do this... that '(' is ugly in this place..man
            {
                result = parse_func_decl();
                if (expect_token(T_ENDL))
                    expect_token_or_emit_error(T_ENDL, "Expect ';' or newline after declaration.");
                return result;
            }
            else if (expect_token(T_IDENT) && peek_token(T_LCURLY)) // this probably a Struct, Task, or Class
            {
                result = parse_class_decl();
                if (expect_token(T_ENDL))
                    expect_token_or_emit_error(T_ENDL, "Expect ';' or newline after declaration.");
                return result;
            }
            else
            {
                // T_COMMAND should come here...
                result = parse_var_decl();
                if (expect_token(T_ENDL))
                    expect_token_or_emit_error(T_ENDL, "Expect ';' or newline after declaration.");
                return result;
            }
        }

        result = parse_stmt();
        if (expect_token(T_ENDL))
            expect_token_or_emit_error(T_ENDL, "Expect ';' or newline after declaration.");
        return result;
    }
    ast_node_ptr parser::parse_func_decl()
    {
        ast_node_ptr func = new_node(N_FUNC_DECL);
        func_decl decl{};
        decl.type = new_node(N_TYPE);
        decl.type->tok = prev_tok;

        expect_token_or_emit_error(T_IDENT, "Expect function name.");
        decl.identifier = new_node(N_IDENT);
        decl.identifier->tok = prev_tok;

        expect_token_or_emit_error(T_LPAREN, "Expect '(' after function name.");
        decl.parameters;
        if (!expect_token(T_RPAREN))
        {
            do
            {
                if (decl.parameters.size() >= 255)
                {
                    emit_error("Can't have more than 255 params.", *curr_tok);
                }
                ast_node_ptr param = new_node(N_VAR_DECL);
                var_decl var_decl{};
                if (expect_token_and_read(T_TYPE))
                {
                    var_decl.type = new_node(N_TYPE);
                    var_decl.type->tok = prev_tok;
                }
                else
                {
                    emit_error("Expect parameter type.", *prev_tok);
                }

                expect_token_or_emit_error(T_IDENT, "Expect parameter name.");
                var_decl.identifier = new_node(N_IDENT);
                var_decl.identifier->tok = prev_tok;
                param->node = std::move(var_decl);

                decl.parameters.push_back(std::move(param));

            } while (expect_token_and_read(T_COMMA));
        }

        expect_token_or_emit_error(T_RPAREN, "Expect closing ')' after parameters.");
        expect_token_or_emit_error(T_LCURLY, "Expect closing '{' before function body.");
        decl.body = parse_block();

        func->node = std::move(decl);

        return func;
    }

    static bool is_unusual_type(const token_kind &kind)
    {
        return kind == T_COMMAND || kind == T_RUNTIME || kind == T_META || T_CALL;
    }
    // parse a class/task/struct declaration...
    //  class have name...
    //  expect '{' to begin class body
    //  expect '}' to close class body
    //  class have members... which can be fields or methods
    ast_node_ptr parser::parse_class_decl()
    {
        ast_node_ptr klass = new_node(N_CLASS_DECL);
        class_decl decl{};

        // class have name...
        expect_token_or_emit_error(T_IDENT, "Expect class name.");
        decl.identifier = new_node(N_IDENT);
        decl.identifier->tok = prev_tok;

        // expect '{' to begin class body
        expect_token_or_emit_error(T_LCURLY, "Expect '{' to begin class body.");

        while (!expect_token(T_RCURLY) && !expect_token(T_EOF))
        {
            // consume any endline T_ENDL before start of class members...
            if (expect_token(T_ENDL))
                expect_token_and_read(T_ENDL);

            // expect type keyword at start of class member
            if ((!expect_token(T_TYPE) && !is_unusual_type(curr_tok->kind)) && !expect_token(T_ENDL))
            {
                emit_error("Expected type keyword at start of class member.", *curr_tok);
                break;
            }

            // if it's a CALL construct, then it's a class (most-likely a WORKFLOW class) member...
            if (expect_token_and_read(T_CALL))
            {
                ast_node_ptr call = new_node(N_WTCALL);

                expect_token_or_emit_error(T_IDENT, "Expect identifier for call construct.");
                ast_node_ptr task_name_to_call = new_node(N_IDENT);
                task_name_to_call->tok = prev_tok;
                call_decl input_decl{};
                if (!expect_token(T_LCURLY))
                {
                    // then this is an input less call construct...
                    call->node = std::move(input_decl); // just push the no-input call...
                    decl.members.push_back(std::move(call));
                    continue;
                }
                expect_token_or_emit_error(T_LCURLY, "Expect '{' to begin call construct input body.");
                if (expect_token(T_ENDL))
                    expect_token_and_read(T_ENDL);
                if (!expect_token(T_TYPE))
                {
                    // input keyword is a type... no need to check for T_IDENT
                    // this is an INPUT-less call construct...
                    // so this unserious user is trying to be funny...
                    // a left CURLY without an intention of having inputs...
                    call->node = std::move(input_decl); // just push the no-input call...
                    decl.members.push_back(std::move(call));
                    continue;
                }
                expect_token_or_emit_error(T_TYPE, "Expect identifier for call construct."); // input: some may not have inputs...handle that...input is a tTYPE
                expect_token_or_emit_error(T_COLON, "Expect ':' after call construct input identifier.");

                input_decl.identifier = std::move(task_name_to_call);

                do
                {
                    // if (expect_token_and_read(T_ENDL))
                    //     continue;
                    if (expect_token(T_ENDL))
                        expect_token_and_read(T_ENDL);
                    expect_token_or_emit_error(T_IDENT, "Expect identifier for call construct input.");
                    ast_node_ptr input_ident = new_node(N_IDENT);
                    input_ident->tok = prev_tok;

                    expect_token_or_emit_error(T_ASSIGN, "Expect ':' after call construct input identifier.");

                    // expect_token_or_emit_error(T_IDENT, "Expect identifier for call construct input.");
                    ast_node_ptr input_value = parse_expr();
                    // expect_token_or_emit_error(T_ENDL, "Expect ';' or newline after call construct input value.");
                    input_decl.arguments.emplace_back(std::move(input_ident), std::move(input_value));
                } while (expect_token_and_read(T_COMMA));
                if (expect_token(T_ENDL))
                    expect_token_and_read(T_ENDL);
                expect_token_or_emit_error(T_RCURLY, "Expect '}' to close call construct body.");
                call->node = std::move(input_decl);
                decl.members.push_back(std::move(call));
                continue;
            }

            // if it's parameter_meta T_META,
            if (expect_token_and_read(T_META))
            {
                meta_decl meta{};

                meta.identifier = new_node(N_IDENT);
                meta.identifier->tok = prev_tok;

                expect_token_or_emit_error(T_LCURLY, "Expect '{' to begin parameter_meta body.");
                while (!expect_token(T_RCURLY) && !expect_token(T_EOF))
                {
                    if (expect_token_and_read(T_ENDL))
                        continue;

                    expect_token_or_emit_error(T_IDENT, "Expect identifier for parameter_meta member.");
                    ast_node_ptr member_identifier = new_node(N_IDENT);
                    member_identifier->tok = prev_tok;

                    expect_token_or_emit_error(T_COLON, "Expect ':' after parameter_meta member identifier.");

                    expect_token_or_emit_error(T_LCURLY, "Expect '{' to begin parameter_meta member body.");
                    meta_decl member_meta{};
                    while (!expect_token(T_RCURLY) && !expect_token(T_EOF))
                    {
                        if (expect_token_and_read(T_ENDL))
                            continue;

                        expect_token_or_emit_error(T_IDENT, "Expect identifier for parameter_meta member property.");
                        ast_node_ptr property_identifier = new_node(N_IDENT);
                        property_identifier->tok = prev_tok;

                        expect_token_or_emit_error(T_COLON, "Expect ':' after parameter_meta member property identifier.");

                        ast_node_ptr property_value = parse_expr();

                        member_meta.members.emplace_back(std::move(property_identifier), std::move(property_value));

                        if (expect_token(T_ENDL))
                            read_token_or_emit_error();
                    }
                    expect_token_or_emit_error(T_RCURLY, "Expect '}' to close parameter_meta member body.");

                    ast_node_ptr member_meta_node = new_node(N_META_MEMBER_DECL);
                    member_meta_node->node = std::move(member_meta);
                    meta.members.emplace_back(std::move(member_identifier), std::move(member_meta_node));
                }
                expect_token_or_emit_error(T_RCURLY, "Expect '}' to close parameter_meta body.");

                ast_node_ptr meta_node = new_node(N_META_DECL);
                meta_node->node = std::move(meta);
                decl.members.push_back(std::move(meta_node));
                continue;
            }

            if (expect_token(T_COMMAND))
            {
                expect_token_or_emit_error(T_COMMAND, "Expected command keyword before command");
                // this is a command
                command_decl command{};
                ast_node_ptr cmd_body = new_node(N_LITERAL);
                cmd_body->tok = prev_tok;
                cmd_body->tok->kind = T_SLITERAL; // this is a string literal
                std::string_view command_text = cmd_body->tok->lexeme;
                command.body = std::move(cmd_body); // cmd_body is a N_LITERAL a StringLiteral to be specific...

                // parse command arguments
                std::vector<std::string_view> args;

                // Find all occurrences of ~{nameOfVariable} using a regex
                std::regex variable_pattern(R"(\~\{([a-zA-Z_][a-zA-Z0-9_]*)\})");
                std::smatch match;
                std::string command_str(command_text);

                while (std::regex_search(command_str, match, variable_pattern))
                {
                    if (match.size() > 1)
                    {
                        // we just use the owned lexer to create a new T_IDENT token on the fly for this command argument
                        ast_node_ptr arg = new_node(N_IDENT);
                        token tk = m_lexer->new_token(T_IDENT, match[1].str(), prev_tok->column);
                        arg->tok = std::make_shared<token>(std::move(tk));
                        command.arguments.push_back(std::move(arg)); // Extract the variable name
                        // args.push_back(match[1].str());   // Extract the variable name
                    }
                    command_str = match.suffix().str(); // Move to the next match
                }
                ast_node_ptr command_node = new_node(N_COMMAND_DECL);
                command_node->node = std::move(command);
                decl.members.push_back(std::move(command_node));
                continue;
            }

            // parse type
            ast_node_ptr type = new_node(N_TYPE);
            read_token_or_emit_error(); // consume type
            type->tok = prev_tok;
            // if the very next token to this type is a '?', then this is a nullable type...
            // so we modify the type of this type-AST_node to N_TYPE_NULLABLE
            if (!util::is_non_primitive_member_type(type->tok->lexeme) && expect_token_and_read(T_QUESTION))
            {
                // type->tok->lexeme += "?";
                type->type = N_TYPE_NULLABLE;
            }

            // if it's a known/inbuilt in summary a non-primitive type like input, output, runtime, meta, etc...
            std::string lexeme = type->tok->lexeme;
            if (util::is_non_primitive_member_type(type->tok->lexeme) || is_unusual_type(type->tok->kind))
            {
                std::stringstream ss;
                // if (expect_token(T_ENDL))
                //     expect_token_or_emit_error(T_ENDL, "Expect ';' or newline after block declaration.");
                ss << "Expect '{' to begin" << type->tok->lexeme << " body.";
                if (type->tok->kind != T_COMMAND) // only command doesn't need a '{' to start...
                    expect_token_or_emit_error(T_LCURLY, ss.str());
                const std::string &type_name = util::to_lowercase(type->tok->lexeme);
                if (type_name == "input")
                {
                    input_decl input{};
                    input.body = parse_block();

                    ast_node_ptr input_node = new_node(N_INPUT_DECL);
                    input_node->node = std::move(input);
                    decl.members.push_back(std::move(input_node));
                }
                else if (type_name == "output")
                {
                    output_decl output{};
                    output.body = parse_block();

                    ast_node_ptr output_node = new_node(N_OUTPUT_DECL);
                    output_node->node = std::move(output);
                    decl.members.push_back(std::move(output_node));
                }
                else if ("runtime")
                {
                    if (expect_token(T_ENDL))
                        expect_token_or_emit_error(T_ENDL, "Expect ';' or newline after block declaration.");
                    // this is a runtime...
                    runtime_decl runtime{};
                    // expect_token_or_emit_error(T_LCURLY, "Expect '{' to begin runtime body.");

                    while (!expect_token(T_RCURLY) && !expect_token(T_EOF))
                    {
                        expect_token_and_read(T_ENDL); // consume any endline T_ENDL before start of runtime members...

                        if (!expect_token(T_IDENT))
                        {
                            emit_error("Expected identifier for runtime member.", *curr_tok);
                            break;
                        }

                        ast_node_ptr identifier = new_node(N_IDENT);
                        read_token_or_emit_error(); // consume identifier
                        identifier->tok = prev_tok;

                        expect_token_or_emit_error(T_COLON, "Expect ':' after runtime member identifier.");

                        ast_node_ptr value = parse_expr(); // parse the value (expression) for the runtime member

                        runtime.members.emplace_back(std::move(identifier), std::move(value));

                        if (expect_token(T_ENDL)) // consume newline after each runtime member
                        {
                            read_token_or_emit_error();
                        }
                    }

                    expect_token_or_emit_error(T_RCURLY, "Expect '}' to close runtime body.");

                    ast_node_ptr runtime_node = new_node(N_RUNTIME_DECL);
                    runtime_node->node = std::move(runtime);
                    decl.members.push_back(std::move(runtime_node));
                }
                else if (type_name == "output")
                {
                    output_decl output{};
                    output.body = parse_block();

                    ast_node_ptr output_node = new_node(N_OUTPUT_DECL);
                    output_node->node = std::move(output);
                    decl.members.push_back(std::move(output_node));
                }
                // else if (type_name == "runtime")
                // {
                //     runtime_decl runtime{};
                //     runtime.members = decl.members;
                //     decl.members.clear();

                //     ast_node_ptr runtime_node = new_node(N_RUNTIME_DECL);
                //     runtime_node->node = std::move(runtime);
                //     decl.members.push_back(std::move(runtime_node));
                // }
                // else if (type_name == "meta")
                // {
                //     meta_decl meta{};
                //     meta.members = decl.members;
                //     decl.members.clear();

                //     ast_node_ptr meta_node = new_node(N_META_DECL);
                //     meta_node->node = std::move(meta);
                //     decl.members.push_back(std::move(meta_node));
                // }
            }
            else
            {
                expect_token_or_emit_error(T_IDENT, "Expect member name after type.");
                ast_node_ptr ident = new_node(N_IDENT);
                ident->tok = prev_tok;

                if (expect_token_and_read(T_LPAREN))
                {
                    // Parse method
                    func_decl func{};
                    func.type = std::move(type);
                    func.identifier = std::move(ident);

                    if (!expect_token(T_RPAREN))
                    {
                        do
                        {
                            if (func.parameters.size() >= 255)
                            {
                                emit_error("Too many parameters in method.", *curr_tok);
                            }

                            ast_node_ptr param = new_node(N_VAR_DECL);
                            var_decl var_decl{};

                            if (expect_token_and_read(T_TYPE))
                            {
                                var_decl.type = new_node(N_TYPE);
                                var_decl.type->tok = prev_tok;
                            }
                            else
                            {
                                emit_error("Expect parameter type.", *curr_tok);
                            }

                            expect_token_or_emit_error(T_IDENT, "Expect parameter name.");
                            var_decl.identifier = new_node(N_IDENT);
                            var_decl.identifier->tok = prev_tok;
                            param->node = std::move(var_decl);

                            func.parameters.push_back(std::move(param));

                        } while (expect_token_and_read(T_COMMA));
                    }

                    expect_token_or_emit_error(T_RPAREN, "Expect ')' after parameters.");
                    expect_token_or_emit_error(T_LCURLY, "Expect '{' before method body.");

                    func.body = parse_block();

                    ast_node_ptr method = new_node(N_FUNC_DECL);
                    method->node = std::move(func);

                    decl.members.push_back(std::move(method));
                }
                else
                {
                    // Parse field
                    ast_node_ptr field = new_node(N_VAR_DECL);
                    var_decl var{};
                    var.type = std::move(type);
                    var.identifier = std::move(ident);

                    if (expect_token(T_QUESTION)) // if its File? or Int? or String? whatever....it's a nullable table and we'll get a '?' before the type Identifier
                    {
                        var.type->type = N_TYPE_NULLABLE;
                        read_token_or_emit_error(); // consume the '?'
                    }

                    if (expect_token_and_read(T_ASSIGN))
                    {
                        var.initializer = parse_expr();
                    }
                    else
                    {
                        var.initializer = nullptr;
                    }
                    if (expect_token(T_ENDL)) // let;s just do this only when there's newline...
                    {
                        expect_token_or_emit_error(T_ENDL, "Expect ';' or newline after field.");
                    }
                    field->node = std::move(var);
                    decl.members.push_back(std::move(field));
                }
            }
        }

        expect_token_or_emit_error(T_RCURLY, "Expect '}' to close class body.");
        klass->node = std::move(decl);
        return klass;
    }

    // parse a struct declaration.../Task or Class declaration....
    ast_node_ptr parser::parse_struct_decl()
    {
        return nullptr;
    }
    ast_node_ptr parser::new_node(const ast_node_type &type)
    {
        ast_node_ptr node = std::make_unique<ast_node>();
        node->type = type;
        return node;
    }
    ast_node_ptr parser::parse_var_decl()
    {
        // handle command, we're trying to parse command decl as a variable declaration...

        ast_node_ptr node = new_node(N_VAR_DECL);
        var_decl var_node{};
        var_node.type = new_node(N_TYPE);
        // read_token_or_emit_error(); // consume type
        var_node.type->tok = prev_tok;

        if (expect_token(T_QUESTION)) // if its File? or Int? or String? whatever....it's a nullable table and we'll get a '?' before the type Identifier
        {
            var_node.type->type = N_TYPE_NULLABLE;
            read_token_or_emit_error(); // consume the '?'
        }
        expect_token_or_emit_error(T_IDENT, "Expect variable name.");
        var_node.identifier = new_node(N_IDENT);

        var_node.identifier->tok = prev_tok;

        if (expect_token_and_read(T_ASSIGN))
        {
            var_node.initializer = parse_expr();
        }
        else
        {
            var_node.initializer = nullptr;
        }

        // expect_token_or_emit_error(T_ENDL, "Expect ';' or newline after variable declaration.");

        node->node = std::move(var_node);

        return node;
    }
    ast_node_ptr parser::parse_block()
    {
        auto node = new_node(N_BLOCK);
        block block{};
        while (!expect_token(T_RCURLY) && !expect_token(T_EOF))
        {
            expect_token_and_read(T_ENDL); // consume any endline T_ENDL before start of block statements...
            auto stmt = parse_decl();
            block.statements.push_back(std::move(stmt));
        }
        node->node = std::move(block);

        expect_token_or_emit_error(T_RCURLY, "Expect a closing '}' after block.");
        if (expect_token(T_ENDL))
            expect_token_or_emit_error(T_ENDL, "Expect ';' or newline after block declaration.");

        return node;
    }
    // parse statement
    ast_node_ptr parser::parse_stmt()
    {
        if (expect_token_and_read(T_IF))
        {
            return parse_if_stmt();
        }
        else if (expect_token_and_read(T_DO))
        {
            return parse_do_while_stmt();
        }
        else if (expect_token_and_read(T_WHILE))
        {
            return parse_while_stmt();
        }
        else if (expect_token_and_read(T_RETURN))
        {
            return parse_return_stmt();
        }
        else if (expect_token_and_read(T_LCURLY))
        {
            return parse_block();
        }
        else if (expect_token_and_read(T_COMMAND))
        {
            auto node = new_node(N_COMMAND_DECL);
            return node;
        }
        return parse_expr_stmt();
    }
    ast_node_ptr parser::parse_if_stmt()
    {
        ast_node_ptr node = new_node(N_IF_STMT);
        expect_token_or_emit_error(T_LPAREN, "Expect '(' after if.");
        if_stmt if_stmt{};
        if_stmt.condition = parse_expr();
        expect_token_or_emit_error(T_RPAREN, "Expect a closing ')' after if condition.");
        if_stmt.then_ = parse_stmt();
        // if there's an ELSE_IF...
        // actually no need for else if as else if is just else and if...
        // so it gets parsed nonetheless
        if (expect_token_and_read(T_ELSE))
        {
            if_stmt.else_ = parse_stmt();
        }
        node->node = std::move(if_stmt);
        return node;
    }
    ast_node_ptr parser::parse_ternary_if_stmt()
    {
        ast_node_ptr node = new_node(N_IF_STMT);
        // expect_token_or_emit_error(T_LPAREN, "Expect '(' after if.");
        if_stmt if_stmt{};
        if_stmt.condition = parse_expr();
        // expect_token_or_emit_error(T_RPAREN, "Expect a closing ')' after if condition.");
        expect_token_or_emit_error(T_THEN, "Expect 'then' after if condition.");
        if_stmt.then_ = parse_stmt();
        // if there's an ELSE_IF...
        // actually no need for else if as else if is just else and if...
        // so it gets parsed nonetheless
        if (expect_token_and_read(T_ELSE))
        {
            if_stmt.else_ = parse_stmt();
        }
        node->node = std::move(if_stmt);
        return node;
    }
    // parse a do while statement
    //  do { ... } while (condition);
    ast_node_ptr parser::parse_do_while_stmt()
    {
        ast_node_ptr node = new_node(N_DO_WHILE_STMT);
        expect_token_or_emit_error(T_LCURLY, "Expect '{' after 'do'.");
        do_while_stmt do_while_stmt{};
        do_while_stmt.body = parse_stmt();
        expect_token_or_emit_error(T_WHILE, "Expect 'while' after 'do' block.");
        expect_token_or_emit_error(T_LPAREN, "Expect '(' after 'while'.");
        do_while_stmt.condition = parse_expr();
        expect_token_or_emit_error(T_RPAREN, "Expect a closing ')' after 'while' condition");

        node->node = std::move(do_while_stmt);
        return node;
    }
    ast_node_ptr parser::parse_while_stmt()
    {
        ast_node_ptr node = new_node(N_WHILE_STMT);
        expect_token_or_emit_error(T_LPAREN, "Expect '(' after 'while'.");
        while_stmt while_stmt{};
        while_stmt.condition = parse_expr();
        expect_token_or_emit_error(T_RPAREN, "Expect a closing ')' after 'while' condition");
        while_stmt.body = parse_stmt();
        node->node = std::move(while_stmt);
        return node;
    }
    ast_node_ptr parser::parse_return_stmt()
    {
        ast_node_ptr node = new_node(N_RET_STMT);
        ret_stmt ret{};
        if (!expect_token(T_ENDL))
        {
            ret.expr = parse_expr();
        }
        node->node = std::move(ret);
        expect_token_or_emit_error(T_ENDL, "Expect ';' or newline after reurn value.");
        return node;
    }
    ast_node_ptr parser::parse_expr_stmt()
    {
        auto node = new_node(N_EXPR_STMT);
        expr_stmt es{};
        es.expr = parse_expr();
        node->node = std::move(es);
        if (expect_token(T_ENDL))
            expect_token_or_emit_error(T_ENDL, "Expect ';' or newline after expression.");
        return node;
    }
    ast_node_ptr parser::parse_expr()
    {
        return parse_assign_expr();
    }
    ast_node_ptr parser::parse_assign_expr()
    {
        ast_node_ptr node = parse_logor_expr();
        if (expect_token_and_read(T_ASSIGN))
        {
            ast_node_ptr assign_node = new_node(N_ASSIGNMENT);
            assign_node->tok = prev_tok;

            assign_expr assign{};
            assign.left = std::move(node);
            assign.right = parse_assign_expr();
            assign_node->node = std::move(assign);

            return assign_node;
        }
        return node;
    }
    ast_node_ptr parser::parse_logor_expr()
    {
        ast_node_ptr node = parse_logand_expr();
        while (expect_token_and_read(T_OR) || expect_token_and_read(T_LOGICAL_OR))
        {
            ast_node_ptr bin_node = new_node(N_BINARY_EXPR);
            bin_node->tok = prev_tok;
            binary_expr bin{};

            bin.left = std::move(node);
            bin.right = parse_logand_expr();

            bin_node->node = std::move(bin);

            node = std::move(bin_node);
        }
        return node;
    }
    ast_node_ptr parser::parse_logand_expr()
    {
        ast_node_ptr node = parse_equality_expr();
        while (expect_token_and_read(T_AND) || expect_token_and_read(T_LOGICAL_AND))
        {
            ast_node_ptr bin_node = new_node(N_BINARY_EXPR);
            bin_node->tok = prev_tok;
            binary_expr bin{};
            bin.left = std::move(node);
            bin.right = parse_equality_expr();
            bin_node->node = std::move(bin);
            node = std::move(bin_node);
        }
        return node;
    }
    ast_node_ptr parser::parse_equality_expr()
    {
        ast_node_ptr node = parse_comparison_expr();
        while (expect_token_and_read(T_EQUALITY) || expect_token_and_read(T_NEQ))
        {
            ast_node_ptr bin_node = new_node(N_BINARY_EXPR);
            bin_node->tok = prev_tok;
            binary_expr bin{};
            bin.left = std::move(node);
            bin.right = parse_comparison_expr();
            bin_node->node = std::move(bin);
            node = std::move(bin_node);
        }
        return node;
    }
    ast_node_ptr parser::parse_comparison_expr()
    {
        ast_node_ptr node = parse_term_expr();
        while (expect_token_and_read(T_LESS_THAN) || expect_token_and_read(T_LESS_OR_EQUAL) || expect_token_and_read(T_GREATER_THAN) || expect_token_and_read(T_GREATER_OR_EQUAL))
        {
            ast_node_ptr bin_node = new_node(N_BINARY_EXPR);
            bin_node->tok = prev_tok;
            binary_expr bin{};
            bin.left = std::move(node);
            bin.right = parse_term_expr();
            bin_node->node = std::move(bin);
            node = std::move(bin_node);
        }

        return node;
    }
    ast_node_ptr parser::parse_term_expr()
    {
        ast_node_ptr node = parse_factor_expr();
        while (expect_token_and_read(T_PLUS) || expect_token_and_read(T_MINUS))
        {
            ast_node_ptr bin_node = new_node(N_BINARY_EXPR);
            bin_node->tok = prev_tok;
            binary_expr bin{};
            bin.left = std::move(node);
            bin.op = bin_node->tok; // operand in ht emiddle of the binary expr...
            bin.right = parse_factor_expr();
            bin_node->node = std::move(bin);
            node = std::move(bin_node);
        }
        return node;
    }
    ast_node_ptr parser::parse_unary_expr()
    {
        if (expect_token_and_read(T_MINUS) || expect_token_and_read(T_NOT))
        {
            ast_node_ptr node = new_node(N_UNARY);
            node->tok = prev_tok;
            unary_expr unary{};
            unary.operand = parse_unary_expr();
            node->node = std::move(unary);
            return node;
        }
        else if (expect_token_and_read(T_IF))
        {
            // std::cout << "Parsing a ternary-like expression..." << std::endl;
            return parse_ternary_if_stmt(); // this is a ternary-like expression...
        }
        return parse_primary_expr();
    }

    ast_node_ptr parser::parse_primary_expr()
    {
        if (expect_token_and_read(T_NLITERAL) || expect_token_and_read(T_SLITERAL) || expect_token_and_read(T_BLITERAL))
        {
            ast_node_ptr node = new_node(N_LITERAL);
            node->tok = prev_tok;
            return node;
        }
        // if (expect_token_and_read(T_IDENT) && peek_token(T_DOT))
        // {
        //     // this is a member access...
        //     ast_node_ptr node = new_node(N_MEMBER_ACCESS);
        //     node->tok = prev_tok;
        //     member_access access{};
        //     access.object = std::move(node);
        //     expect_token_or_emit_error(T_DOT, "Expect '.' after identifier.");
        //     if (expect_token_and_read(T_IDENT))
        //     {
        //         access.member = new_node(N_IDENT);
        //         access.member->tok = prev_tok;
        //         node->node = std::move(access);
        //         return node;
        //     }
        // }
        if (expect_token_and_read(T_IDENT))
        {
            if (!expect_token(T_LPAREN)) // just identifyer
            {
                ast_node_ptr node = new_node(N_IDENT);
                node->tok = prev_tok;
                return node;
            }
            // else it's a function call...
            ast_node_ptr node = new_node(N_FUNC_CALL);
            node->tok = prev_tok;
            func_call call{};

            // expect_token_or_emit_error(T_IDENT, "Expect member name after type.");
            ast_node_ptr ident = new_node(N_IDENT);
            ident->tok = prev_tok;

            call.identifier = std::move(ident);
            if (expect_token_and_read(T_LPAREN))
            {
                if (!expect_token(T_RPAREN))
                {
                    do
                    {
                        if (call.arguments.size() >= 255)
                        {
                            emit_error("Too many arguments in function call.", *curr_tok);
                        }
                        call.arguments.push_back(std::move(parse_expr()));
                    } while (expect_token_and_read(T_COMMA));
                }
                expect_token_or_emit_error(T_RPAREN, "Expect a closing ')' after function arguments.");
            }
            node->node = std::move(call);
            return node;
        }
        if (expect_token_and_read(T_LSQUARE)) // I think this is for us to parse an array right like [1,2,3] or [1,2,3,4,5] or [1:10] or [1:10:2]
        {
            ast_node_ptr array_node = new_node(N_ARRAY);
            array_expr array{};
            if (!expect_token(T_RSQUARE))
            {
                do
                {
                    if (array.elements.size() >= 255)
                    {
                        emit_error("Too many elements in array.", *curr_tok);
                    }
                    auto element = parse_expr();
                    array.elements.push_back(std::move(element));
                } while (expect_token_and_read(T_COMMA));
            }
            array_node->node = std::move(array);
            expect_token_or_emit_error(T_RSQUARE, "Expect a closing ']' after array elements.");
            return array_node;
        }

        // if (expect_token_and_read(T_LPAREN))
        // {
        //     ast_node_ptr expr = parse_expr();
        //     expect_token_or_emit_error(T_RPAREN, "Expect a closing ')' after expression.");
        //     return expr;
        // }

        emit_error("Expect expression.", *curr_tok);
        return nullptr;
    }
    ast_node_ptr parser::parse_factor_expr()
    {
        ast_node_ptr node = parse_unary_expr();
        while (expect_token_and_read(T_STAR) || expect_token_and_read(T_FSLASH))
        {
            ast_node_ptr bin_node = new_node(N_BINARY_EXPR);
            bin_node->tok = prev_tok;
            binary_expr bin{};
            bin.left = std::move(node);
            bin.op = bin_node->tok; // operand in ht emiddle of the binary expr...
            bin.right = parse_unary_expr();
            bin_node->node = std::move(bin);
            node = std::move(bin_node);
        }
        return node;
    }

    void parser::print_ast_node(const ast_node_ptr &node, int indent = 0)
    {
        if (!node)
            return;

        std::string indentation(indent, ' ');
        std::cout << indentation << "Node Type: " << ast_node_type_to_string(node->type) << "\n";

        std::visit(
            [&](auto &&value)
            {
                using T = std::decay_t<decltype(value)>;
                if constexpr (std::is_same_v<T, program>)
                {
                    std::cout << indentation << "Program Node:\n";
                    print_ast_node(value.version, indent + 2);
                    for (const auto &decl : value.declarations)
                    {
                        print_ast_node(decl, indent + 2);
                    }
                }
                else if constexpr (std::is_same_v<T, func_decl>)
                {
                    std::cout << indentation << "Function Declaration:\n";
                    print_ast_node(value.type, indent + 2);
                    print_ast_node(value.identifier, indent + 2);
                    for (const auto &param : value.parameters)
                    {
                        print_ast_node(param, indent + 2);
                    }
                    print_ast_node(value.body, indent + 2);
                }
                else if constexpr (std::is_same_v<T, class_decl>)
                {
                    std::cout << indentation << "Class Declaration:\n";
                    print_ast_node(value.identifier, indent + 2);
                    for (const auto &member : value.members)
                    {
                        print_ast_node(member, indent + 2);
                    }
                }
                else if constexpr (std::is_same_v<T, var_decl>)
                {
                    std::cout << indentation << "Variable Declaration:\n";
                    print_ast_node(value.type, indent + 2);
                    print_ast_node(value.identifier, indent + 2);
                    print_ast_node(value.initializer, indent + 2);
                }
                else if constexpr (std::is_same_v<T, input_decl>)
                {
                    std::cout << indentation << "Input Declaration:\n";
                    print_ast_node(value.body, indent + 2);
                    for (const auto &member : value.members)
                    {
                        print_ast_node(member, indent + 2);
                    }
                }
                else if constexpr (std::is_same_v<T, output_decl>)
                {
                    std::cout << indentation << "Output Declaration:\n";
                    print_ast_node(value.body, indent + 2);
                }
                else if constexpr (std::is_same_v<T, runtime_decl>)
                {
                    std::cout << indentation << "Runtime Declaration:\n";
                    for (const auto &[identifier, value] : value.members)
                    {
                        print_ast_node(identifier, indent + 2);
                        print_ast_node(value, indent + 2);
                    }
                }
                else if constexpr (std::is_same_v<T, meta_decl>)
                {
                    std::cout << indentation << "Parameter Meta Declaration:\n";
                    print_ast_node(value.identifier, indent + 2);
                    for (const auto &[item, val] : value.members)
                    {
                        print_ast_node(item, indent + 2);
                        print_ast_node(val, indent + 2);
                    }
                }
                else if constexpr (std::is_same_v<T, command_decl>)
                {
                    std::cout << indentation << "Command Declaration:\n";
                    print_ast_node(value.body, indent + 2);
                }
                else if constexpr (std::is_same_v<T, block>)
                {
                    std::cout << indentation << "Block:\n";
                    for (const auto &stmt : value.statements)
                    {
                        print_ast_node(stmt, indent + 2);
                    }
                }
                else if constexpr (std::is_same_v<T, while_stmt>)
                {
                    std::cout << indentation << "While Statement:\n";
                    print_ast_node(value.condition, indent + 2);
                    print_ast_node(value.body, indent + 2);
                }
                else if constexpr (std::is_same_v<T, do_while_stmt>)
                {
                    std::cout << indentation << "Do While Statement:\n";
                    print_ast_node(value.body, indent + 2);
                    print_ast_node(value.condition, indent + 2);
                }
                else if constexpr (std::is_same_v<T, ret_stmt>)
                {
                    std::cout << indentation << "Return Statement:\n";
                    print_ast_node(value.expr, indent + 2);
                }
                else if constexpr (std::is_same_v<T, expr_stmt>)
                {
                    std::cout << indentation << "Expression Statement:\n";
                    print_ast_node(value.expr, indent + 2);
                }
                else if constexpr (std::is_same_v<T, block>)
                {
                    std::cout << indentation << "Block:\n";
                    for (const auto &stmt : value.statements)
                    {
                        print_ast_node(stmt, indent + 2);
                    }
                }
                else if constexpr (std::is_same_v<T, if_stmt>)
                {
                    std::cout << indentation << "If Statement:\n";
                    print_ast_node(value.condition, indent + 2);
                    print_ast_node(value.then_, indent + 2);
                    print_ast_node(value.else_if, indent + 2);
                    print_ast_node(value.else_, indent + 2);
                }
                else if constexpr (std::is_same_v<T, binary_expr>)
                {
                    std::cout << indentation << "Binary Expression:\n";
                    print_ast_node(value.left, indent + 2);
                    std::cout << indentation << "Operator: " << value.op->lexeme << "\n";
                    print_ast_node(value.right, indent + 2);
                }
                else if constexpr (std::is_same_v<T, array_expr>)
                {
                    std::cout << indentation << "Array Expression:\n";
                    print_ast_node(value.identifier, indent + 2);
                    for (const auto &element : value.elements)
                    {
                        print_ast_node(element, indent + 2);
                    }
                }
                else if constexpr (std::is_same_v<T, unary_expr>)
                {
                    std::cout << indentation << "Unary Expression:\n";
                    print_ast_node(value.operand, indent + 2);
                }
                else if constexpr (std::is_same_v<T, func_call>)
                {
                    std::cout << indentation << "Function Call:\n";
                    print_ast_node(value.identifier, indent + 2);
                    for (const auto &arg : value.arguments)
                    {
                        print_ast_node(arg, indent + 2);
                    }
                }
                else if constexpr (std::is_same_v<T, assign_expr>)
                {
                    std::cout << indentation << "Assignment Expression:\n";
                    print_ast_node(value.left, indent + 2);
                    print_ast_node(value.right, indent + 2);
                }
                else if constexpr (std::is_same_v<T, literal_expr>)
                {
                    std::cout << indentation << "Literal: " << value.value.lexeme << "\n";
                }
                else
                {
                    std::cout << indentation << "Unhandled Node Type\n";
                }
            },
            node->node);
    }
    // std::ostream &operator<<(std::ostream &os, const ast_node &node)
    // {
    //     os << "ast_node(type=" << ast_node_type_to_string(node.type) << ", tok=" << *node.tok << ")";
    //     return os;
    // }
}