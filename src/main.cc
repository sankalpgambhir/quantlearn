
#include "configuration.hh"
#include "quantdriver.hh"
#include "compdriver.hh"
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
    
    if (Configuration::vm.count("max-depth")){
        Configuration::throw_error("Max depth not specified, defaulting to " + std::to_string(MAX_DEPTH_DEFAULT));
    }

    int error_flag = 0;

    // instantiate a driver, create a context
    if (Configuration::vm.count("composition")){
        comp::CompDriver *driver = new comp::CompDriver(source, Configuration::max_depth);
        error_flag = driver->error_flag;
        delete driver;
    }
    else {
        QuantDriver *driver = new QuantDriver(source, formula);
        error_flag = driver->error_flag;
        delete driver;
    }

    // check if initialization and parsing was fine
    if(error_flag != OK){
        return error_flag;
    }

    // print obtained result

    // destroy internals and delete driver
    source->close();

    return OK;
}