
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
        Configuration::throw_error("No input file specified");
    }
    if(!Configuration::vm.count("formula") || Configuration::vm["formula"].as<std::string>() == __empty){
        Configuration::throw_error("No formula pattern specified");
    }

    // parse input file
        // throw error if none
    std::fstream *source;
    source->open(Configuration::vm["input-file"].as<std::string>());

    if(!source->is_open()){
        Configuration::throw_error("Could not open file");
        return FILE_OPEN_FAIL;
    }

    std::string formula = Configuration::vm["formula"].as<std::string>();

    // instantiate a driver, create a context
    QuantDriver *driver = new QuantDriver(source, formula);

    if(driver->error_flag != OK){
        return driver->error_flag;
    }

    // run driver
    driver->run();

    // print obtained result

    // destroy internals and delete driver
    source->close();
    delete driver;

    return OK;
}