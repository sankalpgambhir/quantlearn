#include "parser.hh"

namespace Parser{

    std::ostream& operator<<(std::ostream& os, const expr& e){ 
        boost::apply_visitor(printer(os), e); 
        return os; 
    }

    bool parse_into_ast(Node* ast, std::string::const_iterator f, std::string::const_iterator l){
        parser<decltype(f)> p;

            try
            {
                expr result;
                bool ok = qi::phrase_parse(f,l,p > ';', qi::space, result);

                if (!ok){
                    Configuration::throw_error("Could not parse formula!");
                    return false;
                }
                else{
                    IFVERBOSE(std::cerr << "\nParse Result: " << result << "\n";)
                    // move into ast
                    if(!copy_to_ast(ast, result)){
                        Configuration::throw_error("Could not parse into AST");
                        exit(FORM_PARSE_FAIL);
                    }
                }

            } catch (const qi::expectation_failure<decltype(f)>& e)
            {
                std::string errstr = "Expectation failure at " + std::string(e.first, e.last) + "\n";
                Configuration::throw_error(errstr);
            }

            if (f != l) Configuration::throw_error("Unparsed formula: " + std::string(f,l));

        return true;
    };

    bool copy_to_ast(Node* ast, expr res){
        
        Parser::copier c;
        c.a = ast;

        return boost::apply_visitor(c)(res);

    };

}