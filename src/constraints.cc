#include "quantdriver.hh"
#include <numeric>
#include <iterator>


//Consider only one from line number 100,101
void merged_x_xp(Node *ast_node);
std::map<int,std::vector<z3::expr>> x; // taking map of node_id and vector of expressions of that node
std::map<int,std::vector<z3::expr>> xp;
std::map<int,std::vector<z3::expr>> merged_xxp;
std::vector<std::vector<z3::expr>> leaf_constr;
std::map<std::string, std::vector<z3::expr>> map_constr;
std::vector<std::string> prop_vars_vec;





z3::expr do_and(z3::expr e1,z3::expr e2){
    return e1 && e2;
}

z3::expr do_or(z3::expr e1,z3::expr e2){
    return e1 || e2;
}



z3::expr true_expr(z3::context &c){
    return c.bool_val(true);
    // std::string temp = "trueStr";
    // z3::expr x = c.real_const(temp.c_str());
    // z3::expr true_expr = x==x;
    // return true_expr;
}

z3::expr and_vec(z3::context &c, std::vector<z3::expr> &vec){
    if(vec.size() == 0){
        return true_expr(c);
    }
    else if(vec.size() == 1){
        return vec.back();
    }
    else{
        z3::expr prev_expr = vec.front();
        for(int i=1; i<vec.size(); i++){
            z3::expr next_expr = vec[i];
            prev_expr = prev_expr && next_expr;
        }
        return prev_expr;
    }
}

z3::expr or_vec(z3::context &c, std::vector<z3::expr> &vec){
    if(vec.size() == 0){
        return true_expr(c);
    }
    else if(vec.size() == 1){
        return vec.back();
    }
    else{
        z3::expr prev_expr = vec.front();
        for(int i=1; i<vec.size(); i++){
            z3::expr next_expr = vec[i];
            prev_expr = prev_expr || next_expr;
        }
        return prev_expr;
    }
}




bool Trace:: isPropExistAtPos(int pos, std::string prop_name){
    for(auto &itr : ((((this->prop_inst).find(prop_name))->second).instances)){ //remove loop if possible
        if (itr.position == pos){
            return true;
        }
        //return false;   
    }

    return false;
}

bool is_leaf_node(Node *ast_node){
    if(ast_node != NULL){
        if(ast_node->left == NULL || ast_node->label == Proposition){
            return true;
        }
        if(ast_node->label == Subformula){
            return (ast_node->subformula_size == 0);
        }
        return false;
    }
    return false;
}



z3::expr ConstraintSystem::valuation(z3::context &c, 
                                        Node *node, 
                                        Trace &t, 
                                        const int pos){
    //IFVERBOSE(std::cerr << "\nValuating " << node << " on trace " << t.id;)
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

#if GF_FRAGMENT
    // nothing
#else
    case ltl_op::Until:
        return this->valuation_until(c, node, t, pos);

    case ltl_op::Next:
        if(pos == t.length - 1){
            return c.real_val("0.0");  //assert(0 && "Decide for end");
        }
        return t.score[node->left->id][pos+1];
        //return this->valuation(c, node, t, pos + 1);
#endif
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
       
    z3::expr r_val = t.score[node->right->id][pos];
    if(pos == t.length-1){
        return r_val;
    }
    else{
        z3::expr next_val = t.score[node->id][pos+1];
        z3::expr l_val = t.score[node->left->id][pos];
        z3::expr l_next_true = (next_val > c.real_val("0.0")) && (l_val > c.real_val("0.0"));
        float rtf = retarder(pos);
        std::string rtf_str = std::to_string(rtf);
        //z3::expr value = ite(l_next_true, (c.real_val(rtf_str.c_str())*l_val) + next_val, c.real_val("0.0"));
        z3::expr value = ite(l_next_true, l_val + next_val, c.real_val("0.0"));
        return z3::ite(r_val > 0, r_val, value);
    }
    /*
    if(pos == t.length-1)
        return c.real_val("0.0");
    else{
        z3::expr l_val = t.score[node->left->id][pos];
        z3::expr next_val = this->valuation_until(c, node, t, pos+1, offset+1);
        float rtf = retarder(offset);
        std::string rtf_str = std::to_string(rtf);
        z3::expr value = z3::ite(l_val > 0, z3::ite(next_val == c.real_val("0.0"),c.real_val("0.0"), (c.real_val(rtf_str.c_str())*l_val) + next_val) , c.real_val("0.0"));
        z3::expr r_val = t.score[node->right->id][pos];
        return z3::ite(r_val > 0, r_val, value);

    }
    */
}

z3::expr ConstraintSystem::valuation_not(z3::context &c,
                                            Node *node,
                                            Trace &t,
                                            const int pos){
    z3::expr lVal = (c.real_val("1.0") - t.score[node->left->id][pos]);
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
        z3::expr l_val = t.score[leftNode->id][pos];
        if(pos == t.length-1){
            return l_val;
        }
        else if(pos < t.length-1){
            z3::expr next_pos_score = t.score[node->id][pos+1];
            float retard_val = retarder(pos);
            std::string retard_str = std::to_string(retard_val);
            z3::expr zero_expr =  (l_val == c.real_val("0.0")) || (next_pos_score == c.real_val("0.0"));
            //c.real_val(retard_str.c_str())*
            res = ite(zero_expr, c.real_val("0.0"), (l_val+next_pos_score));       
        }
        return res;



        // for(int i = pos; i < t.length; i++){
        //     z3::expr l_val = t.score[leftNode->id][i];
        //     float retard_val = retarder(i-pos);
        //     std::string retard_str = std::to_string(retard_val);
        //     z3::expr pos_expr = ite(l_val > 0, c.real_val(retard_str.c_str())*l_val, c.real_val("0.0"));
        //     mult = ite(l_val <= 0, c.real_val("0.0"), mult);
        //     res = (res + pos_expr) * mult;
        // }
        // return res;
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
        int count = t.length-pos;
        z3::expr l_val = t.score[leftNode->id][pos];
        if(pos == t.length-1){
            return l_val;
        }
        else if(pos < t.length-1){
            z3::expr next_pos_score = t.score[node->id][pos+1];
            float retard_val = retarder(pos);
            std::string retard_str = std::to_string(retard_val);
            z3::expr zero_expr = (l_val == c.real_val("0.0")) && (next_pos_score == c.real_val("0.0"));
            res = ite(zero_expr, c.real_val("0.0"), ite(l_val > next_pos_score, l_val,next_pos_score));
            //res = ite(zero_expr, c.real_val("0.0"), (l_val+next_pos_score));
        }
        std::string count_str = std::to_string((float) count);
        return res;//c.real_val(count_str.c_str());


        // for(int i = pos; i < t.length; i++){
        //     z3::expr l_val = t.score[leftNode->id][i];
        //     float retard_val = retarder(i-pos);
        //     std::string retard_str = std::to_string(retard_val);
        //     //c.real_val(retard_str.c_str())
        //     z3::expr pos_expr = ite(l_val > c.real_val("0.0"), c.real_val(retard_str.c_str())*l_val, c.real_val("0.0"));
        //     res = res + pos_expr;
        //     count++;
        // }
        // std::string count_str = std::to_string((float) count);
        // return res/c.real_val(count_str.c_str());
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
    int vec_size = vec_expr.size();
    for(int i=0; i<vec_size-1;i++){
        for(int j=i+1;j<vec_size;j++){
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
        for(auto &itr2 : this->prop_var_vec){
            std::string con_str = "p_"+itr1+","+itr2;
            IFVERBOSE(std::cout<<"\n"<<con_str<<"\n";)
            z3::expr expr_constr = c.bool_const(con_str.c_str());
            expr_vec.push_back(expr_constr);
        }
        map_constr[itr1] = expr_vec;
        //z3::expr or_of_vec = std::accumulate(expr_vec.begin(), expr_vec.end(), !(true_expr(c)), do_or);
        z3::expr or_of_vec = or_vec(c,expr_vec);
        z3::expr at_most = at_most_one(c,expr_vec);
        pat_match_constr = pat_match_constr && or_of_vec && at_most;
    }

    return pat_match_constr;
}

std::vector<z3::expr> ConstraintSystem::pat_prop_constr_pos(z3::context &c, Node * astNode, Trace &t, int pos){
    std::vector<z3::expr> prop_constr;
    std::string prop_var = astNode->prop_label;
    std::vector<z3::expr> expr_vec = map_constr[prop_var];
    if(astNode->isPatVar){
        int k=0;
        for(auto &itr : this->prop_var_vec){
            z3::expr ant = expr_vec[k];
            Node * mod_ast = new Node(ltl_op::Proposition,astNode->left,astNode->right);
            mod_ast->prop_label = itr;
            mod_ast->id = astNode->id;
            z3:: expr con = (t.score[astNode->id][pos]==valuation(c, mod_ast, t, pos));
            z3:: expr cons = z3::implies(ant,con);
            //std::cout<<"\n"<<cons.decl().name().str()<<"\n";
            prop_constr.push_back(cons);
            k++;
        }
    }
    else{
        z3:: expr con = (t.score[astNode->id][pos]==valuation(c, astNode, t, pos));
        prop_constr.push_back(con);
    }
    
    return prop_constr;
}
std::vector<z3::expr> ConstraintSystem::subformula_constr_pos(z3::context &c, Node * astNode, Trace &t, int pos){
    std::vector<z3::expr> subformula_constr;
    std::vector<z3::expr> x_vec = x[astNode->id];
    for(int k=1;k<Proposition;k++){
        z3:: expr ant = x_vec[k-1];
        Node * mod_ast = new Node((ltl_op)k,astNode->left,astNode->right);
        mod_ast->id = astNode->id;
        z3:: expr con = (t.score[astNode->id][pos]== valuation(c, mod_ast, t, pos));
        z3:: expr cons = z3::implies(ant,con);
        subformula_constr.push_back(cons);
    }
    return subformula_constr;
}

std::vector<z3::expr> ConstraintSystem::prop_constr_pos(z3::context &c, Node * astNode, Trace &t, int pos){
    std::vector<z3::expr> subformula_constr;
    int k=0;
    for(auto &itr : this->prop_var_vec){
        std::vector<z3::expr> x_vec = xp[astNode->id];
        z3:: expr ant = x_vec[k];
        Node * mod_ast = new Node(ltl_op::Proposition,astNode->left,astNode->right);
        mod_ast->prop_label = itr;
        mod_ast->id = astNode->id;
        z3:: expr con = (t.score[astNode->id][pos]==valuation(c, mod_ast, t, pos));
        z3:: expr cons = z3::implies(ant,con);
        subformula_constr.push_back(cons);
        k++;
    }
    return subformula_constr;
}
/*
z3::expr ConstraintSystem::score_constraints_pattern(z3::context &c, Node *astNode, Trace &t){
    std::vector<z3::expr> score_constr;
    z3::expr and_score_constr = true_expr(c);
    z3::expr l_score_constr = true_expr(c);
    z3::expr r_score_constr = true_expr(c);
    if(astNode != NULL){
        for(int j = 0; j < t.length; j++){
            if(astNode->label == Empty){
                return and_score_constr;
            }
            else if(astNode->label == Proposition){
                std::vector<z3::expr> prop_constr_vec = this->pat_prop_constr_pos(c,astNode,t,j);
                score_constr.insert(score_constr.end(), prop_constr_vec.begin(), prop_constr_vec.end());
            }
            else if(astNode->label == Subformula){
                if(!is_leaf_node(astNode)){
                    std::vector<z3::expr> subformula_constr_vec = this->subformula_constr_pos(c,astNode, t,j);
                    score_constr.insert(score_constr.end(), subformula_constr_vec.begin(), subformula_constr_vec.end());
                    l_score_constr = this->score_constraints_pattern(c, astNode->left, t);
                    r_score_constr = this->score_constraints_pattern(c, astNode->right, t);
                }
                else{ //leaf node with subformula
                    std::vector<z3::expr> prop_constr_vec = this->prop_constr_pos(c,astNode,t,j);
                    score_constr.insert(score_constr.end(), prop_constr_vec.begin(), prop_constr_vec.end());
                   
                }
            }
            else{
                z3::expr constr = (t.score[astNode->id][j]== valuation(c, astNode, t, j));
                score_constr.push_back(constr);
                int arity = op_arity(astNode->label);
                if(arity == 1){
                    l_score_constr = this->score_constraints_pattern(c, astNode->left, t);
                }
                else if(arity == 2){
                    l_score_constr = this->score_constraints_pattern(c, astNode->left, t);
                    r_score_constr = this->score_constraints_pattern(c, astNode->right, t);
                }     
            }
        }   
        //and_score_constr = std::accumulate(score_constr.begin(), score_constr.end(), true_expr(c), do_and);
        and_score_constr = and_vec(c,score_constr);
        and_score_constr = and_score_constr && l_score_constr && r_score_constr;

    }
    return and_score_constr;
}
*/


z3::expr ConstraintSystem::score_constraints_pattern(z3::context &c, Node *astNode, Trace &t){
    std::vector<z3::expr> score_constr;
    z3::expr and_score_constr = true_expr(c);
    z3::expr l_score_constr = true_expr(c);
    z3::expr r_score_constr = true_expr(c);
    if(astNode != NULL){
        if(astNode->label == Empty){
            return and_score_constr;
        }
        else if(astNode->label == Proposition){
            for(int j = 0; j < t.length; j++){
                std::vector<z3::expr> prop_constr_vec = this->pat_prop_constr_pos(c,astNode,t,j);
                score_constr.insert(score_constr.end(), prop_constr_vec.begin(), prop_constr_vec.end());
            }
        }
        else if(astNode->label == Subformula){
            if(!is_leaf_node(astNode)){
                for(int j = 0; j < t.length; j++){
                    std::vector<z3::expr> subformula_constr_vec = this->subformula_constr_pos(c,astNode, t,j);
                    score_constr.insert(score_constr.end(), subformula_constr_vec.begin(), subformula_constr_vec.end());
                }
                l_score_constr = this->score_constraints_pattern(c, astNode->left, t);
                r_score_constr = this->score_constraints_pattern(c, astNode->right, t);
            }
            else{
                for(int j = 0; j < t.length; j++){
                    std::vector<z3::expr> prop_constr_vec = this->prop_constr_pos(c,astNode,t,j);
                    score_constr.insert(score_constr.end(), prop_constr_vec.begin(), prop_constr_vec.end());
                }
            }
        }
        else{
            for(int j = 0; j < t.length; j++){
                z3::expr constr = (t.score[astNode->id][j]== valuation(c, astNode, t, j));
                score_constr.push_back(constr);
            }
            int arity = op_arity(astNode->label);
            if(arity == 1){
                l_score_constr = this->score_constraints_pattern(c, astNode->left, t);
            }
            else if(arity == 2){
                l_score_constr = this->score_constraints_pattern(c, astNode->left, t);
                r_score_constr = this->score_constraints_pattern(c, astNode->right, t);
            }   
        }   
        //and_score_constr = std::accumulate(score_constr.begin(), score_constr.end(), true_expr(c), do_and);
        and_score_constr = and_vec(c,score_constr);
        and_score_constr = and_score_constr && l_score_constr && r_score_constr;

    }
    return and_score_constr;
}

void ConstraintSystem::construct_bit_matrices1(z3::context &c, Node *ast_node,Trace &trace){
    std::vector<z3::expr> x_constr;
    std::vector<z3::expr> xp_constr;
    if(ast_node->label == Subformula){ //Constrains for subformula
        if(!is_leaf_node(ast_node)){
            for(int j=1;j<Proposition;j++){
                std::string name = "XO" + std::to_string(ast_node->id) + "," + std::to_string(j);
                x_constr.push_back(c.bool_const(name.c_str()));
            }

            // for(auto &itr : trace.prop_inst){
            //     std::string name = "XP" + std::to_string(ast_node->id) + "," + itr.first; 
            //     xp_constr.push_back(c.bool_const(name.c_str()));
            // }
            x[ast_node->id] = x_constr;
            xp[ast_node->id] = xp_constr;
            this->construct_bit_matrices1(c,ast_node->left, trace);
            this->construct_bit_matrices1(c,ast_node->right, trace);            
        }
        else{
            for(auto &itr : this->prop_var_vec){
                std::string name = "XP" + std::to_string(ast_node->id) + "," + itr; 
                xp_constr.push_back(c.bool_const(name.c_str()));
            }
            x[ast_node->id] = x_constr;
            xp[ast_node->id] = xp_constr;
            leaf_constr.push_back(xp_constr);
        }
    }
    else if(ast_node->label != Empty){ //node is already labbelled no need to put constraints
        int arity = op_arity(ast_node->label);
        if(arity == 1){
            this->construct_bit_matrices1(c,ast_node->left, trace);
        }
        else if(arity == 2){
            this->construct_bit_matrices1(c,ast_node->left, trace);
            this->construct_bit_matrices1(c,ast_node->right, trace);
        }
    }
    
}


z3::expr ConstraintSystem::node_constraints(z3::context& c, Node * ast_node,Trace &trace){

    z3::expr node_constr = true_expr(c);
    this->construct_bit_matrices1(c, ast_node, trace);
    merged_x_xp(ast_node);
    for(auto itr=merged_xxp.begin();itr != merged_xxp.end();itr++){
        std::vector<z3::expr> vec_exp = itr->second;
        z3::expr or_of_vec = true_expr(c);
        if(vec_exp.size() > 0){
            //or_of_vec = std::accumulate(vec_exp.begin(), vec_exp.end(), !(true_expr(c)), do_or);
            or_of_vec = or_vec(c, vec_exp);
        }
        z3::expr at_most = at_most_one(c,vec_exp);
        node_constr = node_constr && or_of_vec && at_most;
    }

    return node_constr;
}

z3::expr ConstraintSystem::leaf_constraints(z3::context& c){

    z3::expr leaf_cons = true_expr(c);
    for(auto &it : leaf_constr){
        //z3::expr or_of_vec = std::accumulate(it.begin(), it.end(), !(true_expr(c)), do_or);
        z3::expr or_of_vec = or_vec(c,it);
        leaf_cons = leaf_cons && or_of_vec;
    }

    return leaf_cons;
}

void merged_x_xp(Node * ast_node){
    if(ast_node != NULL){
        if(ast_node->label == Subformula){
            std::vector<z3::expr> vec1 = x[ast_node->id];
            std::vector<z3::expr> vec2 = xp[ast_node->id];
            std::vector<z3::expr> vec3;
            vec3.insert(vec3.end(), vec1.begin(), vec1.end());
            vec3.insert(vec3.end(), vec2.begin(), vec2.end());
            merged_xxp[ast_node->id] = vec3;
            if(!is_leaf_node(ast_node)){
                merged_x_xp(ast_node->left);
                merged_x_xp(ast_node->right);
            }
        }
    }
}

z3::expr ConstraintSystem::and_score_constraints(z3::context &c){

    //z3::expr score_con = std::accumulate(this->score_constraint.begin(), this->score_constraint.end(), true_expr(c), do_and);
    z3::expr score_con = and_vec(c,this->score_constraint);
    //z3::expr final_constr = this->node_constraints(c, ast_node, trace) &&
    //                                         this->leaf_constraints(c) && 
    //                                                        score_con;

    return score_con;
}

void creat_score_vector_at_node(z3::context &c, Node *ast, Trace &t){
    for(int i=0; i<t.length; i++){
        std::string score_str = "score_"+std::to_string(t.id)+","+std::to_string(ast->id)+","+std::to_string(i);              
        z3::expr ex_temp = c.real_const(score_str.c_str());
        t.score[ast->id].push_back(ex_temp);
    }
}

void init_subformula_at_childs(Node *ast, bool is_right_init){
    if(ast->left != NULL){
        if(ast->left->label == Empty){
            ast->left->label = Subformula;
            if(ast->label == Subformula){
                ast->left->subformula_size = (ast->subformula_size) -1;
            }
        }
    }
    if(is_right_init){
        if(ast->right != NULL){
            if(ast->right->label == Empty){
                ast->right->label = Subformula;
                if(ast->label == Subformula){
                    ast->right->subformula_size = (ast->subformula_size) -1;
                }
            }
        }
    }
}

void ConstraintSystem::init_score(z3::context &c, Node *ast, Trace &t){
    if(ast != NULL){
        if(ast->label == Subformula){
            creat_score_vector_at_node(c,ast,t);
            if(!is_leaf_node(ast)){ //non leaf node
                init_subformula_at_childs(ast, true);
                this->init_score(c,ast->left,t);
                this->init_score(c,ast->right,t);
            }
        }
        else if(ast->label != Empty){
            creat_score_vector_at_node(c,ast,t);
            int arity = op_arity(ast->label);
            if(arity == 1){
                init_subformula_at_childs(ast, false); //Incase root node assigned label and all others are empty
                this->init_score(c,ast->left,t);
            }
            else if(arity == 2){
                init_subformula_at_childs(ast, true); //function init subformula at child only if child's label is Empty
                this->init_score(c,ast->left,t);
                this->init_score(c,ast->right,t);
            }
        }
    }
}

void print_model(z3::model &m){
    printf("\nInside function\n");
    for(unsigned i=0;i<m.size();i++){
        z3::func_decl v = m[i];
        assert(v.arity() == 0);
        std::cout<<"Model: " << v.name() << " = " << m.get_const_interp(v) << "\n";
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

std::string ConstraintSystem::construct_formula_if_node_label(z3::model& modl, Node* ast){
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
            else{
                assert(0 && "Tried to print empty node");
            }
        }
        case 1:{
            std::string left_formula = this->construct_formula(modl,ast->left);
            std::string node_op = "";
            switch (ast->label){
                case Not: node_op = "!"; break;
#if GF_FRAGMENT
    // nothing
#else
                case Next: node_op = "X"; break;
#endif
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
#if GF_FRAGMENT
    // nothing
#else
                case Until: node_op = "U"; break;
#endif
                default: assert(0 && "Tried to print invalid operator");
            }
            return "("+left_formula+")"+node_op+"("+right_formula+")";
        }
        default: assert(0 && "Tried to print invalid formula");          
    }
    return "";
}

std::string ConstraintSystem::get_node_val_mapped(z3::model& modl, Node* ast){
    if(ast != NULL){
        std::vector<z3::expr> expr_vec = merged_xxp[ast->id];
        for(auto &itr : expr_vec){
            std::string eval_var = modl.eval(itr).decl().name().str();
            std::cout<<"maped node: "<<itr.decl().name().str() << " : "<<eval_var<<"\n";     
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
    }
    return "";
}

bool is_number(const std::string& s){
    std::string::const_iterator it = s.begin();
    while (it != s.end() && std::isdigit(*it)) ++it;
    return !s.empty() && it == s.end();
}


std::string ConstraintSystem::construct_formula(z3::model& modl, Node* ast){
    IFVERBOSE(print_model(modl);)
    if(ast != NULL){
        if(ast->label == Subformula || ast->label == Empty){
            std::string node_val_mapped = this->get_node_val_mapped(modl,ast);
            std::string mapped_val = "";
            int arity = 0;
            if(is_number(node_val_mapped)){
                int my_int = std::stoi(node_val_mapped);//string to int
                switch(my_int){
                    case Not: mapped_val = "!"; arity = 1; break;
                    case Or: mapped_val = "+"; arity = 2; break;
                    case And: mapped_val = "."; arity = 2; break;
                #if GF_FRAGMENT
                    // nothing
                #else
                    case Next: mapped_val = "X"; arity = 1; break;
                    case Until: mapped_val = "U"; arity = 2; break;
                #endif    
                    case Globally: mapped_val = "G"; arity = 1; break;
                    case Finally: mapped_val = "F"; arity = 1; break;
                    default: assert(0 && "Unrecognized ltl operator");
                }
            }
            else{
                mapped_val = node_val_mapped;
                return mapped_val;
            }

            if(arity == 1){
                std::string left_formula = this->construct_formula(modl,ast->left);
                return mapped_val + "("+left_formula+")";
            }
            else if(arity == 2){
                std::string left_formula = this->construct_formula(modl,ast->left);
                std::string right_formula = this->construct_formula(modl,ast->right);
                return "("+left_formula+")"+mapped_val+"("+right_formula+")";
                
            }    
        }
        else{ //labelled node
            return this->construct_formula_if_node_label(modl, ast);
        }
    }
    return "";
}


