#ifndef __CONFIGURATION_HH__
#define __CONFIGURATION_HH__

#include <boost/program_options.hpp>
#include <iostream>
#include "quantdriver.hh"


// actual config options
#define GF_FRAGMENT 1
#define NNF 1
#define TRACE_DELIMITER # // TODO: input delimiters from user
#define STEP_DELIMITER ,
#define MAX_DEPTH_DEFAULT 4

// config based switches


// descriptive macros
#define __empty ""

// return values
#define OK 0
#define FILE_OPEN_FAIL 1
#define FILE_PARSE_FAIL 2
#define FORM_PARSE_FAIL 3

namespace po = boost::program_options;

namespace Configuration{
    extern void static_init();
    extern void init_options();
    extern void throw_error(std::string);
    extern po::options_description desc;
    extern po::variables_map vm;
}

#endif