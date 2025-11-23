#include <slp/slp.hpp>
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <string>
#include <variant>

class environment_c {
public:
    std::map<std::string, std::variant<std::int64_t, double, std::string>> bindings;
    
    void set(const std::string& name, std::variant<std::int64_t, double, std::string> value) {
        bindings.emplace(name, std::move(value));
    }
    
    std::variant<std::int64_t, double, std::string>* get(const std::string& name) {
        auto it = bindings.find(name);
        if (it != bindings.end()) {
            return &it->second;
        }
        return nullptr;
    }
};

class interpreter_c {
public:
    environment_c global_env;
    
    void print_value(const std::variant<std::int64_t, double, std::string>& val) {
        if (std::holds_alternative<std::int64_t>(val)) {
            std::cout << std::get<std::int64_t>(val);
        } else if (std::holds_alternative<double>(val)) {
            std::cout << std::get<double>(val);
        } else if (std::holds_alternative<std::string>(val)) {
            std::cout << std::get<std::string>(val);
        }
    }
    
    std::variant<std::int64_t, double, std::string> eval(const slp::slp_object_c& obj) {
        auto type = obj.type();
        
        if (type == slp::slp_type_e::INTEGER) {
            return obj.as_int();
        }
        
        if (type == slp::slp_type_e::REAL) {
            return obj.as_real();
        }
        
        if (type == slp::slp_type_e::SYMBOL) {
            std::string sym = obj.as_symbol();
            auto* val = global_env.get(sym);
            if (val) {
                return *val;
            }
            return sym;
        }
        
        if (type == slp::slp_type_e::DQ_LIST) {
            return obj.as_string().to_string();
        }
        
        if (type == slp::slp_type_e::PAREN_LIST) {
            auto list = obj.as_list();
            if (list.empty()) {
                return std::int64_t(0);
            }
            
            auto first = list.at(0);
            if (first.type() == slp::slp_type_e::SYMBOL) {
                std::string cmd = first.as_symbol();
                
                if (cmd == "let" && list.size() >= 3) {
                    auto name_obj = list.at(1);
                    auto value_obj = list.at(2);
                    
                    if (name_obj.type() == slp::slp_type_e::SYMBOL) {
                        std::string name = name_obj.as_symbol();
                        auto val = eval(value_obj);
                        global_env.set(name, std::move(val));
                    }
                    return std::int64_t(0);
                }
                
                if (cmd == "putln" && list.size() >= 2) {
                    auto value_obj = list.at(1);
                    auto val = eval(value_obj);
                    print_value(val);
                    std::cout << std::endl;
                    return std::int64_t(0);
                }
                
                if (cmd == "add" && list.size() >= 3) {
                    auto left = eval(list.at(1));
                    auto right = eval(list.at(2));
                    
                    if (std::holds_alternative<std::int64_t>(left) && 
                        std::holds_alternative<std::int64_t>(right)) {
                        return std::get<std::int64_t>(left) + std::get<std::int64_t>(right);
                    }
                    return std::int64_t(0);
                }
            }
            
            return std::int64_t(0);
        }
        
        if (type == slp::slp_type_e::BRACE_LIST) {
            auto list = obj.as_list();
            if (list.size() >= 2) {
                auto env_obj = list.at(0);
                auto key_obj = list.at(1);
                
                if (env_obj.type() == slp::slp_type_e::SYMBOL) {
                    std::string env_name = env_obj.as_symbol();
                    
                    if (env_name == "$") {
                        if (key_obj.type() == slp::slp_type_e::SYMBOL) {
                            std::string key = key_obj.as_symbol();
                            auto* val = global_env.get(key);
                            if (val) {
                                return *val;
                            }
                        }
                    }
                }
            }
            return std::int64_t(0);
        }
        
        if (type == slp::slp_type_e::BRACKET_LIST) {
            return std::string("[bracket-list]");
        }
        
        return std::int64_t(0);
    }
    
    void execute(const slp::slp_object_c& obj) {
        std::cout << "Executing SLP program..." << std::endl;
        std::cout << "Type: ";
        
        auto type = obj.type();
        switch(type) {
            case slp::slp_type_e::BRACKET_LIST: std::cout << "BRACKET_LIST"; break;
            case slp::slp_type_e::PAREN_LIST: std::cout << "PAREN_LIST"; break;
            case slp::slp_type_e::BRACE_LIST: std::cout << "BRACE_LIST"; break;
            case slp::slp_type_e::INTEGER: std::cout << "INTEGER"; break;
            case slp::slp_type_e::REAL: std::cout << "REAL"; break;
            case slp::slp_type_e::SYMBOL: std::cout << "SYMBOL"; break;
            case slp::slp_type_e::DQ_LIST: std::cout << "DQ_LIST"; break;
            default: std::cout << "OTHER";
        }
        std::cout << std::endl << std::endl;
        
        if (type == slp::slp_type_e::BRACKET_LIST) {
            auto list = obj.as_list();
            std::cout << "Iterating over bracket list with " << list.size() << " elements" << std::endl;
            
            for (size_t i = 0; i < list.size(); i++) {
                std::cout << "  [" << i << "] ";
                auto elem = list.at(i);
                eval(elem);
            }
            std::cout << std::endl;
        } else {
            eval(obj);
        }
    }
};

int main(int argc, char** argv) {
    std::string filename = "example.slp";
    
    if (argc > 1) {
        filename = argv[1];
    }
    
    std::cout << "SLP Example - Reading and executing: " << filename << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return 1;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();
    
    std::cout << "Source code:" << std::endl;
    std::cout << source << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    
    auto result = slp::parse(source);
    
    if (result.is_error()) {
        std::cerr << "Parse error: " << result.error().message << std::endl;
        std::cerr << "At byte position: " << result.error().byte_position << std::endl;
        return 1;
    }
    
    std::cout << "Parse successful!" << std::endl << std::endl;
    
    interpreter_c interpreter;
    interpreter.execute(result.object());
    
    std::cout << std::endl << std::string(60, '=') << std::endl;
    std::cout << "Execution complete." << std::endl;
    
    return 0;
}

