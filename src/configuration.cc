
#include "configuration.hh"

namespace Configuration
{
    po::options_description desc("Options");
    po::variables_map vm;
    int max_depth;

    void init_options(){
        desc.add_options()
            ("help,h", "produce help message")
            ("optimization,o", "run the constraint optimization algorithm, default")
            ("composition,c", "run the compositional algorithm, ignores -f")
            ("positive-input,p", po::value<std::string>()->default_value(""), "input file for positive traces")
            ("negative-input,n", po::value<std::string>()->default_value(""), "input file for negative traces")
            ("formula,f", po::value<std::string>()->default_value(""), "formula pattern to look for")
            ("max-depth,m", po::value<int>(&max_depth)->default_value(MAX_DEPTH_DEFAULT), "maximum formula depth to search for")
        ;
    }

    void throw_error(std::string error_info){
        std::cerr << "Error: " << error_info << "!\n";
    }

    void throw_warning(std::string warning_info){
        std::cerr << "Warning: " << warning_info << "!\n";
    }
}
