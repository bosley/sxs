@ - 3 variants (load store)

debug - 1 variant 


# New


Important reminder: each operation we "load" the list - thats a copy. we then rotate and return - probably another copy in there.
this is preferred.
```
rotl <target list> <int>   -> returns ANY because it _can_ error, but otherwise it returns the number of rotations "left" as an int
rotr <target list> <int>   -> returns ANY like above, returns in, rotates right
rev <target list>          -> returns any cause can error, returns copy of target list, but reversed 
len <target list>          -> returns len of list
at <target list> <int>     -> returns a copy of the item at the index if valid, :none otherwise 
```