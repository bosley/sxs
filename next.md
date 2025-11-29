# NEXT

kv store seems solid. we have install now and dynamic kernel loading. 

Now we need some basic goddamn language tools 

```

if

match

assert

```

the language is listerally just `def` `fn` and the imports/definitions of modules/kernels.

I wanted to get all the heavy lifting out of the way first. Now most of the bigger commands that can be considered "subsystems"
can be made as kernels and isntalled into SXS_HOME/.. dirs.


We dont have the concept of variables in sxs. Instead, we can def symbols that map them to an implementation of something (like a fn) but they
are unique, passed by copy, and die with the context ending. This is all by design.

The KV is a database and memory backed store. we can use memory for whatever in the "local process" and we can use the db for process interop
seeing as we have snx and cas we have atomicity

Once basic if match etc are made and tested, i want to focus on making a "bus" kernel. the "bus" should be a pub/sub broadcaster that we call into
and hand lambdas to. these lambdas will be stored in memory and when a subscription matches, it will call the lambda. this lambda can then do its operations
and if need be, event out itself, or store into the datastore. if no storage is required, it can be done.

# Prograsms

right now we ar ejust running sxs files. this is great, but i want the concept of "programs" to come after events and basic control flow.

This sill be so we can use ./sxs new --project <path> and have a specially structured progam created. from this we can also ./sxs new --kernel "my-kernel" when in
one of these projects to auto-gen (template) a c++ kernel and makefile. the make file should have `make` setup. updated, if sxs is given a dir instead of a file or if te file
given exists as a the init.sxs inmside a project, the sxs application loads the "project" in a different way than a reagular script. a project is made up of multople
scripts that we will define in directories for different use-cases to configure different systems. 

Think of the dir layout as a micro controller, and each sub dir is a different system. The db disk and events is how the different parts communicate with eachother while
runing. 