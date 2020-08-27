
#include "configuration.hh"
#include "quantdriver.hh"
#include <iostream>


int main(int argc, char* argv[]){

    // parse command line opts
    Configuration::init_options();
    po::store(po::parse_command_line(argc, argv, Configuration::desc), Configuration::vm);
    po::notify(Configuration::vm);    

    // print options
    if (Configuration::vm.count("help")){
        std::cout << Configuration::desc << "\n";
        return 0;
    }
    
    // were all options passed?
    if(!Configuration::vm.count("input-file") || Configuration::vm["input-file"].as<std::string>() == __empty){
        std::cerr << "Error: No input file specified!" << "\n";
    }
    if(!Configuration::vm.count("formula") || Configuration::vm["formula"].as<std::string>() == __empty){
        std::cerr << "Error: No formula pattern specified!" << "\n";
    }

    // parse input file
        // throw error if none
    std::fstream *source;
    source->open(Configuration::vm["input-file"].as<std::string>());

    if(!source->is_open()){
        std::cerr << "Error: Could not open file!" << "\n";
        return FILE_OPEN_FAIL;
    }

    // instantiate a driver, create a context
    QuantDriver *driver = new QuantDriver(source);

    if(driver->error_flag){
        return driver->error_flag;
    }
    
    // pass to quantdriver

    // print obtained result

    // destroy internals and delete driver
    source->close();
    delete driver;

    return OK;
}