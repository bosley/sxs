We have the COMMANDS and type stuff down enugh for this level i think.
We can validate types for any given block of code and can work


Now we need to set a define to enable/disable the type enforcement at eval

THe idea is this:

Anywhere we have a dehydrated object (something that needs to be parsed) we ALSO
run the ts object over it. If it passes parsing and type checking, then we should
be safe to disable physical type checks in hot paths of the fns/ 

I want this toggleable for debugging and as an option (the hot path part) but
the type enforcement should be 100% standard. Nothing should hit the processor
that hasn't been type checked AND hydrated (parsed.)

This is a key reason why we store objects in the kv store "raw" this means we can
trust the type and encoded data!