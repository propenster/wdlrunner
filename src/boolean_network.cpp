#include "boolean_network.h"
#include <iostream>
namespace soto
{

    void boolean_network::update_network()
    {
        std::cout << "Boolean network update function is called" << std::endl;
    }
    void boolean_network::print_network()
    {
        std::cout << "Boolean network print network function is called" << std::endl;
        std::cout << "BN name >>> " << name << " value >>> " << value << std::endl;
    }

}