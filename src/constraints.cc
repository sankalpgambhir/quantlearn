#include "quantdriver.hh"
#include <numeric>
#include <iterator>


void construct_bit_matrices1(z3::context& c, Node * ast_node, Trace &trace);//Consider only one from line number 100,101
void merged_x_xp(Node *ast_node);
std::map<int,std::vector<z3::expr>> x; // taking map of node_id and vector of expressions of that node
std::map<int,std::vector<z3::expr>> xp;
std::map<int,std::vector<z3::expr>> merged_xxp;
std::vector<std::vector<z3::expr>> leaf_constr;
std::map<std::string, std::vector<z3::expr>> map_constr;





z3::expr do_and(z3::expr e1,z3::expr e2){
    return e1 && e2;
}

z3::expr do_or(z3::expr e1,z3::expr e2){
    return e1 || e2;
}

z3::expr true_expr(z3::context &c){
    std::string temp = "trueStr";
    z3::expr x = c.real_const(temp.c_str());
    z3::expr true_expr = x==x;
    return true_expr;
}

bool Trace:: isPropExistAtPos(int pos, std::string prop_name){
    for(auto &itr : ((((this->prop_inst).find(prop_name))->second).instances)){ //remove loop if possible
        if (itr.position == pos){
            return true;
        }
        return false;   
    }

    return false;
}

void construct_bit_matrices1(z3::context &c, Node *ast_node,Trace &trace){
    std::vector<z3::expr> x_constr;
    std::vector<z3::expr> xp_constr;
    if(ast_node != NULL){
        for(int j=0;j<Proposition;j++){
            std::string name = "{X(" + std::to_string(ast_node->id) + "," + std::to_string(j) + ")}";
            x_constr.push_back(c.bool_const(name.c_str()));
        }
	

        for(int j=0;j < trace.prop_inst.size();j++){
            std::string name = "{XP(" + std::to_string(ast_node->id) + "," + std::to_string(j) + ")}"; 
            xp_constr.push_back(c.bool_const(name.c_str()));
        }
        x[ast_node->id] = x_constr;
        xp[ast_node->id] = xp_constr;
        if(ast_node->left != NULL || ast_node->right != NULL){
            construct_bit_matrices1(c,ast_node->left, trace);
            construct_bit_matrices1(c,ast_node->right, trace);
        }
        else{
            leaf_constr.push_back(xp_constr);
        }
    }
}

z3::expr ConstraintSystem::valuation(z3::context &c, 
                                        Node *node, 
                                        Trace &t, 
                                        const int pos){

    IFVERBOSE(std::cerr << "\nValuating " << node << " on trace " << t.id;)

    switch (node->label)
    {
    case ltl_op::Proposition:
        assert(node->prop_label != __empty);

        if(t.isPropExistAtPos(pos, node->prop_label)){
            return c.real_val("1.0");
        }
        return c.real_val("0.0");

    case ltl_op::Not:
        return valuation_not(c, node, t, pos);

    case ltl_op::Or:
        return (t.score[node->left->id][pos] + t.score[node->right->id][pos]) / 2;
        //return (this->valuation(c, node->left, t, pos) + this->valuation(c, node->right, t, pos)) / 2;

    case ltl_op::And:
        return (t.score[node->left->id][pos] * t.score[node->right->id][pos]);

    case ltl_op::Globally:
        return this->valuation_G(c, node, t, pos);

    case ltl_op::Finally:
        return this->valuation_F(c, node, t, pos);

    case ltl_op::Until:
        return this->valuation_until(c, node, t, pos);

    case ltl_op::Next:
        if(pos == t.length - 1){
            assert(0 && "Decide for end");
        }
        return this->valuation(c, node, t, pos + 1);
    
    default:
        Configuration::throw_error("Invalid formula for valuation");
        break;
    }

    assert(0 && "Invalid formula for valuation");

    return c.real_val("0.0");//Will change
}

z3::expr ConstraintSystem::valuation_until(z3::context &c, 
                                            Node *node, 
                                            Trace &t,
                                            const int pos, 
                                            const int offset){
    if(pos == t.length-1)
        return c.real_val("0.0");
    else{
        z3::expr l_val = t.score[node->left->id][pos];
        z3::expr next_val = this->valuation_until(c, node, t, pos+1, offset+1);
        float rtf = retarder(offset);
        z3::expr value = z3::ite(l_val > 0, z3::ite(next_val == 0.0,c.real_val("0.0"), (rtf*l_val) + next_val) , c.real_val("0.0"));
        z3::expr r_val = t.score[node->right->id][pos];
        return z3::ite(r_val > 0, r_val, value);

    }
}

z3::expr ConstraintSystem::valuation_not(z3::context &c,
                                            Node *node,
                                            Trace &t,
                                            const int pos){
    z3::expr lVal = (1.0 - t.score[node->left->id][pos]);
    return z3::ite(lVal > 0, lVal, c.real_val("0.0"));
}

z3::expr ConstraintSystem::valuation_G(z3::context &c, Node *node, Trace &t, const int pos){
    Node *leftNode = node->left;
    // if(leftNode->label == Proposition){
    //     //if Gp types of formula
    //     std::string prop_name = leftNode->prop_label;
    //     for(auto &itr : t.prop_inst[prop_name].instances){ //remove loop if possible
    //         if (itr.position == pos){
    //             if(itr.num_after == (t.length - 1 - pos)){
    //                 return c.real_val("1.0");
    //             }
    //             else{
    //                 return c.real_val("0.0");
    //             }
    //         }
    //     }
    //     return c.real_val("0.0");
    // }
    // else{
        z3::expr res = c.real_val("0.0");
        z3::expr mult = c.real_val("1.0");

        for(int i = pos; i < t.length; i++){
            z3::expr l_val = t.score[leftNode->id][i];
            float retard = retarder(i-pos);
            z3::expr pos_expr = ite(l_val > 0, retard * l_val, c.real_val("0.0"));
            mult = ite(l_val <= 0, c.real_val("0.0"), mult);
            res = (res + pos_expr) * mult;
        }
        return res;
    //}
}

z3::expr ConstraintSystem::valuation_F(z3::context &c, 
                                        Node *node, 
                                        Trace &t,
                                        const int pos){
    Node *leftNode = node->left;
    // if(leftNode->label == Proposition){
    //     //Fp types of formula
    //     std::string prop_name = leftNode->prop_label;
    //     for(auto &itr : t.prop_inst[prop_name].instances){ //remove loop if possible
    //         if (itr.position == pos){
    //             if(itr.pos_next > 0){ //Assume default value of pos_next is negative
    //                 return c.real_val("1.0");
    //             }
    //             else{
    //                 return c.real_val("0.0");
    //             }
    //         }
    //     }
    //     return c.real_val("0.0");
    // }
    //else{
        z3::expr res = c.real_val("0.0");
        int count = 0;
        for(int i = pos; i < t.length; i++){
            z3::expr l_val = t.score[leftNode->id][i];
            float retard = retarder(i-pos);
            z3::expr pos_expr = ite(l_val > 0, retard*l_val, c.real_val("0.0"));
            res = res + pos_expr;
            count++;
        }
        return res/((float) count);
    //}      
}

/*
z3::expr test_check(z3::context &c){
    std::string str = "test1";
    z3::expr test_expr = c.real_const(str.c_str());
    return test_expr;
}
*/

z3::expr at_most_one(z3::context& c, std::vector<z3::expr> &vec_expr){

    z3::expr and_constr = true_expr(c);
    for(int i=0;i<vec_expr.size()-1;i++){
        for(int j=i+1;j<vec_expr.size();j++){
            z3::expr e = !(vec_expr[i] && vec_expr[j]);
            and_constr = and_constr && e;
        }
    }
    return and_constr;
}

z3::expr ConstraintSystem::pat_to_prop_map(z3::context &c, std::vector<std::string> &pat_vars, Trace &t){
    z3::expr pat_match_constr = true_expr(c);
    for(auto &itr1 : pat_vars){
        std::vector<z3::expr> expr_vec;
        for(auto &itr2 : t.prop_inst){
            std::string con_str = "p_"+itr1+","+itr2.first;
            z3::expr expr_constr = c.bool_const(con_str.c_str());
            expr_vec.push_back(expr_constr);
        }
        map_constr[itr1] = expr_vec;
        z3::expr or_of_vec = std::accumulate(expr_vec.begin(), expr_vec.end(), !(true_expr(c)), do_or);
        z3::expr at_most = at_most_one(c,expr_vec);
        pat_match_constr = pat_match_constr && or_of_vec && at_most;
    }

    return pat_match_constr;
}

z3::expr ConstraintSystem::score_constraints_pattern(z3::context &c, Node *astNode, Trace &t){
    std::vector<z3::expr> score_constr_pattern;
    z3::expr and_score_constr = true_expr(c);
    if(astNode != NULL){
        if(astNode->label != Empty){ 
            for(int j = 0; j < t.length; j++){
                // std::string score_str = "score_"+std::to_string(astNode->id)+","+std::to_string(j);              
                // z3::expr ex_temp = c.real_const(score_str.c_str());
                // t.score[astNode->id].push_back(ex_temp);
                if(astNode->label != Proposition){
                    z3::expr constr = (t.score[astNode->id][j]== valuation(c, astNode, t, j));
                    score_constr_pattern.push_back(constr);
                }
                else{
                    std::string prop_var = astNode->prop_label;
                    std::vector<z3::expr> expr_vec = map_constr[prop_var];
                    int k=0;
                    for(auto &itr : t.prop_inst){
                        z3::expr ant = expr_vec[k];
                        Node * mod_ast = new Node(ltl_op::Proposition,astNode->left,astNode->right);
                        mod_ast->prop_label = itr.first;
                        mod_ast->id = astNode->id;
                        z3:: expr con = (t.score[astNode->id][j]==valuation(c, mod_ast, t, j));
                        z3:: expr cons = z3::implies(ant,con);
                        score_constr_pattern.push_back(cons);
                        k++;
                    }
                }
            }
            z3::expr l_score_constr = this->score_constraints_pattern(c, astNode->left, t);
            z3::expr r_score_constr = this->score_constraints_pattern(c, astNode->right, t);
    
            and_score_constr = std::accumulate(score_constr_pattern.begin(), score_constr_pattern.end(), true_expr(c), do_and);
            and_score_constr = and_score_constr && l_score_constr && r_score_constr;
        }

    }
    return and_score_constr;
}





z3::expr ConstraintSystem::score_constraints(z3::context &c, Node *astNode, Trace &t){
    std::vector<z3::expr> score_constr;
    z3::expr and_score_constr = true_expr(c);
    if(astNode != NULL){
        for(int j = 0; j < t.length; j++){
            // std::string score_str = "score_"+std::to_string(astNode->id)+","+std::to_string(j);              
            // z3::expr ex_temp = c.real_const(score_str.c_str());
            // t.score[astNode->id].push_back(ex_temp);
            for(int k=Not;k<Proposition;k++){
                std::vector<z3::expr> x_vec = x[astNode->id];
                z3:: expr ant = x_vec[k];
                Node * mod_ast = new Node((ltl_op)k,astNode->left,astNode->right);
                printf("\nCheck..\n");
                z3:: expr con = (t.score[astNode->id][j]== valuation(c, mod_ast, t, j));
                z3:: expr cons = z3::implies(ant,con);
                score_constr.push_back(cons);
            }
            int k=0;
            for(auto &itr : t.prop_inst){
                std::vector<z3::expr> x_vec = xp[astNode->id];
                z3:: expr ant = x_vec[k];
                Node * mod_ast = new Node(ltl_op::Proposition,astNode->left,astNode->right);
                mod_ast->prop_label = itr.first;
                z3:: expr con = (t.score[astNode->id][j]==valuation(c, astNode, t, j));
                z3:: expr cons = z3::implies(ant,con);
                score_constr.push_back(cons);
                k++;
            }
        }
        z3::expr l_score_constr = this->score_constraints(c, astNode->left, t);
        z3::expr r_score_constr = this->score_constraints(c, astNode->right, t);
    
        and_score_constr = std::accumulate(score_constr.begin(), score_constr.end(), true_expr(c), do_and);
        and_score_constr = and_score_constr && l_score_constr && r_score_constr;
    }
    return and_score_constr;
}

z3::expr ConstraintSystem::node_constraints(z3::context& c, Node * ast_node,Trace &trace){

    z3::expr node_constr = true_expr(c);
    construct_bit_matrices1(c, ast_node, trace);
    merged_x_xp(ast_node);
    for(auto itr=merged_xxp.begin();itr != merged_xxp.end();itr++){
        std::vector<z3::expr> vec_exp = itr->second;
        z3::expr or_of_vec = std::accumulate(vec_exp.begin(), vec_exp.end(), !(true_expr(c)), do_or);
        z3::expr at_most = at_most_one(c,vec_exp);
        node_constr = node_constr && or_of_vec && at_most;
    }

    return node_constr;
}

z3::expr ConstraintSystem::leaf_constraints(z3::context& c){

    z3::expr leaf_cons = true_expr(c);
    for(auto &it : leaf_constr){
        z3::expr or_of_vec = std::accumulate(it.begin(), it.end(), !(true_expr(c)), do_or);
        leaf_cons = leaf_cons && or_of_vec;
    }

    return leaf_cons;
}

void merged_x_xp(Node * ast_node){
    if(ast_node != NULL){
        std::vector<z3::expr> vec1 = x[ast_node->id];
        std::vector<z3::expr> vec2 = xp[ast_node->id];
        std::vector<z3::expr> vec3;
        vec3.insert(vec3.end(), vec1.begin(), vec1.end());
        vec3.insert(vec3.end(), vec2.begin(), vec2.end());
        merged_xxp[ast_node->id] = vec3;
        merged_x_xp(ast_node->left);
        merged_x_xp(ast_node->right);
    }
}

z3::expr ConstraintSystem::and_score_constraints(z3::context &c){

    z3::expr score_con = std::accumulate(this->score_constraint.begin(), this->score_constraint.end(), true_expr(c), do_and);
    //z3::expr final_constr = this->node_constraints(c, ast_node, trace) &&
    //                                         this->leaf_constraints(c) && 
    //                                                        score_con;

    return score_con;
}

void ConstraintSystem::init_score(z3::context &c, Node *ast, Trace &t){
    if(ast != NULL){
        if(ast->label != Empty){
            for(int i=0; i<t.length; i++){
                std::string score_str = "score_"+std::to_string(ast->id)+","+std::to_string(i);              
                z3::expr ex_temp = c.real_const(score_str.c_str());
                t.score[ast->id].push_back(ex_temp);
            }
            this->init_score(c,ast->left,t);
            this->init_score(c,ast->right,t);
        }
    }
}

void print_model(z3::model &m){
    printf("\nInside function\n");
    for(unsigned i=0;i<m.size();i++){
        z3::func_decl v = m[i];
        assert(v.arity() == 0);
        std::cout << v.name() << " = " << m.get_const_interp(v) << "\n";
    }
}

std::string get_mapped_prop_str(z3::model modl, std::string pat_var){
    std::vector<z3::expr> expr_vec = map_constr[pat_var];
    for(auto &itr : expr_vec){
        std::string eval_var = modl.eval(itr).decl().name().str();
        std::cout<<"maped variable: "<<itr.decl().name().str() << " : "<<eval_var<<"\n";     
        if(eval_var == "true"){
            std::string expr_str = itr.decl().name().str();
            //std::cout<<"Expr is: "<<expr_str<<"\n";
            int str_size = expr_str.size();
            int pos = expr_str.find(",");
            if(pos != -1 && pos < str_size){
                return expr_str.substr(pos+1,str_size);
            }
        }
    }
    return "";
}

std::string ConstraintSystem::construct_formula(z3::model& modl, Node* ast){
    print_model(modl);
    if(ast->label != Subformula){
        if(ast->label != Empty){
            switch(op_arity(ast->label)){
                case 0:{ 
                    if(ast->label == Proposition){
                        if(ast->isPatVar){
                            std::string prop_var = get_mapped_prop_str(modl, ast->prop_label);
                            return prop_var;
                        }
                        else{
                            return ast->prop_label;
                        }    
                    }
                    else if(ast->label == Subformula){

                    }
                    else{
                        assert(0 && "Tried to print empty node");
                    }
                }
                case 1:{
                    std::string left_formula = this->construct_formula(modl,ast->left);
                    std::string node_op = "";
                    switch (ast->label){
                        case Not: node_op = "!"; break;
                        case Next: node_op = "X"; break;
                        case Globally: node_op = "G"; break;
                        case Finally: node_op = "F"; break;
                        default: assert(0 && "Tried to print invalid operator");
                    }
                    return node_op+"("+left_formula+")";
                } 
                case 2:{ 
                    std::string left_formula = this->construct_formula(modl,ast->left);
                    std::string right_formula = this->construct_formula(modl,ast->right);
                    std::string node_op = "";
                    switch (ast->label){
                        case Or: node_op = "+"; break;
                        case And: node_op = "."; break;
                        case Until: node_op = "U"; break;
                        default: assert(0 && "Tried to print invalid operator");
                    }
                    return "("+left_formula+")"+node_op+"("+right_formula+")";
                }
                default: assert(0 && "Tried to print invalid formula");
                    
            }
        }
    }
    return "";
}


