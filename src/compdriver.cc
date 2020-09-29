#include "compdriver.hh"

comp::CompDriver::CompDriver(const std::fstream* source, const int max_depth){
    // parse the traces
    if(!parse_traces(source)){
        CompDriver::error_flag = FILE_PARSE_FAIL;
        return;
    }   

    // construct proposition valuations
    for(auto m : this->traces->front().prop_inst){
        // construct new node
        auto p = m.second;
        form_set.emplace_back();
        auto form_p = form_set.back();
        form_p->prop_label = m.first;

        for(auto t : *(this->traces)){
            form_p->holds.emplace_back(t.length, false);
            auto holds_pt = form_p->holds.back();
            for(auto inst : t.prop_inst[form_p->prop_label].instances){
                holds_pt[inst.position] = true;
            }
        }
    }

    // start with basic formulae
    // form_set = {G, F} x prop_set
    // a trailer for composition collector later
    int prev_size = form_set.size();
    for(auto f : form_set){
        for(auto op : {ltl_op::Globally, ltl_op::Finally}){
            form_set.emplace_back();
            auto curr = form_set.back();
            curr->label = op;
            curr->left = f;
        }
    }
    // done adding depth 1 formulae, remove depth zero
    form_set.erase(form_set.begin(), form_set.begin() + prev_size);

    // run while depth not reached
    for(int i = 1; i < max_depth; i++){
        this->run();
        // print all the formulas at each step?
    }
}

void comp::CompDriver::run(){

    std::vector<bool> check_holds(form_set.size(), false);
    int form_count = 0; // count is cheaper than find

    // run a check on the current set
    for(auto form : form_set){
        bool f_check = true;
        for(auto t : *(this->traces)){
            if(f_check){
                if(!check(form, &t)){
                    // formula doesn't hold on atleast one trace. 
                    // if it atleast passes an F-check, compute holds
                    if(form->label == ltl_op::Finally){ // a check on f is vacuously an f-check
                        f_check = false;
                    }
                    else{
                        f_check = false;
                        for(int i = 0; i < t.length; i++){
                            f_check |= check(form, &t, i);
                        }
                    }
                    
                    // stop checking for this formula
                    break;
                }
            }
        }
        check_holds[form_count++] = f_check;
    }

    // compute holds or throw out
    for(int i = 0; i < form_set.size(); i++){
        if(!check_holds[i]){
            form_set.erase(form_set.begin() + i);
            i--;
            continue;
        }

        if(!compute_holds(form_set[i])){ // computing holds found a violating trace
            form_set.erase(form_set.begin() + i);
            i--;
            continue;
        }
    }

    // compose the current operators
    if(!this->compose()){
        Configuration::throw_error("No formulas of higher depth available");
    }
}

bool comp::CompDriver::check(Node* f, Trace* t, const int i = 0){

    assert(f->left);

    std::vector<std::vector<bool> > *left_holds = &(f->left->holds);

    switch (f->label)
    {
    case ltl_op::And:
        return check(f->left, t, i) && check(f->right, t, i);
    case ltl_op::Or:
        return check(f->left, t, i) || check(f->right, t, i);
    case ltl_op::Globally:
        // is left child true everywhere? false not found
        return !(std::find(left_holds->begin() + i, left_holds->end(), false) == left_holds->end());
    case ltl_op::Finally:
        // is left child true anywhere? true found - written --> not (true not found)
        return !(std::find(left_holds->begin() + i, left_holds->end(), true) == left_holds->end());

    default: // not respecting one of GF, NNF, and composition
        Configuration::throw_error("Invalid formula type check");
        break;
    }

    return false;
}

bool comp::CompDriver::compute_holds(Node* f){

    

    switch (f->label)
    {
    case ltl_op::And:
        for(int j = 0; j < f->holds.size(); j++){
            bool f_check = false;
            for(int i = 0; i < f->holds[j].size(); i++){
                f_check |= (f->holds[j][i] = f->left->holds[j][i] & f->right->holds[j][i]);
            }
            if(!f_check){
                return false; // this trace violated an f-check, so we throw the formula out
            }
        }
    case ltl_op::Or:
        for(int j = 0; j < f->holds.size(); j++){
            bool f_check = false;
            for(int i = 0; i < f->holds[j].size(); i++){
                f_check |= (f->holds[j][i] = f->left->holds[j][i] | f->right->holds[j][i]);
            }
            if(!f_check){
                return false; // this trace violated an f-check, so we throw the formula out
            }
        }
    case ltl_op::Globally:
        // we start from the back and terminate soon as we hit a not holds condition
        // everything after it holds trivially
        for(int j = 0; j < f->holds.size(); j++){
            bool f_check = false;

            auto last = std::find(f->left->holds[j].rbegin(), f->left->holds[j].rend(), false);

            int fin = std::distance(last, f->left->holds[j].rbegin());
            f_check = (fin == 0);

            if(!f_check){
                return false; // this trace violated an f-check, so we throw the formula out
            }

            fin += f->holds[j].size(); // get a nice positive index

            // else we found the last hit
            for(int i = fin; i < f->holds[j].size(); i++){
                f->holds[j][i] = true;
            }
        }


    case ltl_op::Finally:
        // we start from the back and terminate soon as we hit a holds condition
        // everything before it holds trivially
        for(int j = 0; j < f->holds.size(); j++){
            bool f_check = false;

            auto last = std::find(f->left->holds[j].rbegin(), f->left->holds[j].rend(), true);

            int fin = std::distance(last, f->left->holds[j].rend());
            f_check = (fin == 0);

            if(!f_check){
                return false; // this trace violated an f-check, so we throw the formula out
            }

            // else we found the last hit
            for(int i = fin; i >= 0; i--){
                f->holds[j][i] = true;
            }
        }

    default: // not respecting one of GF, NNF, and composition
        Configuration::throw_error("Invalid formula type check");
        break;
    }

    return true;
}

bool comp::CompDriver::compose(){
    // todo
}
