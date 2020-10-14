#ifndef __TRACE_HH__
#define __TRACE_HH__


#include <map>
#include <string>
#include <vector>
#include <z3++.h>
#include "configuration.hh"

struct Trace{

    Trace() : id(Trace::trace_count++){
        this->length = 0;
        this->prop_inst.clear();
        this->parity = parity_t::positive; // by default must show in score calculation
        this->trace_string.clear();
    }

    //Trace(const Trace&) = default; // copy constructor -- needed for parallel quantdriver
    //Trace(Trace&&) = default; // move constructor

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

    //void construct_bit_matrices(z3::context& c, const int ast_size);
    //void construct_bit_matrices1(z3::context& c, Node * ast_node);//Consider only one from line number 100,101
    void compute_prop_counts();
    void compute_not_props();
    void compute_optimizations();

    bool isPropExistAtPos(int pos, std::string prop_name);

    // calculating stuff for this trace
    // Not sure about usage of z3::expr_vector. Possible optimization?
    std::map<int,std::vector<z3::expr>> score;
    std::vector<z3::expr> score_constr;

    // constraints

    // logistics
    bool operator==(const Trace& t){
        return this->id == t.id;
    }
};

#endif