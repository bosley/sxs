

- virtual lists

```scheme
some_symbol starts a (virtual list that 
doesnt_end until_newline) after all nets
```

then check value of mock callbacks (mimicing sxs callbacks) to ensure all callbacks and errors are correct by analyzing values
from lists, symbols, etc

note:

this is not to test the evaluation of code, rather, confirm the correctness of SLP pipeline of buffer -> slp_object
and to ensure there are no memory leaks etc within slp library