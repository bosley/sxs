#ifndef SXS_SLP_SLP_H
#define SXS_SLP_SLP_H

// simple language protocol 
/*
S- Expressions

()              =       list-p  (parens) 
{}              =       list-c  (curly)
[]              =       list-b  (bracket)
""              =       list-q  (quotes) - like a string but termed in lists. internally, each char is an int

'               =       quoted     - any object this is preifxed to: i.e '() '4 ''x
#               =       errpr      - same rules as quoted, but any attatched data is typed as an error for distinct digestin in user code
@               =       functional - signals to the SLP to parse a function object:

```
    This is the ONE and ONLY ONE "complex" type that SLP handles/manages. it MUST take this form or it will be invalid
    If "@" then next MUST be a list of: (<param list> <return typename> <body list (brackets)>)
    The parser will not do symbol resolution or anything of that nature, it simply defines the "primitive function"
    meaning that it can ONLY use types known to the parser (builtin types)

    @((a :int b :int c :int ) :int [
        ... instructions ... 
    ])
```

[0-9]+          =       integer (also with attatched signage -1 +3 etc)
[0-9]+.[0-9]+   =       real (also with prefixed signage)
w+              =       symbol (any contiguous string of non matched chars)
:w+             =       type-symbol. any "symbol" with a prefixed ":" is MUST signify a specific SLP type via the value it holds:
                            :int
                            :real
                            :numeric   int OR real
                            :list-p
                            :list-c
                            :list-b
                            :list-q
                            :list       any f the p,c,b,q lists
                            :some       (quoted   ')
                            :error      (error    #)
                            :fn         (lambda   @)
                            :none       its an empty list '() but signifies that it means to not take a value
                            :any        not a direct data, but means "any of the known types"
                            :okay       anything NOT an error (helper type)
                            :callable   :fn OR :list-p

Note:

While SLP parsing doesnt enforce this (obviouly, it cant) the paren list (list-p) will be interpreted as "instructions" or a thing _do_
to make it a data list `'()` is required otherwise the interpreter/compiler will enforce the first element being a callable function.
To bybass this and to enforce specific types, the `{}` list is a "passthrough" list that is "a list of data with no functionality or constraints"
and the `[]` list is a "contextualized execution" meaning "no symbol defined inside will be left over, each item will be evaluated" which naturally
means the last item in the list will be the result of the list post-evaluation, though that information is not specifically tracked on the object itself
at parse time (as slp isnt aware of external symbols, definition semantics, or conditionsls)

*/

enum slp_object_type_e {
    SLP_OBJECT_TYPE_NONE, // nothing - cant happen from text along
    SLP_OBJECT_TYPE_SOME, // '() quoted
    SLP_OBJECT_TYPE_LIST_P, // ()
    SLP_OBJECT_TYPE_LIST_C, // {}
    SLP_OBJECT_TYPE_LIST_B, // []
    SLP_OBJECT_TYPE_LIST_Q, // ""
    SLP_OBJECT_TYPE_ERROR, // #
    SLP_OBJECT_TYPE_FN, // @
    SLP_OBJECT_TYPE_INT, // [0-9]+
    SLP_OBJECT_TYPE_REAL, // [0-9]+.[0-9]+
    SLP_OBJECT_TYPE_SYMBOL, // w+
};

typedef struct slp_object_s {
    int type;
    void *data;
} slp_object_t;


#endif