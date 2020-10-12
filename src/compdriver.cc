#include "compdriver.hh"

extern std::ostream& operator << (std::ostream& os, Node const* n);

comp::CompDriver::CompDriver(const std::fstream& p_source, 
                             const std::fstream& n_source,
                             const int max_depth){
    // parse the traces
    if(!parse_traces(p_source, n_source)){
        CompDriver::error_flag = FILE_PARSE_FAIL;
        return;
    }   

    // construct proposition valuations
    for(auto m : this->traces.front().prop_inst){
        // construct new node
        auto p = m.second;
        std::cout << m.first << std::endl;
        form_set.emplace_back(new Node());
        auto form_p = form_set.back();
        form_p->label = ltl_op::Proposition;
        form_p->prop_label = m.first;

        for(auto &t : this->traces){
            form_p->holds.emplace_back(t.length, false);
            auto &holds_pt = form_p->holds.back();
            for(auto inst : t.prop_inst[form_p->prop_label].instances){
                holds_pt[inst.position] = true;
            }
        }
    }

    // start with basic formulae
    // form_set = {G, F} x prop_set
    // a trailer for composition collector later
    int prev_size = form_set.size();
    for(int i = 0; i < prev_size; i++){
        for(auto op : {ltl_op::Globally, ltl_op::Finally}){
            form_set.emplace_back(new Node());
            auto curr = form_set.back();
            curr->label = op;
            curr->left = form_set[i];
        }
    }
    // done adding depth 1 formulae, remove depth zero
    form_set.erase(form_set.begin(), form_set.begin() + prev_size);

    // run while depth not reached
    for(int i = 1; i < max_depth; i++){
        IFDEBUG(
            std::cerr << "Formulae before run:";
            for(auto f : form_set){
                std::cerr << f << '\n';
            }
        )
        this->run((i != (max_depth-1)));
        // print all the formulas at each step?
        IFDEBUG(
            std::cerr << "\nFormulae after run:" << form_set.size() << '\n';
            for(auto f : form_set){
                std::cerr << f << '\n';
            }
        )
    }

    /*
    // print all formulae at the end

    for(auto f : form_set){
        std::cout << f;
    }
    */

    // compute scores
    for(int i = 0; i < form_set.size(); i++){
        this->scores.push_back({form_set[i]->id, compute_score(form_set[i])});
    }

    // sort scores
    std::sort(this->scores.begin(), this->scores.end(),
                    [](const std::pair<int, float> &a, const std::pair<int, float> &b){
                        return a.second > b.second;
                    });

    std::cout << std::fixed;
    std::cout << std::setprecision(3); // limit score printing

    for(int i = 0; (i < NUM_TO_PRINT) && (i < this->scores.size()); i++){
        std::cout << '\n' << i+1 << ". " 
                    << *std::find_if(form_set.begin(), 
                                     form_set.end(), 
                                     [&](Node* f){return f->id == this->scores[i].first;}) 
                    << '\t' << this->scores[i].second;
    }

}

void comp::CompDriver::run(bool to_compose){

    std::vector<bool> check_holds(form_set.size(), false);
    int form_count = 0; // count is cheaper than find

    // run a check on the current set
    for(auto form : form_set){
        bool f_check = true;
        for(auto &t : this->traces){
            if(f_check){
                if(((bool)t.parity ^ check(form, &t))){ // parity xor check. are parity and check diff?
                    // formula doesn't hold on atleast one trace. 
                    // (or holds but with negative parity)
                    // if it atleast passes an F-check, compute holds
                    if(form->label == ltl_op::Finally){ // a check on f is vacuously an f-check
                        f_check = false;
                    }
                    else{
                        f_check = false;
                        for(int i = 0; i < t.length; i++){
                            f_check |= !((bool)t.parity ^ check(form, &t, i));
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
    if(to_compose){
        if(!this->compose()){
            Configuration::throw_error("No formulas of higher depth available");
        }
    }
}

bool comp::CompDriver::parse_traces(const std::fstream &p_source, const std::fstream &n_source){
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
        Trace &curr_trace = this->traces.back();
        curr_trace.trace_string.emplace_back();

        for(auto m : this->traces.front().prop_inst)
                    curr_trace.prop_inst.insert({m.first, Trace::proposition(m.first)});

        std::string temp_prop = __empty;
        std::string temp_step = __empty;
        int curr_step = 0;

        for(int i = 0; i < str[j].length(); i++){
            if(str[j][i] == PROP_DELIMITER 
                    || str[j][i] == STEP_DELIMITER 
                    || str[j][i] == TRACE_DELIMITER){
                // check prop
                if((curr_trace.prop_inst.find(temp_prop) == curr_trace.prop_inst.end()) && temp_prop != __empty){
                    // add new prop to EVERY trace
                    // if it's not in this trace, it's not in any trace
                    for(auto &t : this->traces){
                        t.prop_inst.insert({temp_prop, Trace::proposition(temp_prop)});
                    }
                }

                // add new instance of prop
                curr_trace.prop_inst[temp_prop].instances.emplace_back(curr_step);
            
                // add prop to trace
                temp_step += temp_prop;
                temp_prop = __empty;
                temp_step += str[j][i];

                if(str[j][i] == PROP_DELIMITER) continue;

            }
            if(str[j][i] == STEP_DELIMITER 
                    || str[j][i] == TRACE_DELIMITER){
                curr_trace.trace_string.back().emplace_back(temp_step);
                temp_step = __empty;
                curr_step++;
                curr_trace.length++;
                if(str[j][i] == STEP_DELIMITER) continue;
            }
            if(str[j][i] == TRACE_DELIMITER){
                curr_trace.parity = j;

                if(i != (str[j].length() - 1)){
                    this->traces.emplace_back();
                    curr_trace = this->traces.back();
                    curr_step = 0;
                }

                // add all old props to new trace
                for(auto m : this->traces.front().prop_inst)
                    curr_trace.prop_inst.insert({m.first, Trace::proposition(m.first)});
                continue;
            }

            // not a delimiter, move on
            temp_prop += str[j][i];

        }
    }
    // let every trace individually compute while we go on
    // do NOT use curr_trace for async, since it'll be modified 
    // this breaks things in unimaginable ways
    // trace computations moved to the end to allow all propositions to be collected.
    for(auto &t : this->traces){
        async_trace_comp.emplace_back(
                std::async(&Trace::compute_optimizations, t));
    }
    
    // make sure all trace computations finished
    for(auto &comp : async_trace_comp){
        comp.get();
    }

    return true;
}

bool comp::CompDriver::check(Node* f, Trace* t, const int i){

    assert(f->left);

    auto t_iter = std::find_if(this->traces.begin(), this->traces.end(), [t](Trace &inp){return t->id == inp.id;});
    assert(t_iter != this->traces.end());

    int t_index = std::distance(this->traces.begin(), t_iter);

    auto &left_holds = (f->left->holds[t_index]);

    switch (f->label)
    {
    case ltl_op::And:
        return check(f->left, t, i) && check(f->right, t, i);
    case ltl_op::Or:
        return check(f->left, t, i) || check(f->right, t, i);
    case ltl_op::Globally:
        // is left child true everywhere? not (false not found)
        return (std::find(left_holds.begin() + i, left_holds.end(), false) ==  left_holds.end());
    case ltl_op::Finally:
        // is left child true anywhere? true found - written --> not (true not found)
        return !(std::find(left_holds.begin() + i,  left_holds.end(), true) ==  left_holds.end());

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
                f_check |= !(((bool) this->traces.at(j).parity) 
                                ^ (f->holds[j][i] = f->left->holds[j][i] & f->right->holds[j][i]));
            }
            if(!f_check){
                return false; // this trace violated an f-check, so we throw the formula out
            }
        }
    case ltl_op::Or:
        for(int j = 0; j < f->holds.size(); j++){
            bool f_check = false;
            for(int i = 0; i < f->holds[j].size(); i++){
                f_check |= !(((bool) this->traces.at(j).parity) 
                                ^ (f->holds[j][i] = f->left->holds[j][i] | f->right->holds[j][i]));
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
            f_check |= !(((bool) this->traces.at(j).parity) 
                            ^ (fin == 0));

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
            f_check |= !(((bool) this->traces.at(j).parity) 
                            ^ (fin == 0));

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
    // add depth n formulae from the current set of depth n-1

    if(form_set.empty()){
        return false;
    }

    int prev_size = form_set.size();

    // unary
    for(int i = 0; i < prev_size; i++){
        for(auto op : {ltl_op::Globally, ltl_op::Finally}){
            form_set.emplace_back();
            auto curr = form_set.back();
            curr->label = op;
            curr->left = form_set[i];
        }
    }

    // binary
    for(int i = 0; i < prev_size; i++){
        for(int j = 0; j < prev_size; i++){ // compositions of current depth
            for(auto op : {ltl_op::And, ltl_op::Or}){
                form_set.emplace_back();
                auto curr = form_set.back();
                curr->label = op;
                curr->left = form_set[i];
                curr->right = form_set[j];
            }
        }
        for(auto f : used_form){ // compositions with lower depths
            for(auto op : {ltl_op::And, ltl_op::Or}){
                form_set.emplace_back();
                auto curr = form_set.back();
                curr->label = op;
                curr->left = form_set[i];
                curr->right = f;
            }
        }
    }

    // move n-1 depth formulae to used_form
    for(int i = 0; i < prev_size; i++){
        used_form.push_back(form_set[i]);
    }
    
    // done adding depth n formulae, remove depth n-1
    form_set.erase(form_set.begin(), form_set.begin() + prev_size);

    return true;
}

float comp::CompDriver::compute_score(Node* f){

    std::vector<float> t_scores;

    for(auto &t : this->traces){
        if(t.parity == positive){
            t_scores.emplace_back(compute_trace_score(f, &t));
            assert(t_scores.back() > 0.0);
            IFDEBUG(std::cerr << "\nGot score " << t_scores.back()
                        << " on trace " << t.id << " and formula " << f;)
        }
    }

    // process trace scores
    // todo

    return std::accumulate(t_scores.begin(), t_scores.end(), decltype(t_scores)::value_type(0));
}



float comp::CompDriver::compute_trace_score(Node* f, Trace* t, const int pos){

    assert(pos < t->length);

    float res = 0;

    auto t_iter = std::find(this->traces.begin(), this->traces.end(), *t);
    assert(t_iter != this->traces.end());

    int t_index = std::distance(this->traces.begin(), t_iter);

    int l_pos, count;

    switch (f->label)
    {
        case ltl_op::Proposition:
            return float(f->holds[t_index][pos]);

        case ltl_op::And:
            return compute_trace_score(f->left, t, pos) * compute_trace_score(f->right, t, pos);

        case ltl_op::Or:
            return (compute_trace_score(f->left, t, pos) + compute_trace_score(f->right, t, pos)) / 2.0;

        case ltl_op::Globally:

            res = 0;

            for(int i = pos; i < t->length; i++){
                assert(compute_trace_score(f->left, t, i) > 0.0);
                res += retarder(i - pos) * compute_trace_score(f->left, t, i);
            }

            return res;

        case ltl_op::Finally:

            res = 0;
            count = 0;

            for(int i = pos; i < t->length; i++){
                if(f->left->holds[t_index][i]){
                    res += retarder(i - pos) * compute_trace_score(f->left, t, i);
                    count++;
                    break; // only consider first, remove to average
                }
            }            

            return res / (float) count;

        default: // not respecting GF
            Configuration::throw_error("Invalid formula type for score calculation");
            // terminate here? or continue and return 0?
            break;
    }

    return 0.0;
}