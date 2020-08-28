#ifndef __QUANTDRIVER_HH__
#define __QUANTDRIVER_HH__

#include <string>
#include <vector>
#include <map>
#include <z3++.h>
#include <fstream>
#include <thread>
#include <algorithm>
#include "configuration.hh"

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
    Subformula,
    Proposition
};

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

const int op_arity(ltl_op o);

struct Node{
    Node()  : label(Empty), prop_label(""), subformula_size(0),
                left(nullptr), right(nullptr){}

    Node(Node& copy, ltl_op new_label = Empty) 
            : prop_label(""), subformula_size(0),
                left(nullptr), right(nullptr){
        if (new_label == Empty){
            this->label = copy.label;
        }
        else{
            this->label = new_label;
        }
    }

    ~Node(){
        this->destroy_children();
    }

    ltl_op label;
    int subformula_size;
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
    std::vector<std::string> trace_string; // original trace
    int length;
};

struct Result{
    std::string output_formula;
    float value;
};

class QuantDriver{
    public:
        QuantDriver(const std::fstream* source, const std::string formula);

        // create a driver from a parsed trace set, for parallelization
        QuantDriver(std::vector<Trace> *traces, Node* ast);
        QuantDriver(const QuantDriver&) = delete;
        QuantDriver& operator=(const QuantDriver&) = delete;

        ~QuantDriver();

        void run();
        void run_parallel();

        static int error_flag;
        Result result;

    private:
        static int parse_traces(const std::fstream* source);
        static int parse_formula(const std::string formula);

        static void construct_ast(Node* ast, int depth);

        Node *ast; // syntax tree
        std::vector<Trace> *traces;

        Z3_context* context; // optimization context

        int max_depth;
};

#endif