#include "quantdriver.hh"
#include <numeric>
#include <iomanip>

namespace comp{

class CompDriver final{
    public:
        CompDriver(const std::fstream& p_source, 
                   const std::fstream& n_source,
                   const int max_depth);

        // create a driver from a parsed trace set, for parallelization
        CompDriver(std::vector<Trace> *traces, Node* ast);
        CompDriver(const CompDriver&) = delete;
        CompDriver& operator=(const CompDriver&) = delete;

        ~CompDriver(){}

        void run(bool to_compose = true);

        int error_flag;
        Result result;

    private:
        bool parse_traces(const std::fstream& p_source, 
                          const std::fstream& n_source);

        static void construct_ast(Node* ast, int depth);
        bool check(Node*, Trace*, const int = 0);
        bool compute_holds(Node*);
        float compute_score(Node*);
        float compute_trace_score(Node*, Trace*, const int = 0);
        bool compose();

        std::vector<Node*> form_set; // formulae in current computation
        std::vector<Node*> used_form; // formulae of lower depth previously used, kept for compositions

        std::vector<std::pair<int, float> > scores; // computed scores of formulae in form_set

        std::vector<Trace> traces;

        z3::context opt_context; // optimization context

        int max_depth;
};

}
