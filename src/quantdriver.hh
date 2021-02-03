#ifndef __QUANTDRIVER_HH__
#define __QUANTDRIVER_HH__

#include <string>
#include <vector>
#include <cmath>
#include <map>
#include <z3++.h>
#include <fstream>
#include <sstream>
#include <thread>
#include <future>
#include <algorithm>
#include "configuration.hh"
#include "constraints.hh"
#include "trace.hh"
#include "node.hh"

inline const float retarder(float f){
    float val = exp(-f);
    // float val1 = (int)(val * 1000); //roundoff upto three digit
    // return (float)val1 / 1000; 
    return val;
}

struct Result{
    std::string output_formula;
    float value;
};

class QuantDriver{
    public:
        QuantDriver(const std::fstream &p_source, 
                    const std::fstream &n_source, 
                    const std::string formula);

        // create a driver from a parsed trace set, for parallelization
        QuantDriver(std::vector<Trace> &traces, Node* ast);
        QuantDriver(const QuantDriver&) = delete;
        QuantDriver& operator=(const QuantDriver&) = delete;

        ~QuantDriver();

        void run();
        void run_parallel();

        int error_flag;
        static int ast_size;
        Result result;

    private:
        bool parse_traces(const std::fstream &p_source, const std::fstream &n_source);
        bool parse_formula(const std::string formula);

        static void construct_ast(Node* ast, int depth);

        Node *ast; // syntax tree
        std::vector<Trace> traces;

        ConstraintSystem consys;
        z3::context opt_context; // optimization context

        int max_depth;


};

#endif
