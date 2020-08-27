
#include "configuration.hh"

namespace Configuration
{
    po::options_description desc("Options");
    po::variables_map vm;

    void init_options(){
        desc.add_options()
            ("help,h", "produce help message")
            ("input-file,i", po::value<std::string>(), "input file")
            ("formula,f", po::value<std::string>(), "formula pattern to look for")
        ;
    }
}
