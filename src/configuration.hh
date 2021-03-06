#ifndef __CONFIGURATION_HH__
#define __CONFIGURATION_HH__

#include <boost/program_options.hpp>
#include <iostream>


// actual config options
#define GF_FRAGMENT 1
#define NNF 1
#define TRACE_DELIMITER '#' // TODO: input delimiters from user
#define STEP_DELIMITER ';'
#define PROP_DELIMITER ','
#define MAX_DEPTH_DEFAULT 2
#define NUM_TO_PRINT 5

// config based switches


// descriptive macros
#define __empty ""

// return values
#define OK 0
#define FILE_OPEN_FAIL 1
#define FILE_PARSE_FAIL 2
#define FORM_PARSE_FAIL 3
#define NO_RESULT 4

// debugging
#define VERBOSE (void*) 0
//#define NVERBOSE 1
#ifndef NVERBOSE
    #define IFVERBOSE(x) x
#else
    #define IFVERBOSE(x) (void*) 0;
#endif

namespace po = boost::program_options;

namespace Configuration{
    extern void init_options();
    extern void throw_error(std::string);
    extern void throw_warning(std::string);
    extern po::options_description desc;
    extern po::variables_map vm;
    extern int max_depth;
    extern std::string pos_file, neg_file;
}

enum parity_t{
    negative,
    positive
};

enum ltl_op {
    Empty, // Free
    Not,
    Or,
    And,
    #if GF_FRAGMENT
    //
    #else
        Next,
        Until,
    #endif
    Globally,
    Finally,
    Proposition,
    Subformula
};

extern std::map<ltl_op, char> operators;

const int op_arity(ltl_op o);

#endif