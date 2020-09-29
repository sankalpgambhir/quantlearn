#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/variant/recursive_wrapper.hpp>
#include "configuration.hh"
#include "quantdriver.hh"

namespace Parser {

namespace qi  = boost::spirit::qi;
namespace phx = boost::phoenix;

// TAGS
// boolean
struct op_or  {};
struct op_and {};
struct op_not {};
// temporal
struct op_next {};
struct op_until {};
struct op_globally {};
struct op_finally {};
// meta
struct op_subform {};

typedef std::string var;
template <typename tag> struct binop;
template <typename tag> struct unop;

typedef boost::variant<var, int,
        boost::recursive_wrapper<unop <op_not> >, 
        boost::recursive_wrapper<binop<op_and> >,
        boost::recursive_wrapper<binop<op_or> >,
        boost::recursive_wrapper<unop <op_next> >, 
        boost::recursive_wrapper<binop<op_until> >,
        boost::recursive_wrapper<unop<op_globally> >,
        boost::recursive_wrapper<unop<op_finally> >, 
        boost::recursive_wrapper<unop<op_subform> >
        > expr;

template <typename tag> struct binop 
{ 
    explicit binop(const expr& l, const expr& r) : oper1(l), oper2(r) { }
    expr oper1, oper2; 
};

template <typename tag> struct unop  
{ 
    explicit unop(const expr& o) : oper1(o) { }
    expr oper1; 
};

struct printer : boost::static_visitor<void>
{
    printer(std::ostream& os) : _os(os) {}
    std::ostream& _os;

    //
    void operator()(const var& v) const { _os << v; }
    void operator()(const int& i) const { _os << i; }

    void operator()(const binop<op_and>& b) const { print(" & ", b.oper1, b.oper2); }
    void operator()(const binop<op_or >& b) const { print(" | ", b.oper1, b.oper2); }
    void operator()(const binop<op_until >& b) const { print(" U ", b.oper1, b.oper2); }
    
    void print(const std::string& op, const expr& l, const expr& r) const
    {
        _os << "(";
            boost::apply_visitor(*this, l);
            _os << op;
            boost::apply_visitor(*this, r);
        _os << ")";
    }

    void operator()(const unop<op_not>& u) const
    {
        _os << "(";
            _os << "!";
            boost::apply_visitor(*this, u.oper1);
        _os << ")";
    }

    void operator()(const unop<op_next>& u) const
    {
        _os << "(";
            _os << "X ";
            boost::apply_visitor(*this, u.oper1);
        _os << ")";
    }

    void operator()(const unop<op_globally>& u) const
    {
        _os << "(";
            _os << "G ";
            boost::apply_visitor(*this, u.oper1);
        _os << ")";
    }

    void operator()(const unop<op_finally>& u) const
    {
        _os << "(";
            _os << "F ";
            boost::apply_visitor(*this, u.oper1);
        _os << ")";
    }

    void operator()(const unop<op_subform>& u) const
    {
        _os << "(";
            _os << "S ";
            boost::apply_visitor(*this, u.oper1);
        _os << ")";
    }
};

std::ostream& operator<<(std::ostream& os, const expr& e)
{ boost::apply_visitor(printer(os), e); return os; }


template <typename Iter, typename Skipper = qi::space_type>
    struct parser : qi::grammar<Iter, expr(), Skipper>
{
    parser() : parser::base_type(expr_)
    {
        using namespace qi;

        expr_ = 
            (
                ( ("not"  >   simple_ [_val = phx::construct<unop<op_not>>(_1)]       )) 
                |
                ( ("sub"  >   int_ [_val = phx::construct<unop<op_subform>>(_1)]       )) 
                |
                ( ("globally"  >   simple_ [_val = phx::construct<unop<op_globally>>(_1)]       )) 
                |
                ( ("finally"  >   simple_ [_val = phx::construct<unop<op_finally>>(_1)]       )) 
            #if GF_FRAGMENT
                //
            #else
                |
                ( ("next"  >   simple_ [_val = phx::construct<unop<op_next>>(_1)]       ))  
            #endif
                |
                simple_    [_val = phx::construct<expr>(_1)])

            >> *( 
                    ("and"  >>  simple_ [_val = phx::construct<binop<op_and>>(_val, _1)]  )
                    |
                    ("or"   >>  simple_ [_val = phx::construct<binop<op_or>>(_val, _1)]   )
                #if GF_FRAGMENT
                    //
                #else
                    |
                    ("until"   >>  simple_ [_val = phx::construct<binop<op_until>>(_val, _1)]   )   
                #endif
                    |
                    ("not"  >   simple_ [_val = phx::construct<unop<op_not>>(_val)]       )  
                );

        simple_ = (('(' > expr_ > ')') | var_);
        var_ = qi::lexeme[ +alpha ];


        BOOST_SPIRIT_DEBUG_NODE(expr_);
        BOOST_SPIRIT_DEBUG_NODE(simple_);
        BOOST_SPIRIT_DEBUG_NODE(var_);
    }

    private:
    qi::rule<Iter, var() , Skipper> var_;
    qi::rule<Iter, expr(), Skipper> simple_, expr_;
};

template <typename Node, typename Iter = std::string::const_iterator>
bool parse_into_ast(Node* ast, Iter f, Iter l){
    parser<Iter> p;

        try
        {
            expr result;
            bool ok = qi::phrase_parse(f,l,p > ';',qi::space,result);

            if (!ok){
                Configuration::throw_error("Could not parse formula!");
                return false;
            }
            else{
                std::cout << "result: " << result << "\n";
                // move into ast
                if(!copy_to_ast(ast, result)){
                    Configuration::throw_error("Could not parse into AST!");
                }
            }

        } catch (const qi::expectation_failure<Iter>& e)
        {
            std::string errstr = "Expectation failure at " + std::string(e.first, e.last) + "\n";
            Configuration::throw_error(errstr);
        }

        if (f!=l) std::cerr << "unparsed: '" << std::string(f,l) << "'\n";

    return true;
};

template <typename Node, typename expres>
struct copier{
    // kinda dumb, make it nicer probably
    static bool copy_expr(Node* a, var v){a->label = ltl_op::Proposition; a->prop_label = v; return true;}
    static bool copy_expr(Node* a, int i){a->label = ltl_op::Subformula; a->subformula_size = i; return true;}

    static bool copy_expr(Node* a, unop<op_not> u){a->label = ltl_op::Not; copy_expr(a->l, u.oper1); return true;}
    static bool copy_expr(Node* a, unop<op_globally> u){a->label = ltl_op::Globally; copy_expr(a->l, u.oper1); return true;}
    static bool copy_expr(Node* a, unop<op_finally> u){a->label = ltl_op::Finally; copy_expr(a->l, u.oper1); return true;}
    static bool copy_expr(Node* a, unop<op_subform> u){a->label = ltl_op::Subformula; copy_expr(a->l, u.oper1); return true;}

    #if GF_FRAGMENT
        //
    #else
    static bool copy_expr(Node* a, unop<op_next> u){a->label = ltl_op::Next; copy_expr(a->l, u.oper1); return true;}
    static bool copy_expr(Node* a, binop<op_until> b){a->label = ltl_op::Until; copy_expr(a->l, b.oper1); copy_expr(a->r, b.oper2); return true;}
    #endif

    static bool copy_expr(Node* a, binop<op_and> b){a->label = ltl_op::And; copy_expr(a->l, b.oper1); copy_expr(a->r, b.oper2); return true;}
    static bool copy_expr(Node* a, binop<op_or> b){a->label = ltl_op::Or; copy_expr(a->l, b.oper1); copy_expr(a->r, b.oper2); return true;}

    static bool copy_expr(Node* a, expres e){return false;}
};

template <typename Node, typename expres = expr>
    bool copy_to_ast(Node* ast, expres res){

    return copier<Node, expres>::copy_expr(ast, res);

};


}


/*

int main()
{
    for (auto& input : std::list<std::string> {

            /// Simpler tests:
            "a until b;",
            "(a or b);",
            "sub 3;",
            "next (globally a);",
            "not a and b;",
            "not (a until (finally b));",
            })
    {
        auto f(std::begin(input)), l(std::end(input));
        parser<decltype(f)> p;

        try
        {
            expr result;
            bool ok = qi::phrase_parse(f,l,p > ';',qi::space,result);

            if (!ok)
                std::cerr << "invalid input\n";
            else
                std::cout << "result: " << result << "\n";

        } catch (const qi::expectation_failure<decltype(f)>& e)
        {
            std::cerr << "expectation_failure at '" << std::string(e.first, e.last) << "'\n";
        }

        if (f!=l) std::cerr << "unparsed: '" << std::string(f,l) << "'\n";
    }

    return 0;
}

*/
