#ifndef __QUANTDRIVER_HH__
#define __QUANTDRIVER_HH__

#include <string>
#include <vector>
#include <map>
#include <z3++.h>

typedef int op_t;

enum ltl_op {
    Empty,
    Not,
    Or,
    And,
    Next,
    Until,
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
        QuantDriver();
        QuantDriver(const QuantDriver&) = delete;
        QuantDriver& operator=(const QuantDriver&) = delete;

    private:
        Node *ast;
        std::vector<std::string> *traces;
};

#endif