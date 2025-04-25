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
int main(int argc, char *argv[])
{
    std::cout << "This is a test of the Workflow Definition Language (WDL) Runner v1.0\n";

    // if (argc < 2)
    // {
    //     std::cerr << "Usage: " << argv[0] << " <source_file>" << std::endl;
    //     return 1;
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

    std::cout << "Parsed program successfully.\n";

    return 0;
}