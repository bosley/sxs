# Project and Theory

I for some reason have an addiction to making things like this. The first one was "Nabla + Solace"
in 2020 where I made a bytecode and assembler + high level language. 

Anway, here we go again 

In this case I was making a distributed kv store that has a networkd real-time event system (insulalabs.io btw.)

It kind of tickled me thinking about the insi project as if it was multiple microcontrollers
all doing the actions needed to stay in sync and offer the eventing network.

So this sxs project is just what i made to express the idea. the name might change.
the concept is pretty simple.

We have the core concepts:

1) K/V Store
    - used to map any given key -> value. its a map.
    - has atomic operations on it `snx` `cas` 
    - can iterate keys based on a "prefix"

2) Events
    - has "channels" that each contain "topics" that can be published and subscribed to
    - one channel is dedicated to dispatching commands to be executed
        the "topic" selected on this channel is how the processor can be selected to execute the command
    - 7 Channels "A-F" and all topics (uint32 size) can be freely used. I like to think of these similar
        to "registers" that subscribers are notified about on change. 