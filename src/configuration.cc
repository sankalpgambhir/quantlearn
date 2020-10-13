
#include "configuration.hh"


namespace Configuration
{
    po::options_description desc("Options");
    po::variables_map vm;
    int max_depth;
    std::string pos_file, neg_file;

    void init_options(){
        desc.add_options()
            ("help,h", "produce help message")
            ("optimization,o", "run the constraint optimization algorithm, default")
            ("composition,c", "run the compositional algorithm, ignores -f")
            ("positive-input,p", po::value<std::string>(&pos_file)->default_value(""), "input file for positive traces")
            ("negative-input,n", po::value<std::string>(&neg_file)->default_value(""), "input file for negative traces")
            ("formula,f", po::value<std::string>()->default_value(""), "formula pattern to look for")
            ("max-depth,m", po::value<int>(&max_depth)->default_value(0), "maximum formula depth to search for")
        ;
    }

    void throw_error(std::string error_info){
        std::cerr << "\n\033[1;31mError: " << error_info << "!\033[0m\n";
    }

    void throw_warning(std::string warning_info){
        std::cerr << "\n\033[1;33mWarning: " << warning_info << "!\033[0m\n";
    }
}

std::map<ltl_op, char> operators = {
    {Empty , (char) 0}, // Free
    {Not , '-'},
    {Or , '+'},
    {And , '*'},
    #if GF_FRAGMENT
    //
    #else
        {Next , 'X'},
        {Until , 'U'},
    #endif
    {Globally , 'G'},
    {Finally , 'F'},
    {Subformula , 'S'},
    {Proposition , (char) 0}
};

const int op_arity(ltl_op o){

    switch (o)
    {
        case ltl_op::Not:
        case ltl_op::Globally:
        case ltl_op::Finally:

        #if GF_FRAGMENT
        #else
        case ltl_op::Next:
        #endif

            return 1;

        case ltl_op::And:
        case ltl_op::Or:
        
        #if GF_FRAGMENT
        #else
        case ltl_op::Until:
        #endif

            return 2;
        
        default:
            return 0;
    }
}