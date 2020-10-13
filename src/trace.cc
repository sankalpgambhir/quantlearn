#include "trace.hh"

// initialize static variables
int Trace::trace_count = 0;

void Trace::compute_prop_counts(){
    // compute prop optimizations
    for(auto m : this->prop_inst){
        auto p = m.second;
        int count = 0;
        for (auto i = p.instances.begin(); i != p.instances.end(); i++){
            count = 0;
            auto j = i;
            while((*(j+1)).position - (*j).position == 1){
                count++;
                j++;
            }
            (*i).num_after = count;
            (*i).pos_next = (*(i+1)).position - (*i).position; // redundant? Can use iterators directly instead
        }
    }
}

void Trace::compute_not_props(){
    // compute prop optimizations
    std::vector<std::pair<std::string, proposition> > new_inserts;
    for(auto const &m : this->prop_inst){
        auto &s = m.first;
        auto &p = m.second;

        proposition np; // create a not prop
        np.name = "~" + s;
        np.instances.clear(); // make sure we're empty

        auto it = p.instances.begin();

        for(int i = 0; i < this->length; i++){
            if(it != p.instances.end()){ // else we have reached the end of p, keep adding ~p
                if(it->position == i){
                    it++; 
                    continue; // ~p doesn't hold here
                }
            }
            np.instances.emplace_back(i); // ~p here
        }
        new_inserts.push_back({np.name, np});
    }

    assert(new_inserts.size() == this->prop_inst.size()); // propositions should double

    for(auto &npair : new_inserts){
        this->prop_inst.insert(npair);
    }
}

void Trace::compute_optimizations(){
    this->compute_not_props();
    //this->compute_prop_counts();
}