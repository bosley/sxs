If we add the following datum lists we can enforce type definitions to be done at the start
of the file (just like imports and kernels) and define lists with datum lists as such:



Base types :

```cpp 


enum class slp_type_e {
  NONE = 0,
  SOME = 1,
  PAREN_LIST = 2,
  BRACE_LIST = 4,
  DQ_LIST = 5,
  SYMBOL = 7,
  RUNE = 8,
  INTEGER = 9,
  REAL = 10,
  BRACKET_LIST = 11,
  ERROR = 12,
  DATUM = 13,
  ABERRANT = 14,
};

```

:any
:some
:list-p
:list-b
:list-c
:str
:symbol
:rune
:int
:real
:error
:datum
:aberrant

etc

see core/context.cpp

each type has ".." suffix also as a type i.e ":int.." meaning "a list of ints.

and we have :list / :list.. meaning "some list - we dont care about the inner type" 


we need:

```
[
  #(define-form myType {:int :str :real})
]
```

All "forms" derive from `:list-c` conceptually. We will allow all forms passed as if it were a `:list-c`.

When defining a form the typechecker needs to add the `..` form (meaning a list of this type containing one or more items) `:myType..` 

# Cast - runtime

the `cast` object, at runtime, if given a :list-c and a composit as destinatin MUST evaluate evey item in the composit, ensure it fits the `form` of the composit, and then will return it if it can. if the parameters dont match up exactly. we do not "unwrap" somes in here, we type-evaluate each item at runtime to see what it is and ensure it meets the form. if its a symbol obv we have to reslve it. 


if going from composit->list-c it shuld be a no-op as its perfectly valid, its only when attempting to go
from a :list-c -> composit is it an issue. if two items are composits only permit if they underlying data is the same and it matches (as if temporary upcast to list-c)


```
[
  #(define-form pair {:int :int})
  #(define-form two {:pair :pair :str}) ; now available in later forms
  (def x 3)
  (def a (cast :pair {1 x})) 


]
```