@ - 3 variants (load store)

debug - 1 variant 

# New

Important reminder: each operation we "load" the list - thats a copy. we then rotate and return - probably another copy in there.
this is preferred.
```
rev <target list>          -> returns any cause can error, returns copy of target list, but reversed 
len <target list>          -> returns len of list
idx <target list> <int>    -> returns a copy of the item at the index if valid, :none otherwise 
is <int> <symbol>          -> checks if item in object storage is of form type `:int` etc returns 1 or 0 

proc <list-b>
```


Functions: 


```

;  Tales 2 ints (evaluated and checked before call at runtime)
;
;
(@ 0 (fn [a :int b :int] :int {


}))


```