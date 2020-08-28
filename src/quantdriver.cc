#include "quantdriver.hh"

QuantDriver::QuantDriver(const std::fstream* source, const std::string formula)
        : max_depth(MAX_DEPTH_DEFAULT){
    // parse traces
    if(parse_traces(source)){
        this->error_flag = FILE_PARSE_FAIL;
        return;
    }   
    
    // parse formula
    this->ast_size = 0;
    if(parse_formula(formula)){
        this->error_flag = FORM_PARSE_FAIL;
        return;
    }

    this->error_flag = OK;
}

QuantDriver::QuantDriver(std::vector<Trace> *traces, Node* ast)
        : max_depth(MAX_DEPTH_DEFAULT){
    
    this->traces = traces;

    this->ast = ast;

    this->error_flag = OK;
}

QuantDriver::~QuantDriver(){

    // delete trees
    this->ast->destroy_children();
    delete ast;

    // delete traces
}

int QuantDriver::parse_traces(const std::fstream *source){
    // do actual parsing
    // how to store traces??
}

int QuantDriver::parse_formula(const std::string formula){
    // initialize ast
    // do actual parsing
    // format for formulae??
}

void QuantDriver::run(){
    // if top level unknown, parallelize
    if(this->ast->label == Subformula){
        this->run_parallel();
        return;
    }   

    // construct variables

    // construct constraint

    // create optimizer
    z3::optimize opt(this->opt_context);
    z3::params opt_params(this->opt_context);
    opt_params.set("priority", this->opt_context.str_symbol("pareto"));

    // add constraint
    // optimize
    // write result

    delete opt;
}

void QuantDriver::run_parallel(){
    std::vector<Node*> par_ast;
    std::vector<QuantDriver*> par_driver;
    std::vector<std::thread*> par_threads;

    for(int i = 1; i < Proposition; i++){ // swap in every operator
    // start with 1 to ignore Empty label
        par_ast.emplace_back();
        par_ast.back()->label = (ltl_op) i;
        this->construct_ast(par_ast.back(), this->max_depth);

        if(op_arity((ltl_op) i) == 1){
            par_ast[i-1]->left->label = Subformula;
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

        par_driver.emplace_back(this->traces, par_ast[i-1]);
        par_threads.emplace_back(&QuantDriver::run, par_driver[i-1]);
    }

    for(auto th : par_threads){ // wait for all threads to stop
        th->join();
    }

    // compute max of all results
    auto driver_iterator = std::distance(par_driver.begin(), 
                            std::max_element(par_driver.begin(), par_driver.end(), 
                                [](const QuantDriver &a, const QuantDriver &b){
                                    return (bool) (a.result.value < b.result.value);
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

const int op_arity(ltl_op o){

    switch (o)
    {
        case ltl_op::Not:
        case ltl_op::Globally:
        case ltl_op::Finally:

        #if GF_FRAGMENT
        #else
        case ltl_op::Next:
        #endif

            return 1;

        case ltl_op::And:
        case ltl_op::Or:
        
        #if GF_FRAGMENT
        #else
        case ltl_op::Until:
        #endif

            return 2;
        
        default:
            return 0;
    }
}