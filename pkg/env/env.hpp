#pragma once

/*



    A session defines an interaction with the runtime for emitting events and
   for receiving them as well as has a means to bind to a persistent scope (on
   disk) with a k/v database.

    The env package here is to define an environment that can process slp
   objects and interpret them to interact with the system.

    for instance :


    ```slp


        (kv/set aaa 3)
        (kv/snx aab 4) ; if it fails an "error type" is returned
        (kv/cas aab 4 22)

        (kv/iter "a" 0 100 (fn () ))


        ; we will add pub/sub functions after the kv functions are fully working

    ```




*/