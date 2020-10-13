#include <z3++.h>
#include "node.hh"
#include "trace.hh"

class ConstraintSystem{
    public:

    private:
        z3::expr all_constraints(Node * ast_node, z3::context&);
        void score_constraints(z3::context &c, Node *astNode);


        z3::expr valuation(z3::context &c, Node *node, int pos);
        z3::expr valuation_until(z3::context &c, Node *node, int pos, int offset);
        z3::expr valuation_G(z3::context &c, Node *node, int pos);
        z3::expr valuation_F(z3::context &c, Node *node, int pos);

        z3::expr node_constraint;
        z3::expr leaf_constraint;
        std::vector<z3::expr> score_constraint;

};