#include "quantdriver.hh"

namespace comp{

class CompDriver final{
    public:
        CompDriver(const std::fstream* source, const int max_depth);

        // create a driver from a parsed trace set, for parallelization
        CompDriver(std::vector<Trace> *traces, Node* ast);
        CompDriver(const CompDriver&) = delete;
        CompDriver& operator=(const CompDriver&) = delete;

        ~CompDriver();

        void run();
        void run_parallel();

        static int error_flag;
        Result result;

    private:
        bool parse_traces(const std::fstream* source);
        bool parse_formula(const std::string formula);

        static void construct_ast(Node* ast, int depth);
        static bool check(Node*, Trace*, const int);
        bool compute_holds(Node*);
        bool compose();

        std::vector<Node*> form_set;

        std::vector<Trace> *traces;

        z3::context opt_context; // optimization context

        int max_depth;
};

}
