#include <iostream>
#include <vector>
#include "boolean_network.h"
#include <token.h>
#include <lexer.h>
#include <fstream>

#include <sstream>
#include <stdexcept>
#include <string>
#include "soto.h"
#include <parser.h>

// read file content into a string...
// mostly used for source code reading in this codebase...
std::string read_file(const std::string &path)
{
    std::ifstream file(path);
    if (!file)
        throw std::runtime_error("Failed to open file: " + path);
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}
void write_file(const std::string &path, const std::string &content)
{
    std::ofstream file(path);
    if (!file)
        throw std::runtime_error("Failed to open file: " + path);
    file << content;
    if (!file)
        throw std::runtime_error("Failed to write to file: " + path);
    file.close();
    if (!file)
        throw std::runtime_error("Failed to close file: " + path);
}
void print_help()
{
    std::cout << "wdlrunner v1.0" << std::endl;
    std::cout << "A simple WDL runner." << std::endl;
    std::cout << "Usage: wdlrunner <source_file>" << std::endl;
    std::cout << "Example: wdlrunner ../test3.wdl" << std::endl;
}

void print_version()
{
    std::cout << "wdlrunner v1.0" << std::endl;
}
int main(int argc, char *argv[])
{
    std::cout << "This is a test of the Workflow Definition Language (WDL) Runner v1.0\n";

    // if (argc < 2)
    // {
    //     print_help();
    //     return 1;
    // }
    // if (argc == 3 && std::string(argv[1]) == "--version")
    // {
    //     print_version();
    //     return 0;
    // }
    // if (argc == 2 && std::string(argv[1]) == "--help")
    // {
    //     print_help();
    //     return 0;
    // }
    // std::string source_code = read_file(argv[1]);
    // std::cout << "Source code read from file:\n"
    //           << source_code << std::endl;

    std::string source_code = read_file("../test2.wdl");
    std::cout << "Source code read from file:\n"
              << source_code << std::endl;

    soto::lexer lexer{source_code};
    soto::parser parser{std::make_unique<soto::lexer>(lexer)};
    const soto::ast_node_ptr prog = parser.parse_program();
    std::cout << "Parsed program: " << std::endl;
    parser.print_ast_node(prog, 0);
    // parser.write_ast_node_to_file(prog, "output.ast", 0);

    std::cout << "AST written to output.ast\n";

    std::cout << "Parsed program successfully.\n";

    return 0;
}