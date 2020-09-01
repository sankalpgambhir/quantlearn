#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix.hpp>
#include "quantdriver.hh"

namespace qi = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;
namespace phoenix = boost::phoenix;

using Value = boost::make_recursive_variant<std::string, double, std::vector<boost::recursive_variant_> >::type;
using Map = std::map<std::string, Value>;

template <typename Parser, typename ... Args>
void ParseOrDie(const std::string& input, const Parser& p, Args&& ... args)
{
    std::string::const_iterator begin = input.begin(), end = input.end();
    bool ok = qi::parse(begin, end, p, std::forward<Args>(args) ...);
    if (!ok || begin != end) {
        std::cout << "Unparseable: "
                  << std::quoted(std::string(begin, end)) << std::endl;
        throw std::runtime_error("Parse error");
    }
}

template <typename It>
struct ltlgrammar : qi::grammar<It, Map()> {
    ltlgrammar() : ltlgrammar::base_type(start) {
        using namespace qi;

        start     = skip(space) [*key_value];
        key_value = key >> '=' >> value;
        value     = v_string | v_array | v_num;

        // lexemes
        key = alpha >> *alnum;
        v_string  = "“" >> *('\\' >> char_ | ~char_("”")) >> "”";
        v_array   = '(' >> *value >> ')';
        v_num     = double_;

        BOOST_SPIRIT_DEBUG_NODES((start)(key_value)(value)(key)(v_string)(v_array)(v_num))
    }
  private:
    qi::rule<It, Map()> start;
    qi::rule<It, std::pair<std::string, Value>(), qi::space_type> key_value;
    qi::rule<It, Value(), qi::space_type> value;
    qi::rule<It, std::vector<Value>(), qi::space_type> v_array;

    qi::rule<It, std::string()> key, v_string;
    qi::rule<It, double()> v_num;
    // lexemes
};

// define operators

struct ltloperators_ : qi::symbols<char, ltl_op>
{
    ltloperators_()
    {
        add
        ('-', Not)
        ('+', Or)
        ('*', And)
        #if GF_FRAGMENT
        //
        #else
            ("X", Next)
            ("U", Until)
        #endif
        ('G', Globally)
        ('F', Finally);
    }

} ltloperators;

struct subform_ : qi::symbols<char, ltl_op>
{
    subform_()
    {
        add
        ('S', Subformula);
    }

} subform;