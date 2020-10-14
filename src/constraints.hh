#include <z3++.h>
#include "node.hh"
#include "trace.hh"

class ConstraintSystem{
    public:

    private:
        z3::expr all_constraints(Node * ast_node, z3::context&,Trace &trace);
        z3::expr score_constraints(z3::context &c, Node *astNode, Trace &trace);
        void node_constraints(z3::context& c, Node * ast_node, Trace &trace);
        void leaf_constraints(z3::context& c);


        z3::expr valuation(z3::context &c, Node *node, Trace &t, const int pos = 0);
        z3::expr valuation_until(z3::context &c, Node *node, Trace &t, const int pos = 0, const int offset = 0);
        z3::expr valuation_G(z3::context &c, Node *node, Trace &t, const int pos = 0);
        z3::expr valuation_F(z3::context &c, Node *node, Trace &t, const int pos = 0);

        z3::expr node_constraint;
        z3::expr leaf_constraint;
        std::vector<z3::expr> score_constraint;

};