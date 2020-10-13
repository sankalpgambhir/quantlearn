
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
    if(!Configuration::vm.count("positive-input") || Configuration::vm["positive-input"].as<std::string>() == __empty){
        Configuration::throw_error("No positive trace input specified");
    }
    if(!Configuration::vm.count("negative-input") || Configuration::vm["negative-input"].as<std::string>() == __empty){
        Configuration::throw_error("No negative trace input specified");
    }
    if((Configuration::vm.count("composition") == 0) &&(!Configuration::vm.count("formula") || Configuration::vm["formula"].as<std::string>() == __empty)){
        Configuration::throw_error("No formula pattern specified");
    }

    // parse input file
        // throw error if none
    std::cout << Configuration::pos_file << std::endl;

    std::fstream p_source = std::fstream(Configuration::pos_file);
    std::fstream n_source = std::fstream(Configuration::neg_file);

    std::cout << Configuration::pos_file << std::endl;
    if(!p_source.is_open()){
        Configuration::throw_error("Could not open positive trace input");
        return FILE_OPEN_FAIL;
    }

    if(!n_source.is_open()){
        Configuration::throw_error("Could not open negative trace input");
        return FILE_OPEN_FAIL;
    }

    std::string formula = Configuration::vm["formula"].as<std::string>();
    
    if (Configuration::vm.count("max-depth")){
        Configuration::throw_warning("Max depth not specified, defaulting to " + std::to_string(MAX_DEPTH_DEFAULT));
    }

    int error_flag = 0;

    // instantiate a driver, create a context
    if (Configuration::vm.count("composition")){
        comp::CompDriver *driver = new comp::CompDriver(p_source, n_source, Configuration::max_depth);
        error_flag = driver->error_flag;
        delete driver;
    }
    else {
        QuantDriver *driver = new QuantDriver(p_source, n_source, formula);
        error_flag = driver->error_flag;
        delete driver;
    }

    // check if initialization and parsing was fine
    if(error_flag != OK){
        return error_flag;
    }

    // print obtained result
        // left to driver for now, better if cleaned to be here

    std::cout << std::endl;

    return OK;
}