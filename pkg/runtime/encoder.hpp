#pragma once

#include <slp/slp.hpp>
#include <string>

namespace runtime {

/*
    The SLP object is a psuedo-compiled object. Once we parse it in
    we remove implied symbols and compact internals. If we need to
    for some reason get it back into a state where it can be re-parsed
    for re-evaluation sake (eval) we need to rehydrate the object and
    return it back to its original state
*/
std::string slp_object_to_string(const slp::slp_object_c &obj);

} // namespace runtime
