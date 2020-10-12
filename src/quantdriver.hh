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

enum parity_t{
    negative,
    positive
};

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
    Proposition,
    Subformula
};

extern std::map<ltl_op, char> operators;

const int op_arity(ltl_op o);

inline const float retarder(float f){
    return exp(-f);
}

struct Node{
    Node()  : label(Empty), subformula_size(0), prop_label(__empty), 
                left(nullptr), right(nullptr)
            {
                this->id = Node::node_total++;
            }

    Node(Node& copy, ltl_op new_label = Empty) 
            : subformula_size(0), prop_label(__empty), 
                left(nullptr), right(nullptr){
        if (new_label == Empty){
            this->label = copy.label;
        }
        else{
            this->label = new_label;
        }
        this->id = Node::node_total++;
    }

    ~Node(){
        this->destroy_children();
    }

    int id;
    static int node_total;
    ltl_op label;
    int subformula_size;
    std::string prop_label;
    Node *left, *right;
    std::vector<std::vector<bool> > holds;

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

    friend std::ostream& operator << (std::ostream& os, Node const* n);

};

struct Trace{

    Trace(){
        this->id = Trace::trace_count++;
    }

    //Trace(const Trace&) = delete;

    int id;
    static int trace_count;

    struct proposition{
        struct instance{
            instance(int pos = 0, int num = 0, int posn = 0)
            : position(pos), num_after(num), pos_next(posn){}
            int position;
            int num_after;
            int pos_next;
        };

        proposition(std::string n) : name(n){}
        proposition() {}

        std::string name;
        std::vector<instance> instances; // <position, props after, next prop>
        
    };

    std::map<std::string, proposition> prop_inst;
    std::vector<std::vector<std::string> > trace_string; // original trace
    int length;
    parity_t parity;

    void construct_bit_matrices(z3::context& c, const int ast_size);
    void construct_bit_matrices1(z3::context& c, Node * ast_node);//Consider only one from line number 100,101
    void compute_prop_counts();
    void compute_not_props();
    void compute_optimizations();

    bool isPropExistAtPos(int pos, std::string prop_name);
    z3::expr valuation(z3::context &c, Node *node, int pos);
    void score_constraints(z3::context &c, Node *astNode);
    z3::expr node_constraints(z3::context& c, Node * ast_node);
    void merged_x_xp(Node *ast_node);
    z3::expr leaf_constraints(z3::context& c, std::vector<std::vector<z3::expr>> &vec__vec_expr);
    z3::expr all_constraints(Node * ast_node);
    z3::expr leaf_constraints(z3::context& c);
    //z3::expr do_and(z3::expr e1,z3::expr e2);
    //z3::expr do_or(z3::expr e1,z3::expr e2);



    // calculating stuff for this trace
    // Not sure about usage of z3::expr_vector. Possible optimization?
    //std::vector<std::vector<z3::expr> > x; // matrix of bools
    //std::vector<std::vector<z3::expr> > xp;
    std::map<int,std::vector<z3::expr>> x; // taking map of node_id and vector of expressions of that node
    std::map<int,std::vector<z3::expr>> xp;
    std::map<int,std::vector<z3::expr>> merged_xxp;
    std::map<int,std::vector<z3::expr>> score;
    std::vector<std::vector<z3::expr>> leaf_constr;
    std::vector<z3::expr> score_constr;

    // constraints

    // logistics
    bool operator==(const Trace& t){
        return this->id == t.id;
    }
};

struct Result{
    std::string output_formula;
    float value;
};

class QuantDriver{
    public:
        QuantDriver(const std::fstream* p_source, 
                    const std::fstream* n_source, 
                    const std::string formula);

        // create a driver from a parsed trace set, for parallelization
        QuantDriver(std::vector<Trace> *traces, Node* ast);
        QuantDriver(const QuantDriver&) = delete;
        QuantDriver& operator=(const QuantDriver&) = delete;

        ~QuantDriver();

        void run();
        void run_parallel();

        int error_flag;
        static int ast_size;
        Result result;

    private:
        bool parse_traces(const std::fstream* p_source, const std::fstream* n_source);
        bool parse_formula(const std::string formula);

        static void construct_ast(Node* ast, int depth);

        Node *ast; // syntax tree
        std::vector<Trace> *traces;

        z3::context opt_context; // optimization context

        int max_depth;
};

#endif
