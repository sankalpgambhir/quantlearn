
#include "configuration.hh"

namespace Configuration
{
    po::options_description desc("Options");
    po::variables_map vm;

    void init_options(){
        desc.add_options()
            ("help,h", "produce help message")
            ("input-file,i", po::value<std::string>()->default_value(""), "input file")
            ("formula,f", po::value<std::string>()->default_value(""), "formula pattern to look for")
        ;
    }

    void throw_error(std::string error_info){
        std::cerr << "Error: " << error_info << "!\n";
    }

    void static_init(){
        // initialize static variables as needed
        Trace::trace_count = 0;
    }
}
