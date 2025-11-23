#pragma once

#include <memory>
#include <spdlog/spdlog.h>
/*
    Adapters are items that are used to abstract how the runtime can receive events remotely 

    FOr instance: with http an adapter could leverage the runtime to associate a password with an entity
    (though not mandatory, its an adapter-level concern) and then establish an web socket connection, permitting
    the publishing of events on behalf of that entity

    we could also do this with UDP, etc. 

    The primary idea being that the adapter is an object that is externally defined (outside of runtime)
    that through some means associates eventing with some external input by-way of the session object


    The final implementation of the software will contain MULTIPLE adapters in the event-driven system 

    An example of an HTTP- APIKey based adapter: 

        adapter maps a unqiue api key to an entity id in the record store 

        on POST, get the header and validate key, load entity.

        create ephemeral session with entity, 

        perform whatever work repated to the endpoint we need to do, 

        destroy the session

    In that adapter the sessions are made adhoc on-request. 

    However, in a long-lived connection we might have an adapter that "holds" the session active
    to continuously process. It is up to the adapter.

    The adapter is NOT to be limited to the entity session events though, an adapter should be able to pub/sub
    to/from any of the eventing of the runtime so the adapter itself can be fully integrated 



    the adapter will take the runtime-proper on creation but it ALSO must BE a subsystem
*/

namespace runtime {
  using logger_t = spdlog::logger*;
  class runtime_c; // cyclic include 
}

namespace runtime::module {

class adapter_c {
public:
  adapter_c(const adapter_c &) = delete;
  adapter_c(adapter_c &&) = delete;
  adapter_c &operator=(const adapter_c &) = delete;
  adapter_c &operator=(adapter_c &&) = delete;

  adapter_c(logger_t logger, runtime_c* runtime);
  ~adapter_c() = default;
};

using module_t = std::unique_ptr<adapter_c>;

} // namespace runtime::adapter