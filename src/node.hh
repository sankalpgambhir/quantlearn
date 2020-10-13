#include <iostream>
#include "configuration.hh"

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
    Node(ltl_op lab, Node * left_node, Node * right_node)
            : label(lab), left(left_node), right(right_node){}


    ~Node(){
        //this->destroy_children(); // not really needed, we don't dynamically allocate more than once ig
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