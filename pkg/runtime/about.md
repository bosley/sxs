# Runtime

The runtime of the application helps define how we will be eventing/ storing data

the runtime has a series of datastores

using these data stores we have "Records" of "entities. 

These entities can be R/W permissioned onto a kv_c. 

A "session" is an instance of an entity who is working within a specified scope on the kv store

the session masks the scoping so the assigned entity can work regardless of the parent scoping, ensuring
we have data isolation

sessions have maximum number of sessions per-entity as a control specifier