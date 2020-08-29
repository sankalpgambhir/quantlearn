#include "quantdriver.hh"

// initialize static variables as needed
int Trace::trace_count = 0;
int QuantDriver::ast_size = 0;
int QuantDriver::error_flag = 0;

std::map<ltl_op, char> operators = {
    {Empty , (char) 0}, // Free
    {Not , '-'},
    {Or , '+'},
    {And , '*'},
    #if GF_FRAGMENT
    //
    #else
        {Next , 'X'},
        {Until , 'U'},
    #endif
    {Globally , 'G'},
    {Finally , 'F'},
    {Subformula , 'S'},
    {Proposition , (char) 0}
};

QuantDriver::QuantDriver(const std::fstream* source, const std::string formula)
        : max_depth(MAX_DEPTH_DEFAULT){
    // parse traces
    if(!parse_traces(source)){
        QuantDriver::error_flag = FILE_PARSE_FAIL;
        return;
    }   
    
    // parse formula
    QuantDriver::ast_size = 0;
    if(!parse_formula(formula)){
        QuantDriver::error_flag = FORM_PARSE_FAIL;
        return;
    }

    QuantDriver::error_flag = OK;
}

QuantDriver::QuantDriver(std::vector<Trace> *traces, Node* ast)
        : max_depth(MAX_DEPTH_DEFAULT){
    
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

bool QuantDriver::parse_traces(const std::fstream *source){
    std::ostringstream ss;
    ss << source->rdbuf(); // reading data
    std::string str = ss.str();

    // clean whitespace
    str.erase(std::remove_if(str.begin(), str.end(), isspace), str.end()); 
    
    this->traces->emplace_back();
    Trace curr_trace = this->traces->back();

    std::string temp_prop = __empty;
    std::string temp_step = __empty;

    for(int i = 0; i < str.length(); i++){
        if(str[i] == PROP_DELIMITER){
            // check prop
            if(curr_trace.prop_inst.find(temp_prop) == curr_trace.prop_inst.end()){
                // add new prop
            }
            else{
                // add new instance of prop
            }
            // add prop to trace

            temp_prop = __empty;
            continue;

        }
        if(str[i] == STEP_DELIMITER){
            // tie up trace
            // compute props
            curr_trace.trace_string.back().emplace_back(temp_step);
            temp_step = __empty;
            continue;
        }
        if(str[i] == TRACE_DELIMITER){
            this->traces->emplace_back();
            curr_trace = this->traces->back();
            continue;
        }


    }

    // do actual parsing
    // how to store traces??

    return true;
}

bool QuantDriver::parse_formula(const std::string formula){
    // initialize ast
    // do actual parsing
    // format for formulae??
    return true;
}

void QuantDriver::run(){
    // if top level unknown, parallelize
    if(this->ast->label == Subformula){
        this->run_parallel();
        return;
    }   

    // construct variables
    for(auto tr : *this->traces){
        tr.construct_bit_matrices(this->opt_context, QuantDriver::ast_size);
    }

    // construct constraint

    // create optimizer
    z3::optimize opt(this->opt_context);
    z3::params opt_params(this->opt_context);
    opt_params.set("priority", this->opt_context.str_symbol("pareto"));

    // add constraint
    // optimize
    // write result

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

        //par_driver.emplace_back(this->traces, par_ast[i-1]);
        //par_threads.emplace_back([&](){par_driver[i-1]->run();});
        QuantDriver *temp_driver = new QuantDriver(this->traces, par_ast[i-1]);
        std::thread *temp_thread = new std::thread(&QuantDriver::run, par_driver[i-1]);
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
    }