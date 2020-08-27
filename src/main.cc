
#include "configuration.hh"
#include <iostream>


int main(int argc, char* argv[]){

    // parse command line opts
    Configuration::init_options();
    po::store(po::parse_command_line(argc, argv, Configuration::desc), Configuration::vm);
    po::notify(Configuration::vm);    

    if (Configuration::vm.count("help")) {
        std::cout << Configuration::desc << "\n";
        return 0;
    }

    // parse input file
        // throw error if none
    
    // pass to quantdriver

    // print obtained result

    return 0;
}