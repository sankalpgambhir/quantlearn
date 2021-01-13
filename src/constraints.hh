#ifndef __CONSTRAINTS_HH__
#define __CONSTRAINTS_HH__

#include <z3++.h>
#include "node.hh"
#include "trace.hh"

class ConstraintSystem{
    public:

        ConstraintSystem() = default;
        bool is_sub_formula = false;
        z3::expr and_score_constraints(z3::context& c);
        z3::expr node_constraints(z3::context& c, Node * ast_node, Trace &trace);
        z3::expr leaf_constraints(z3::context& c);
        std::vector<z3::expr> score_constraint;
        z3::expr score_constraints(z3::context &c, Node *astNode, Trace &trace);
        z3::expr score_constraints_pattern(z3::context &c, Node *astNode, Trace &trace);
        z3::expr pat_to_prop_map(z3::context &c, std::vector<std::string> &pat_var, Trace &t);
        void init_score(z3::context &c, Node *ast, Trace &t);
        std::string construct_formula(z3::model& modl, Node* ast);
        std::string construct_formula_if_node_label(z3::model& modl, Node* ast);
        std::string get_node_val_mapped(z3::model& modl, Node* ast);
        std::vector<z3::expr> prop_constr_pos(z3::context &c, Node * astNode, Trace &t, int pos);
        std::vector<z3::expr> subformula_constr_pos(z3::context &c, Node * astNode, Trace &t, int pos);

    private:
       
        


        z3::expr valuation(z3::context &c, Node *node, Trace &t, const int pos = 0);
        z3::expr valuation_not(z3::context &c, Node *node, Trace &t, const int pos = 0);
        z3::expr valuation_until(z3::context &c, Node *node, Trace &t, const int pos = 0, const int offset = 0);
        z3::expr valuation_G(z3::context &c, Node *node, Trace &t, const int pos = 0);
        z3::expr valuation_F(z3::context &c, Node *node, Trace &t, const int pos = 0);

        

};

#endif