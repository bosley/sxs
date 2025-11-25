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


# KV Store

Let's assume we have the following K/V Store:

| key               | value      |
|-------------------|------------|
| user:1:name       | "Alice"    |
| user:1:score      | 42         |
| user:2:name       | "Bob"      |
| user:2:score      | 17         |
| config:max_users  | 100        |
| metrics:cpu:avg   | 18.7       |
| session:abc123    | "active"   |

When we iterate over the prefix `user:`, we would select all keys starting with `user:`:

| key           | value   |
|---------------|---------|
| user:1:name   | "Alice" |
| user:1:score  | 42      |
| user:2:name   | "Bob"   |
| user:2:score  | 17      |

Iterating over the prefix `user:1` would result in:

| key           | value   |
|---------------|---------|
| user:1:name   | "Alice" |
| user:1:score  | 42      |


Working with these made me think about access rights and how we could leverage this behavior for building a multi-tenant system. I ended on locking user access to a specified and hidden "user prefix" (a uuid.)
For each user command of set/get i would add or remove this prefix behind the scenes. It added some access overhead but it also offered complete user isolation in the shared space.

My implementation in insi handles "memory" and "disk" operations like this. Naturally this made me think about the heap and stack (despite not really being the same at all.) The though occurred to me "what if the stack
was really just a spot on disk." Why? I don't know. As I considered what it could be useful for i thought about reenterant functions and otherwise returning to a task that was paused. 

So lets take a look at what isolated "stack" sections might look like if the data that backed variable
storage was really in the k/v database:

## Stack-like Isolation Using Prefixes

Each execution context (function call, task, or "stack frame") could have its own prefix in the K/V store. Consider three concurrent tasks executing:

**Global K/V Store State:**

| key                    | value        |
|------------------------|--------------|
| task:001:counter       | 5            |
| task:001:temp          | "processing" |
| task:001:result        | 42           |
| task:002:counter       | 12           |
| task:002:temp          | "waiting"    |
| task:002:user          | "Alice"      |
| task:003:counter       | 0            |
| task:003:result        | 99           |
| shared:config          | "enabled"    |
| shared:max_tasks       | 100          |

Each task operates in isolation using prefix-based "variable storage":

### Task 001 View (prefix: `task:001:`)

When iterating with prefix `task:001:`:

| key               | value        |
|-------------------|--------------|
| task:001:counter  | 5            |
| task:001:temp     | "processing" |
| task:001:result   | 42           |

**Operations:**
```
(core/kv/set counter 5)
(core/kv/set temp "processing")
(core/kv/get counter)

(core/kv/iterate "task:001:" 0 100 {
  (core/util/log "Variable:" $key)
})
```

### Task 002 View (prefix: `task:002:`)

When iterating with prefix `task:002:`:

| key              | value     |
|------------------|-----------|
| task:002:counter | 12        |
| task:002:temp    | "waiting" |
| task:002:user    | "Alice"   |

**Operations:**
```
(core/kv/set counter 12)
(core/kv/set user "Alice")
(core/kv/cas temp "waiting" "running")
```

### Reentrant Function Example

Consider a function that can be paused and resumed. Its state lives entirely in the K/V store:

**Initial State (Function Call ID: abc123)**

| key                      | value           |
|--------------------------|-----------------|
| fn:abc123:pc             | 0               |
| fn:abc123:arg:x          | 10              |
| fn:abc123:arg:y          | 20              |
| fn:abc123:local:sum      | 0               |
| fn:abc123:state          | "initialized"   |

**After Pause (PC = 5)**

| key                      | value           |
|--------------------------|-----------------|
| fn:abc123:pc             | 5               |
| fn:abc123:arg:x          | 10              |
| fn:abc123:arg:y          | 20              |
| fn:abc123:local:sum      | 30              |
| fn:abc123:state          | "paused"        |

**Resume Operations:**
```
(core/kv/get pc)
(core/kv/get local:sum)
(core/kv/cas state "paused" "running")
(core/kv/set pc 6)
```

The function can be completely reconstructed from the K/V store and continue execution exactly where it left off. Multiple instances of the same function can execute concurrently, each with their own prefix (like `fn:abc123:`, `fn:def456:`, etc.).

### Stack Frame Hierarchy

You could even model a call stack with nested prefixes:

| key                          | value     |
|------------------------------|-----------|
| stack:0:fn                   | "main"    |
| stack:0:local:result         | 0         |
| stack:1:fn                   | "process" |
| stack:1:local:input          | "data"    |
| stack:1:local:count          | 3         |
| stack:2:fn                   | "helper"  |
| stack:2:local:temp           | 42        |

**Iterating a specific frame:**
```
(core/kv/iterate "stack:1:" 0 100 {
  (core/util/log "Frame 1 variable:" $key)
})
```

**Popping a frame (cleanup):**
```
(core/kv/iterate "stack:2:" 0 100 {
  (core/kv/del $key)
})
```

# Events

Now, lets think about events. What is eventing? In the context of this runtime its a way for different parts of the system to communicate without directly calling each other. Its pub/sub. One part publishes an event, other parts that subscribed to that event get notified and can react to it.

The thing that makes this interesting is that we have multiple "channels" (like radio stations) and each channel has "topics" (like different shows on that station). You pick a channel and a topic ID, and then youre able to
broadcast a message to anyone listening in.

## Event Architecture

The event system is built around a thread pool with a shared queue. The core flow is:

1. A producer publishes an event to a specific channel and topic
2. Event gets queued up in a thread-safe queue
3. Worker threads pull events from the queue and dispatch them
4. All registered consumers for that topic get notified

Pretty straightforward producer/consumer pattern with a thread pool to handle concurrent event processing.

### Channels (Categories)

We've got 7 channels total:

| Channel | Enum | Purpose |
|---------|------|---------|
| Special | RUNTIME_EXECUTION_REQUEST | Command execution channel - this is where script commands get dispatched to processors |
| A | RUNTIME_BACKCHANNEL_A | General purpose event channel |
| B | RUNTIME_BACKCHANNEL_B | General purpose event channel |
| C | RUNTIME_BACKCHANNEL_C | General purpose event channel |
| D | RUNTIME_BACKCHANNEL_D | General purpose event channel |
| E | RUNTIME_BACKCHANNEL_E | General purpose event channel |
| F | RUNTIME_BACKCHANNEL_F | General purpose event channel |

The "execution request" channel is special - its how commands get routed to the right processor. When you execute a command, it gets published to this channel with the topic ID determining which processor handles it.

The A-F channels are what you access from runtime commands as `$CHANNEL_A` through `$CHANNEL_F`. Think of them as general-purpose communication buses.

### Event Structure

Each event has three parts:

- **category** (channel) - which channel is this on?
- **topic_identifier** (uint16) - which topic within that channel?
- **payload** (std::any) - the actual data being sent

So if you do:

```
(core/event/pub $CHANNEL_A 100 "hello world")
```

You're publishing to channel A, topic 100, with payload "hello world". Any subscribers listening on channel A topic 100 will receive that message.

### Thread Pool Design

The event system spins up a configurable number of worker threads (default 4) that all share a single event queue. When an event comes in:

1. Producer acquires a lock and pushes event to the queue
2. If queue is full, producer blocks until space opens up (backpressure)
3. Worker threads wait on a condition variable for events
4. First available worker grabs the event and processes it
5. Processing means looking up all consumers registered for that topic and calling them

The queue has a max size (default 1000) to prevent unbounded memory growth. If the queue fills up, publishers block until space is available. This provides natural backpressure - if consumers can't keep up, producers slow down.

### Consumer Registration

Multiple consumers can register for the same topic. When an event comes in on that topic, ALL registered consumers get called. This is how you can have multiple handlers reacting to the same event.

From the runtime commands perspective:

```
(core/event/sub $CHANNEL_A 100 {
  (core/util/log "Handler 1:" $data)
})

(core/event/sub $CHANNEL_A 100 {
  (core/util/log "Handler 2:" $data)
})
```

Both handlers would fire when someone publishes to channel A topic 100. The order they execute in is undefined (depends on which worker thread processes the event).

### Why This Design?

The threaded queue design lets events be processed asynchronously without blocking the publisher. When you do `core/event/pub`, it just queues the event and returns immediately (unless the queue is full). The actual processing happens later on a worker thread.

This is particularly useful for the command execution channel. When a command comes in over the network, it gets queued as an event and the network handler can go back to listening for more commands. The processor picks it up when ready.

For the general channels (A-F), this means you can use them for coordinating work between different "tasks" or "processors" without them needing to know about each other. One task publishes, another subscribes, and the event system handles all the threading and delivery.

### Blocking with Await

There's one special case - the `core/expr/await` command. This lets you publish something and then block waiting for a response on a different channel/topic:

```
(core/expr/await
  (core/event/pub $CHANNEL_A 1 "request")
  $CHANNEL_B
  100)
```

This publishes to channel A topic 1, then blocks waiting for a response on channel B topic 100. Under the hood it:

1. Registers a temporary one-shot consumer for channel B topic 100
2. Executes the body (the pub)
3. Blocks with a timeout waiting for the consumer to fire
4. Returns the event data when it arrives (or times out)

This gives you a request/response pattern on top of the pub/sub system. Useful for having one processor ask another processor to do something and wait for the result.

## Event Flow Example

Let's say we have two concurrent tasks coordinating via events:

**Task 1 (Producer):**
```
(core/kv/set request_id "abc123")
(core/event/pub $CHANNEL_A 50 "process_data")
(core/util/log "Request sent")
```

**Task 2 (Consumer):**
```
(core/event/sub $CHANNEL_A 50 {
  (core/util/log "Received request:" $data)
  (core/kv/set status "processing")
  (core/event/pub $CHANNEL_B 51 "complete")
})
```

**Task 1 (Waiting for completion):**
```
(core/expr/await
  (core/event/pub $CHANNEL_A 50 "process_data")
  $CHANNEL_B
  51)
```

The flow:
1. Task 1 publishes to channel A topic 50 and blocks waiting on channel B topic 51
2. Event system queues the event
3. Worker thread pulls it and calls Task 2's subscriber
4. Task 2 does its work and publishes to channel B topic 51
5. Task 1's await unblocks with the response data

All the threading, queueing, and synchronization is handled by the event system. The tasks just pub and sub. 