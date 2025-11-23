# Session System Architecture

## Overview

The session system provides permission-based resource access control through the combination of persistent entities and ephemeral sessions. Entities hold long-term permissions for both KV store scopes and event system topics, while sessions enforce these permissions at runtime for every operation.

## Core Components

### Entity (`entity_c`)

Persistent permission holder that maintains two independent permission maps:

- **Scope Permissions**: Control access to KV store namespaces
  - `READ_ONLY` ("R"): Read operations only
  - `WRITE_ONLY` ("W"): Write operations only
  - `READ_WRITE` ("RW"): Full read/write access

- **Topic Permissions**: Control event system participation
  - `PUBLISH` ("P"): Can publish events to topic
  - `SUBSCRIBE` ("S"): Can subscribe to topic events
  - `PUBSUB` ("PS"): Both publish and subscribe

### Session (`session_c`)

Temporary runtime context that links an entity, a scope, and the event system:

- Creates a scoped KV store wrapper at construction
- Validates entity permissions on every operation
- Enables dynamic event category selection per publish
- Returns `false` on permission denial (fail silently)

### Scoped KV Store (`scoped_kv_c`)

Permission-enforcing wrapper around the underlying `kvds::kv_c`:

- Automatically prefixes all keys with scope name (e.g., `"my_scope/key1"`)
- Checks entity permissions before each operation
- Isolates different scopes from each other
- Transparently handles key prefixing/unprefixing

## Permission Flow Diagrams

### Entity Permission Structure

```mermaid
graph TD
    Entity[Entity: user123]
    Entity --> ScopePerms[Scope Permissions]
    Entity --> TopicPerms[Topic Permissions]
    
    ScopePerms --> S1["scope_a: READ_WRITE"]
    ScopePerms --> S2["scope_b: READ_ONLY"]
    
    TopicPerms --> T1["topic_100: PUBLISH"]
    TopicPerms --> T2["topic_200: PUBSUB"]
```

### Session Creation & KV Scoping

```mermaid
sequenceDiagram
    participant Sub as session_subsystem
    participant Ent as entity_c
    participant Sess as session_c
    participant SKV as scoped_kv_c
    participant KV as underlying kvds
    
    Sub->>Ent: get_entity(entity_id)
    Sub->>Sess: new session(scope="data_scope")
    Sess->>SKV: new scoped_kv_c(kv, "data_scope", entity)
    
    Note over SKV: All keys automatically prefixed
    
    Sess->>SKV: set("key1", "val")
    SKV->>Ent: is_permitted("data_scope", "W")?
    alt Permission granted
        SKV->>KV: set("data_scope/key1", "val")
    else Permission denied
        SKV-->>Sess: return false
    end
```

### Event Permission Flow

#### Publishing Events

```mermaid
sequenceDiagram
    participant Sess as session_c
    participant Ent as entity_c
    participant ES as event_system
    participant Prod as producer
    participant Writer as topic_writer
    
    Sess->>Sess: publish_event(category, topic_id, payload)
    Sess->>Ent: is_permitted_topic(topic_id, "P")?
    
    alt Has PUBLISH or PUBSUB permission
        Sess->>ES: get_event_producer_for_category(category)
        ES-->>Sess: producer
        Sess->>Prod: get_topic_writer_for_topic(topic_id)
        Prod-->>Sess: topic_writer
        Sess->>Writer: write_event()
        Note over Writer: Event published with category
    else No permission
        Sess-->>Sess: return false
    end
```

#### Subscribing to Events

```mermaid
sequenceDiagram
    participant Sess as session_c
    participant Ent as entity_c
    participant ES as event_system
    participant Consumer as event_consumer
    
    Sess->>Sess: subscribe_to_topic(category, topic_id, handler)
    Sess->>Ent: is_permitted_topic(topic_id, "S")?
    
    alt Has SUBSCRIBE or PUBSUB permission
        Sess->>Sess: Store handler for (category, topic_id) pair
        Sess->>ES: register_consumer(topic_id, consumer)
        Note over Sess: Handler only invoked for matching category
        Sess-->>Sess: return true
    else No permission
        Sess-->>Sess: return false
    end
    
    Note over ES: When event arrives...
    ES->>Consumer: consume_event(event)
    Consumer->>Sess: Look up handler by (event.category, event.topic_id)
    alt Handler found for this category+topic
        Consumer->>Sess: invoke handler(event)
    else No handler for this category
        Note over Consumer: Event ignored
    end
```

## Key Behaviors

### KV Store Isolation

- Sessions see ONLY their scope prefix
- Key `"user_data"` becomes `"my_scope/user_data"` in underlying store
- Different sessions with different scopes cannot interfere
- Permission checks happen before EVERY operation

### Event System Filtering

- Topic permissions checked on publish AND subscribe
- Category can be dynamically chosen per event
- **Subscriptions are category-specific**: `subscribe_to_topic(category, topic_id, handler)` only receives events matching both the category and topic_id
- Events published to a different category on the same topic will not trigger the handler
- Permissions stored persistently in entity (topic permissions only, not category-specific)
- Failed operations return false (fail silently)
- Unsubscribing requires both category and topic_id: `unsubscribe_from_topic(category, topic_id)`
- Note: Consumer remains registered in event system (no unregister API), but won't be invoked

### Session Lifecycle

- Created with `entity_id` + `scope`
- Scope permissions NOT validated at creation time
- Operations fail at runtime if permissions missing
- Sessions can be created for any scope regardless of entity permissions

### Category-Scoped Event Subscriptions

- Subscriptions are identified by `(category, topic_id)` pairs
- Multiple subscriptions to the same `topic_id` but different categories are independent
- Example: Subscribing to `(BACKCHANNEL_A, 100)` will NOT receive events published to `(BACKCHANNEL_B, 100)`
- This enables category-based event routing without requiring separate topic IDs
- Topic permissions remain category-agnostic (one permission grants access to a topic across all categories)

## Code Examples

### Example 1: Scope Permission Setup

```cpp
entity->grant_permission("user_data", permission::READ_WRITE);
entity->grant_permission("config", permission::READ_ONLY);
entity->save();
```

### Example 2: Session with Scoped Access

```cpp
auto session = subsystem.create_session("user123", "user_data");
auto* store = session->get_store();

store->set("preferences", "dark_mode");

auto session2 = subsystem.create_session("user123", "config");
auto* store2 = session2->get_store();
store2->set("key", "val");
```

Note: If entity "user123" only has READ_ONLY permission to "config" scope (as shown in Example 1), the `store2->set()` call will return `false`.

### Example 3: Event Publishing & Subscribing

```cpp
// Grant permissions
entity->grant_topic_permission(100, topic_permission::PUBSUB);
entity->grant_topic_permission(200, topic_permission::PUBLISH);
entity->save();

auto session = subsystem.create_session("user123", "data");

// Publish to BACKCHANNEL_A on topic 100
session->publish_event(event_category_e::RUNTIME_BACKCHANNEL_A, 100, data);

// Subscribe to BACKCHANNEL_A on topic 100
session->subscribe_to_topic(
    event_category_e::RUNTIME_BACKCHANNEL_A, 
    100, 
    [](const event_s& event) {
        // Handler only receives events published to BACKCHANNEL_A on topic 100
        // Events on BACKCHANNEL_B topic 100 will NOT trigger this handler
    }
);

// Publish to topic 200 (entity has PUBLISH permission)
session->publish_event(event_category_e::RUNTIME_BACKCHANNEL_A, 200, data);
// Returns true - succeeds with PUBLISH permission

// Attempting to subscribe to topic 200 would fail (no SUBSCRIBE permission)
// session->subscribe_to_topic(event_category_e::RUNTIME_BACKCHANNEL_A, 200, handler);
// Returns false - entity only has PUBLISH, not SUBSCRIBE or PUBSUB

// Unsubscribe from specific category+topic combination
session->unsubscribe_from_topic(event_category_e::RUNTIME_BACKCHANNEL_A, 100);
```

### Example 4: Category-Scoped Subscription Isolation

```cpp
entity->grant_topic_permission(500, topic_permission::PUBSUB);
entity->save();

auto session = subsystem.create_session("user123", "data");

std::atomic<int> channel_a_count{0};
std::atomic<int> channel_b_count{0};

// Subscribe to topic 500 on BACKCHANNEL_A
session->subscribe_to_topic(
    event_category_e::RUNTIME_BACKCHANNEL_A, 
    500, 
    [&](const event_s& event) { channel_a_count++; }
);

// Subscribe to the SAME topic 500 but on BACKCHANNEL_B
session->subscribe_to_topic(
    event_category_e::RUNTIME_BACKCHANNEL_B, 
    500, 
    [&](const event_s& event) { channel_b_count++; }
);

// Publish to BACKCHANNEL_A topic 500
session->publish_event(event_category_e::RUNTIME_BACKCHANNEL_A, 500, "data1");
// Result: channel_a_count = 1, channel_b_count = 0

// Publish to BACKCHANNEL_B topic 500
session->publish_event(event_category_e::RUNTIME_BACKCHANNEL_B, 500, "data2");
// Result: channel_a_count = 1, channel_b_count = 1

// Each handler only receives events from its subscribed category
```

## Design Rationale

### Why separate Entity and Session?

- **Entity**: Persistent, shared across sessions, manages long-term permissions
- **Session**: Ephemeral, runtime context, enforces permissions

### Why fail silently (return false)?

- Caller can check return value
- No exceptions in hot path
- Consistent error handling pattern

### Why check permissions at operation time?

- Allows dynamic permission changes
- Simpler session creation
- No need to validate all possible operations upfront

## Permission Matrix

| Operation | Signature | Requires Scope Permission | Requires Topic Permission |
|-----------|-----------|--------------------------|--------------------------|
| `store->get()` | `get(key)` | READ_ONLY or READ_WRITE | N/A |
| `store->set()` | `set(key, value)` | WRITE_ONLY or READ_WRITE | N/A |
| `session->publish_event()` | `publish_event(category, topic_id, payload)` | N/A | PUBLISH or PUBSUB |
| `session->subscribe_to_topic()` | `subscribe_to_topic(category, topic_id, handler)` | N/A | SUBSCRIBE or PUBSUB |
| `session->unsubscribe_from_topic()` | `unsubscribe_from_topic(category, topic_id)` | N/A | N/A (just removes handler) |

**Note**: Topic permissions are checked per `topic_id` only. The `category` parameter in subscribe/publish controls event routing but does not require separate permissions per category.

