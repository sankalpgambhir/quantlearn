
#include "configuration.hh"

namespace Configuration
{
    po::options_description desc("Options");
    po::variables_map vm;

    void init_options(){
        desc.add_options()
            ("help,h", "produce help message")
            ("optimization,o", "run the constraint optimization algorithm, default")
            ("composition,c", "run the compositional algorithm, ignores -f")
            ("input-file,i", po::value<std::string>()->default_value(""), "input file")
            ("formula,f", po::value<std::string>()->default_value(""), "formula pattern to look for")
            ("max-depth,m", po::value<int>(&max_depth)->default_value(MAX_DEPTH_DEFAULT), "maximum formula depth to search for")
        ;
    }

    void throw_error(std::string error_info){
        std::cerr << "Error: " << error_info << "!\n";
    }
}
