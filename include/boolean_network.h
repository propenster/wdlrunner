#ifndef BN_H
#define BN_H
#include <string>

namespace soto
{

    struct boolean_network
    {
    public:
        boolean_network(std::string name, int val) : name(name), value(std::move(val))
        {
        }
        boolean_network() = default;

        std::string name;
        int value;

        void update_network();
        void print_network();
    };

}

#endif