#include "quantdriver.hh"

QuantDriver::QuantDriver(const std::fstream* source, const std::string formula){
    // parse traces
    if(parse_traces(source)){
        this->error_flag = FILE_PARSE_FAIL;
        return;
    }   
    
    // parse formula
    if(parse_formula(formula)){
        this->error_flag = FORM_PARSE_FAIL;
    }


    // create a context
    context = new Z3_context();

    this->error_flag = OK;
}

QuantDriver::~QuantDriver(){
    // delete context
    delete context;

    // delete trees
    this->ast->destroy_children();
    delete ast;

    // delete traces
}

int QuantDriver::parse_traces(const std::fstream *source){
    // do actual parsing
    // how to store traces??
}

int QuantDriver::parse_formula(const std::string formula){
    // initialize ast
    // do actual parsing
    // format for formulae??
}