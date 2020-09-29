#include "compdriver.hh"

comp::CompDriver::CompDriver(const std::fstream* source, const int max_depth){
    // parse the traces
    if(!parse_traces(source)){
        CompDriver::error_flag = FILE_PARSE_FAIL;
        return;
    }   

    // start with basic formulae
    // form_set = {G, F} x prop_set

    // run while depth not reached
    for(int i = 1; i < max_depth; i++){
        this->run();
    }
}

void comp::CompDriver::run(){

    // run a check on the current set
    for(auto form : form_set){
        // check(form);
    }

    // compose the current operators


}