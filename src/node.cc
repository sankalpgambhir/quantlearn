
#include "node.hh"

// initialize static variables as needed
int Node::node_total = 0;

std::ostream& operator << (std::ostream& os, Node const* n){

    switch (op_arity(n->label))
    {
    case 0:
        // proposition or subform or empty
        if(n->label == ltl_op::Proposition){
            os << n->prop_label;
        }
        else if(n->label == ltl_op::Subformula){
            os << operators[n->label] << '(' << n->subformula_size << ')';
        }
        else{
            assert(0 && "Tried to print empty node");
        }
        break;
    
    case 1:
        os << operators[n->label] << n->left;
        break;
    
    case 2:
        os << n->right << operators[n->label] << n->left;
        break;
    
    default:
        break;
    }

    return os;
}