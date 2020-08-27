#ifndef __CONFIGURATION_HH__
#define __CONFIGURATION_HH__

#include <boost/program_options.hpp>

namespace po = boost::program_options;

namespace Configuration{
    extern void init_options();
    extern po::options_description desc;
    extern po::variables_map vm;
}

#endif