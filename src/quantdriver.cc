#include "quantdriver.hh"
#include "parser.hh"

// initialize static variables as needed
int QuantDriver::ast_size = 0;

QuantDriver::QuantDriver(const std::fstream &p_source, 
                         const std::fstream &n_source, 
                         const std::string formula)
        : max_depth(Configuration::max_depth){
    // parse traces
    if(!parse_traces(p_source, n_source)){
        QuantDriver::error_flag = FILE_PARSE_FAIL;
        return;
    }   
    
    // parse formula
    QuantDriver::ast_size = 0;
    this->ast = new Node();
    construct_ast(this->ast, this->max_depth);
    if(!parse_formula(formula)){
        QuantDriver::error_flag = FORM_PARSE_FAIL;
        return;
    }
    IFVERBOSE(std::cerr << "\nTree after parsing : " << this->ast;)

    this->run();

    QuantDriver::error_flag = OK;
}

QuantDriver::QuantDriver(std::vector<Trace> &traces, Node* ast)
        : max_depth(Configuration::max_depth){
    
    this->traces = traces;

    this->ast = ast;

    QuantDriver::error_flag = OK;
}

QuantDriver::~QuantDriver(){

    // delete trees
    this->ast->destroy_children();
    delete ast;

    // delete traces
}

bool QuantDriver::parse_traces(const std::fstream &p_source, const std::fstream &n_source){
    std::ostringstream ps, ns;
    ps << p_source.rdbuf(); // reading data
    ns << n_source.rdbuf(); 
    std::string str[] = {ns.str(), ps.str()};

    // clean whitespace
    for(auto &s : str){
        s.erase(std::remove_if(s.begin(), s.end(), isspace), s.end());
    }

    // hold futures for trace computations
    std::vector<std::future<void> > async_trace_comp;

    for(auto j : {negative, positive}){
        // compute both positive and negative traces
        this->traces.emplace_back();
        Trace *curr_trace = &this->traces.back();
        curr_trace->trace_string.emplace_back();

        for(auto m : this->traces.front().prop_inst)
                    curr_trace->prop_inst.insert({m.first, Trace::proposition(m.first)});

        std::string temp_prop = __empty;
        std::string temp_step = __empty;
        int curr_step = 0;

        for(int i = 0; i < str[j].length(); i++){
            if(str[j][i] == PROP_DELIMITER 
                    || str[j][i] == STEP_DELIMITER 
                    || str[j][i] == TRACE_DELIMITER){
                // check prop
                if((curr_trace->prop_inst.find(temp_prop) == curr_trace->prop_inst.end()) && temp_prop != __empty){
                    // add new prop to EVERY trace
                    // if it's not in this trace, it's not in any trace
                    for(auto &t : this->traces){
                        t.prop_inst.insert({temp_prop, Trace::proposition(temp_prop)});
                    }
                }

                // add new instance of prop
                curr_trace->prop_inst[temp_prop].instances.emplace_back(curr_step);
            
                // add prop to trace
                temp_step += temp_prop;
                temp_prop = __empty;
                temp_step += str[j][i];

                if(str[j][i] == PROP_DELIMITER) continue;

            }
            if(str[j][i] == STEP_DELIMITER 
                    || str[j][i] == TRACE_DELIMITER){
                curr_trace->trace_string.back().emplace_back(temp_step);
                temp_step = __empty;
                curr_step++;
                curr_trace->length++;
                if(str[j][i] == STEP_DELIMITER) continue;
            }
            if(str[j][i] == TRACE_DELIMITER){
                IFVERBOSE(std::cerr << "\nFinished parsing trace " << curr_trace->id << " of length " << curr_trace->length;)
                curr_trace->parity = j;

                if(i != (str[j].length() - 1)){
                    this->traces.emplace_back();
                    curr_trace = & this->traces.back();
                    curr_step = 0;
                    curr_trace->trace_string.emplace_back();
                    // add all old props to new trace
                    for(auto m : this->traces.front().prop_inst)
                        curr_trace->prop_inst.insert({m.first, Trace::proposition(m.first)});
                }

                continue;
            }

            // not a delimiter, move on
            temp_prop += str[j][i];

        }
    }

    
    IFVERBOSE(std::cerr << "\nPerforming trace computations.";)
    // let every trace individually compute while we go on
    // do NOT use curr_trace for async, since it'll be modified 
    // this breaks things in unimaginable ways
    // trace computations moved to the end to allow all propositions to be collected.
    for(auto &t : this->traces){
        async_trace_comp.emplace_back(
                std::async(std::launch::async, &Trace::compute_optimizations, &t));
    }
    
    // make sure all trace computations finished
    for(auto &comp : async_trace_comp){
        comp.get();
    }

    IFVERBOSE(std::cerr << "\nFinished trace computations.";)

    return true;
}

bool QuantDriver::parse_formula(const std::string formula){
    // initialize parser 
    auto f (std::begin(formula)), l (std::end(formula));
    Parser::parse_into_ast(ast, f, l);

    return true;
}

bool is_prop_exist(std::vector<std::string> &prop_vec, std::string elem){
    for(auto &itr : prop_vec){
        if(itr == elem){
            return true;
        }
    }
    return false;
}

bool is_empty_prop(std::string elem){
    if(elem == ""){
        return true;
    }
    else if(elem[0] == '~'){
        if(elem[1] == '\0'){
            return true;
        }
    }
    return false;
}

std::vector<std::string> get_all_prop_vars(std::vector<Trace> &traces){
    std::vector<std::string> prop_vec;
    for(auto &tr : traces){
        for(auto &itr : tr.prop_inst){
            std::string elem = itr.first;
            if(!is_prop_exist(prop_vec,elem) && !is_empty_prop(elem)){
                prop_vec.push_back(elem);
            }
        }
    }
    return prop_vec;
}

void get_pattern_vars(Node *ast, std::vector<std::string> &var_vec, std::vector<std::string> &prop_vec){
    if(ast != NULL){
        ltl_op ast_label = ast->label;
        std::string prop_var = ast->prop_label;
        if (prop_var != ""){
            if(!is_prop_exist(prop_vec,prop_var)){
                var_vec.push_back(prop_var);
                ast->isPatVar = true;
            }
        }
        else if (ast_label != Empty){
            get_pattern_vars(ast->left, var_vec, prop_vec);
            get_pattern_vars(ast->right, var_vec, prop_vec);
        }
    }
}





void QuantDriver::run(){
    // if top level unknown, parallelize
    
    if(this->ast->label == Subformula){
        //this->consys.is_sub_formula=true;
        //this->run_parallel();
        //return;
    }

    z3::optimize opt(this->opt_context);
    z3::params opt_params(this->opt_context);
    opt_params.set("priority", this->opt_context.str_symbol("pareto"));
    opt.set(opt_params);

    std::vector<std::string> prop_vec = get_all_prop_vars(this->traces);
    this->consys.prop_var_vec = prop_vec;
    std::vector<std::string> patterrn_vars;
    get_pattern_vars(this->ast, patterrn_vars, prop_vec);
    for(auto &it : patterrn_vars){
        std::cout << "\nPattern var is: " << it <<"\n";
    }
    bool b = true;
    z3::expr sum_expr = this->opt_context.real_val("0.0");
    for(auto &tr : this -> traces){
        if(tr.parity == parity_t::positive){
        consys.init_score(this->opt_context, this->ast, tr);
        if(b){
            opt.add(this->consys.node_constraints(opt_context, this->ast, tr));
            opt.add(consys.pat_to_prop_map(this->opt_context, patterrn_vars, tr));
            b = false;
        }
        opt.add(consys.score_constraints_pattern(this->opt_context,this->ast, tr));
        z3::expr zero_expr = this->opt_context.real_val("0.0");
        opt.add(tr.score[this->ast->id][0] > zero_expr);
        sum_expr = sum_expr + tr.score[this->ast->id][0];
        //opt.maximize(tr.score[this->ast->id][0]);
        //break;
        } // TODO what about negative traces?      
    }
    opt.maximize(sum_expr);

    // optimize
    if(opt.check() == z3::sat){
        // parse result into ast

        // write result
        z3::model modl = opt.get_model();
        std::string formula_str = consys.construct_formula(modl, this->ast);
        std::cout << "\nObtained optimal formula: " << formula_str;
        //std::cout << "\nObtained optimal formula: " << this->ast;
    }
    else{ // unsat
        Configuration::throw_error("No satisfying assignment found. Consider changing your parameters.");
    }
    

}

void QuantDriver::run_parallel(){
    std::vector<Node*> par_ast;
    std::vector<QuantDriver*> par_driver;
    std::vector<std::thread*> par_threads;

    for(int i = 1; i < Proposition; i++){ // swap in every operator
    // start with 1 to ignore Empty label
        par_ast.emplace_back(new Node());
        par_ast.back()->label = (ltl_op) i;
        this->construct_ast(par_ast.back(), this->max_depth);

        if(op_arity((ltl_op) i) == 1){
            par_ast[i-1]->left->label = Subformula;
            par_ast[i-1]->left->subformula_size = this->ast->subformula_size - 1;
        }
        else if(op_arity((ltl_op) i) == 2){
            par_ast[i-1]->left->label = Subformula;
            par_ast[i-1]->left->subformula_size = this->ast->subformula_size - 1;
            par_ast[i-1]->right->label = Subformula;
            par_ast[i-1]->right->subformula_size = this->ast->subformula_size - 1;
        }
        else{
            Configuration::throw_error("Non 1/2-ary operator");
        }

        //par_driver.emplace_back(this->traces, par_ast[i-1]);
        //par_threads.emplace_back([&](){par_driver[i-1]->run();});
        QuantDriver *temp_driver = new QuantDriver(this->traces, par_ast[i-1]);
        std::thread *temp_thread = new std::thread(&QuantDriver::run, temp_driver);//par_driver[i-1]);
        par_driver.push_back(temp_driver);
        par_threads.push_back(temp_thread);
        

    }

    for(auto th : par_threads){ // wait for all threads to stop
        th->join();
    }

    // compute max of all results
    auto driver_iterator = std::distance(par_driver.begin(), 
                            std::max_element(par_driver.begin(), par_driver.end(), 
                                [](const QuantDriver *a, const QuantDriver *b){
                                    return (bool) (a->result.value < b->result.value);
                                }));
                        
    this->result = par_driver[driver_iterator]->result;

    // clean up
    par_threads.clear();
    par_driver.clear();
    par_ast.clear();
}

void QuantDriver::construct_ast(Node* ast, int depth){
    if(depth == 0){
        return;
    }
    
    ast->left = new Node();
    ast->right = new Node();

    construct_ast(ast->left, depth - 1);
    construct_ast(ast->right, depth - 1);
}


/*
void Trace::construct_bit_matrices(z3::context &c, const int ast_size){
        // construct operator matrix
        for(int i = 1; i <= Proposition; i++){
            x.emplace_back();
            for(int j = 0; j < ast_size; j++){
                std::string name = "{X(" + std::to_string(this->id)
                                     + "," + std::to_string(i) + "," + std::to_string(j) + ")}";
                x.back().push_back(c.bool_const(name.c_str()));
            }
        }

        // construct proposition matrix
        for(int i = 0; i < this->prop_inst.size(); i++){
            xp.emplace_back();
            for(int j = 0; j < ast_size; j++){
                std::string name = "{XP(" + std::to_string(this->id)
                                     + "," + std::to_string(i) + "," + std::to_string(j) + ")}"; 
                xp.back().push_back(c.bool_const(name.c_str()));
            }
        }
}*/
