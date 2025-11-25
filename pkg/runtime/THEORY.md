# Project and Theory

I for some reason have an addiction to making things like this. The first one was "Nabla + Solace"
in 2020 where I made a bytecode and assembler + high level language. 

Anyway, here we go again 

In this case I was making a distributed kv store that has a networked real-time event system (insulalabs.io btw.)

It kind of tickled me thinking about the insi project as if it was multiple microcontrollers
all doing the actions needed to stay in sync and offer the eventing network.

So this sxs project is just what i made to express the idea. the name might change.
the concept is pretty simple.

We have the core concepts:

1) K/V Store
    - used to map any given key -> value. its a map.
    - has atomic operations on it `snx` `cas` 
    - can iterate keys based on a "prefix"

2) Objects
    - homoiconic representation where code and data have the same structure
    - uses SLP (S-expression Like Parser) format for both instructions and data
    - instructions are just lists, symbols, numbers, and strings
    - scripts can be stored in K/V and executed directly without parsing

3) Events
    - has "channels" that each contain "topics" that can be published and subscribed to
    - one channel is dedicated to dispatching commands to be executed
        the "topic" selected on this channel is how the processor can be selected to execute the command
    - 7 Channels "A-F" and all topics (uint16 size) can be freely used. I like to think of these similar
        to "registers" that subscribers are notified about on change. 

4) Core commands
    - minimal actions to perform primitive tasks
    - akin to machine instructions, though more complex in form
    - intended to be expanded and used as the backend for a higher level, more sane language set

# Core concepts

## KV Store

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

My implementation in insula handles "memory" and "disk" operations like this. Naturally this made me think about the heap and stack (despite not really being the same at all.) The thought occurred to me "what if the stack
was really just a spot on disk." Why? I don't know. As I considered what it could be useful for I thought about reentrant functions and otherwise returning to a task that was paused. 

So lets take a look at what isolated "stack" sections might look like if the data that backed variable
storage was really in the k/v database:

### Stack-like Isolation Using Prefixes

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

#### Task 001 View (prefix: `task:001:`)

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

#### Task 002 View (prefix: `task:002:`)

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

#### Reentrant Function Example

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

#### Stack Frame Hierarchy

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

# Objects

When considering this all in the context of a single controller unit and how we could leverage this for processing, the first natural question is "what are we processing." Well, if its a kv store we can store any given bytes so the structure is up to us. Data means nothing unless you give it value, and if we are going to create any semblance of instructions to make a controller we are going to need the notion of data and instruction.

## Instruction vs Data

What is an instruction? What is data? The distinction seems obvious until you really think about it, then it gets kind of philosophical.

At the lowest level, everything is just bytes. A chunk of memory holding `11101010101010110` doesn't inherently "mean" anything. It's just a pattern. But the moment I tell you "when you see that pattern, go do something," suddenly those bits become an instruction. Every time you encounter them, you execute some action. The pattern hasn't changed, but your relationship to it has.

An instruction is really just "a set of symbols or a pattern that when acted upon yields a predictable result." Its like a todo list - you see the item, you do the thing. The specific pattern doesn't matter. I could use `11101010101010110` or `MAKE_COFFEE` or `0x2A`. What matters is that we've agreed on a mapping between the pattern and the action.

Data, on the other hand, is information that gets operated on. But here's where it gets weird - the exact same bytes can be data in one context and an instruction in another. Look at this:

```
01001000 01100101 01101100 01101100 01101111
```

Is that data or an instruction? Well, if you're a text renderer, those bytes are data. They're ASCII characters spelling "Hello". But if you're a CPU and I tell you those bytes are machine code, suddenly they're instructions to execute. Same bytes, completely different interpretation.

### Homoiconicity

In the object representation I made for this runtime, I chose to use a `homoiconic` representation of instructions meaning that the instructions used to drive the controller are represented the same way, in memory, as the controller represents data.

Homoiconicity is one of those words that sounds way more complicated than it actually is. It just means "code and data have the same structure." The term comes from Lisp-family languages where the program you write looks exactly like the data structures the language uses internally.

In most languages, code is this special thing that gets compiled or interpreted into some other form. You write `if (x > 5) { doThing(); }` and the language transforms that into something completely different internally. Maybe an AST, maybe bytecode, maybe machine code. The structure of what you wrote bears little resemblance to how its actually represented in memory.

But in a homoiconic system, what you write IS the data structure. If I write:

```
(core/kv/set counter 42)
```

That's not just source code that gets parsed into some internal representation, that IS the internal representation. Its a list containing the symbol `core/kv/set`, the symbol `counter`, and the integer `42`. When the processor executes it, it's literally observing that list structure.

For this runtime, I built SLP (S-expression Like Parser) as the representation format. Its essentially s-expressions - nested lists with atoms. Everything is either:

- A list: `(something inside here)`
- A symbol: `core/kv/set` or `counter`
- A number: `42` or `3.14`
- A string: `"hello"`

That's it. And because instructions follow this same structure, there's no difference between storing a command in the K/V store, passing it as an event payload, or executing it. Its all just SLP objects.

Here's a concrete example. Say I want to store a script in the K/V store to execute later:

```
(core/kv/set script "(core/util/log \"Running deferred task\") (core/kv/set status \"complete\")")
```

Later, I can retrieve and execute it:

```
(core/kv/get script)
(core/expr/eval $result)
```

The bytes stored in the K/V store are the same format as the instructions being executed. When I call `eval`, I'm not parsing a string into some executable form because the SLP structure IS the executable form. I'm just pointing the processor at a different piece of data and saying "treat this as instructions."

This is why homoiconicity fits perfectly with the earlier point about instructions vs data. 
 Whether something is an instruction or data depends entirely on whether you're executing it or operating on it. And because they're the same structure, you can seamlessly move between the two modes.

## Object Structure and Immutability

The SLP object representation in this runtime has it such-that all objects are static, immutable blocks of data.

### Contiguous Buffer Model

Every SLP object lives in a single contiguous buffer managed by `slp_buffer_c`. There's no dynamic allocation happening when you work with the object structure. When you parse something like:

```
(core/kv/set counter 42)
```

The entire structure the outer list, the symbols, and the interger all get packed into one continuous chunk of memory. This includes:

- The actual units of storage (list headers, symbols, integers, strings)
- Offset arrays that tell where each element in a list lives
- A symbol table that maps symbol IDs to their string representations

Everything is laid out in this buffer, and once created, it doesn't change. The buffer is immutable.

### No Dynamic Allocation

This is a key design decision. Each list `()` is a fully contained set of data to describe itself. When you have nested lists like:

```
(outer (inner 1 2) 3)
```

Both the outer list and the inner list live in the same buffer. The outer list has an offset array pointing to where each of its children starts in the buffer. The inner list has its own offset array pointing to its children. Its all just offsets into the same contiguous memory.

### Buffer Ownership Model

The `slp_object_c` type that wraps these buffers has an ownership model:

- The root object owns the buffer (an `slp_buffer_c` instance and the symbol table)
- The object has a lightweight `view_` pointer that points to a specific unit within that buffer
- When you call `at(index)` on a list to get a child, it creates a new `slp_object_c` with its own copy of the full buffer and symbol table
- This child object now owns its copy of the buffer and points to a different location within it

This means extracting children from lists involves copying the buffer. The architecture prioritizes safety and ownership clarity over minimal copying. Each object you're holding knows it has exclusive ownership of its data. No shared pointers, no reference counting, no lifetime management complexity.

Moving an `slp_object_c` is efficient though, it just moves the buffer ownership which is cheap.

### Immutability

Once you create an SLP object, you can't modify it. You can't change a symbol, you can't add elements to a list, you can't mutate an integer. The buffer is read-only data.

If you want to create a new structure, you parse new text, which creates a new buffer. If you want to transform data, you build a new object. The original stays unchanged.

### Why This Design?

The contiguous, immutable buffer model gives us several advantages:

1. **Cache locality**: Everything is packed together, so when you traverse a structure, you're accessing adjacent memory
2. **Serialization is trivial**: The buffer already contains everything needed to reconstruct the object. You could write it directly to disk or send it over the network.
3. **No allocation during traversal**: Reading an SLP structure never allocates. You're just reading from the buffer.
4. **Thread-safe by default**: Immutability means no locks needed when multiple threads read the same object
5. **Simple lifetime management**: Each object owns its buffer. When the object goes out of scope, the buffer is freed. No shared ownership complexity.

The tradeoff is that copying happens when you extract children from lists. But in this runtime, we're typically traversing small command structures, not huge data trees. The simplicity and safety of the ownership model is worth the occasional copy.

### Practical Implications

When you execute a command like:

```
(core/kv/iterate "user:" 0 10 {
  (core/util/log "Found key:" $key)
  (core/kv/del $key)
})
```

The entire outer list is one immutable buffer. The handler body `{...}` is a child list in that same buffer. When the processor extracts the handler body to execute it for each iteration, it copies that portion of the buffer into a new `slp_object_c`. That handler object is now independent - it has its own buffer containing the handler code.

This independence is exactly what you want. Each iteration can potentially execute concurrently without worrying about the original command structure being modified or freed.

## Events

Now, lets think about events. What is eventing? In the context of this runtime its a way for different parts of the system to communicate without directly calling each other. Its pub/sub. One part publishes an event, other parts that subscribed to that event get notified and can react to it.

The thing that makes this interesting is that we have multiple "channels" (like radio stations) and each channel has "topics" (like different shows on that station). You pick a channel and a topic ID, and then youre able to
broadcast a message to anyone listening in.

### Event Architecture

The event system is built around a thread pool with a shared queue. The core flow is:

1. A producer publishes an event to a specific channel and topic
2. Event gets queued up in a thread-safe queue
3. Worker threads pull events from the queue and dispatch them
4. All registered consumers for that topic get notified

Pretty straightforward producer/consumer pattern with a thread pool to handle concurrent event processing.

#### Channels (Categories)

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

#### Event Structure

Each event has three parts:

- **category** (channel) - which channel is this on?
- **topic_identifier** (uint16) - which topic within that channel?
- **payload** (std::any) - the actual data being sent

So if you do:

```
(core/event/pub $CHANNEL_A 100 "hello world")
```

You're publishing to channel A, topic 100, with payload "hello world". Any subscribers listening on channel A topic 100 will receive that message.

#### Thread Pool Design

The event system spins up a configurable number of worker threads (default 4) that all share a single event queue. When an event comes in:

1. Producer acquires a lock and pushes event to the queue
2. If queue is full, producer blocks until space opens up (backpressure)
3. Worker threads wait on a condition variable for events
4. First available worker grabs the event and processes it
5. Processing means looking up all consumers registered for that topic and calling them

The queue has a max size (default 1000) to prevent unbounded memory growth. If the queue fills up, publishers block until space is available. This provides natural backpressure - if consumers can't keep up, producers slow down.

#### Consumer Registration

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

#### Why This Design?

The threaded queue design lets events be processed asynchronously without blocking the publisher. When you do `core/event/pub`, it just queues the event and returns immediately (unless the queue is full). The actual processing happens later on a worker thread.

This is particularly useful for the command execution channel. When a command comes in over the network, it gets queued as an event and the network handler can go back to listening for more commands. The processor picks it up when ready.

For the general channels (A-F), this means you can use them for coordinating work between different "tasks" or "processors" without them needing to know about each other. One task publishes, another subscribes, and the event system handles all the threading and delivery.

#### Blocking with Await

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

### Event Flow Example

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

## Core Commands

So we have the K/V store, objects (SLP), and events. What do we actually do with these? We need instructions. Commands. Something that tells the processor what actions to take.

The core commands are the primitive operations that make this runtime actually useful. They're organized into four namespaces based on what they operate on:

**core/kv** - K/V Store Operations (6 commands)
- `set` - write a key/value pair
- `get` - read a value by key
- `del` - delete a key
- `exists` - check if a key exists
- `snx` - set if not exists (atomic)
- `cas` - compare and swap (atomic)
- `iterate` - walk keys matching a prefix, execute a handler for each

These are your basic database operations. The atomic ones (`snx` and `cas`) are crucial for coordination between concurrent tasks. The iterate command is how you leverage the prefix behavior I talked about earlier for working with namespaced data.

**core/event** - Event System Operations (2 commands)
- `pub` - publish an event to a channel and topic
- `sub` - subscribe to a channel and topic with a handler

These hook directly into the event system. You can pub/sub on any of the six general purpose channels (A through F). The handler you pass to `sub` gets executed whenever an event arrives on that channel/topic.

**core/expr** - Expression Control (2 commands)
- `eval` - dynamically evaluate SLP text as code
- `await` - execute something and block waiting for a response

The `eval` command is where homoiconicity really shines. You can store SLP code in the K/V store, retrieve it, and execute it directly without any parsing step because the stored format is already executable. The `await` command gives you request/response patterns on top of pub/sub as evaluation happens concurrently.

**core/util** - Utility Operations (1 command)
- `log` - output messages to the session logger

Pretty straightforward. It takes variadic arguments so you can log multiple values in one call.

### Command Structure

Every command follows the same pattern. It's an SLP list where the first element is the command identifier (like `core/kv/set`) and the remaining elements are parameters:

```
(core/kv/set mykey "hello")
(core/event/pub $CHANNEL_A 100 "message")
(core/expr/eval "(core/util/log \"dynamic code\")")
```

All commands return either `true` on success or an ERROR type (formatted as `@"message"`) on failure. This makes it easy to check results and handle errors consistently.

### Variable Semantics

There's an important distinction in how symbols work. In general, symbols are explicit storage locations. If you want to work with a value, you have to explicitly get it from the K/V store:

```
(core/kv/set counter 42)
(core/kv/get counter)
```

There's no implicit loading. The symbol `counter` doesn't magically resolve to its value. You have to ask for it.

The exception is symbols prefixed with `$`. These are context variables provided by the runtime:

- `$CHANNEL_A` through `$CHANNEL_F` evaluate to the channel identifiers for event operations
- `$key` is injected into `core/kv/iterate` handler bodies with the current iteration key
- `$data` is injected into `core/event/sub` handler bodies with the event payload

These context variables are how the runtime passes information into your handlers without you having to explicitly load it from storage.

### Why These Commands?

You might notice this is a pretty minimal set. Only 12 commands total. That's intentional. These are meant to be the primitive operations, the machine instructions of this runtime. They're low level and a bit awkward to use directly for complex tasks.

The idea is that you'd build higher level abstractions on top of these. Maybe a proper language with nicer syntax, control flow constructs, functions, all that good stuff. But underneath, it would compile down to these core primitives.

Think of it like how high level languages compile to assembly. Nobody wants to write assembly by hand for their entire application, but having a clean, minimal instruction set as the foundation makes it possible to build whatever abstractions you want on top.

