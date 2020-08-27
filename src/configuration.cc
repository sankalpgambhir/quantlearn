
#include "configuration.hh"

//namespace po = boost::program_options;

po::options_description Configuration::desc("Options");
po::variables_map Configuration::vm;

void Configuration::init_options(){
    desc.add_options()
        ("help,h", "produce help message")
        ("input-file,i", po::value<std::string>(), "input file")
        ("formula,f", po::value<std::string>(), "formula pattern to look for")
    ;
}