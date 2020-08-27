#ifndef __QUANTDRIVER_HH__
#define __QUANTDRIVER_HH__

#include <string>
#include <vector>
#include <map>
#include <z3++.h>
#include <fstream>
#include "configuration.hh"

typedef int op_t;

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
    Proposition
};

struct Node{
    Node() : label(Empty), prop_label(""), left(nullptr), right(nullptr){}

    ltl_op label;
    std::string prop_label;
    Node *left, *right;

    void destroy_children(){
        if(left){
            left->destroy_children();
            delete left;
        }
        if(right){
            right->destroy_children();
            delete right;
        }
    }
};

struct Trace{

    struct proposition{
        std::string name;
        std::vector<std::pair<int, int> > instances;
    };

    std::map<std::string, proposition> prop_inst;
    int length;
};

class QuantDriver{
    public:
        QuantDriver(const std::fstream* source, const std::string formula);
        QuantDriver(const QuantDriver&) = delete;
        QuantDriver& operator=(const QuantDriver&) = delete;

        ~QuantDriver();

        static int error_flag;

    private:
        static int parse_traces(const std::fstream* source);
        static int parse_formula(const std::string formula);

        Node *ast; // syntax tree
        std::vector<Trace> *traces;

        Z3_context* context; // optimization context
};

#endif