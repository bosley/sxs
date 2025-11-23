# Record: Structured Data Persistence System

## Package Overview

The record package provides a framework for defining, managing, and persisting structured data types within the system. It enables applications to work with strongly-typed records that are automatically validated against schemas and persisted to key-value storage. The package acts as a bridge between application-level data structures and the underlying kvds storage layer, handling serialization, schema validation, concurrency control, and lifecycle management.

Records in this system are identified by a combination of type identifier and instance identifier. The type identifier represents the record's schema or class, while the instance identifier uniquely identifies a particular instance of that type. This two-level identification scheme allows multiple instances of the same record type to coexist while maintaining clear separation between different record types.

The record package integrates tightly with the sconf schema definition system and the slp parsing library to provide compile-time type safety combined with runtime schema validation. It leverages the kvds package for all storage operations, providing a higher-level abstraction that handles the complexities of distributed field storage, locking, and consistency.

## Core Architecture

The package defines a three-tier architecture consisting of interfaces, management infrastructure, and base implementations. At the interface level, record_if defines the contract that all record types must satisfy. This interface exposes methods for field access, persistence operations, and metadata retrieval. The interface is designed to be implemented by application-specific record types that define their own schemas and field structures.

The record_manager_c component serves as the central orchestrator for all record operations within the system. It acts as a factory for creating record instances, manages schema registration and validation, provides existence checking and iteration capabilities, and maintains the namespace structure within the underlying key-value store. The manager is the sole entry point for creating records and ensures consistency between schema definitions and stored data.

The record_base_c class provides a concrete base implementation of the record_if interface that handles common functionality shared across all record types. This includes the complete save and delete operations, lock acquisition and verification logic, instance identification management, and field storage. Application record types inherit from record_base_c and implement only the schema-specific methods related to field definitions and load operations.

## Storage Architecture

Records are persisted to the underlying kvds key-value store using a structured key namespace that enables efficient querying and isolation. The storage system uses three distinct key prefixes to organize different categories of data.

Schema metadata is stored under keys with the prefix "record:meta:" followed by the type identifier. Each schema key contains the complete schema definition as its value, allowing the system to validate schema consistency across restarts and detect incompatible schema changes. Only one schema can exist per type identifier, and attempting to register a different schema for an existing type results in an error.

Record field data is stored under keys with the prefix "record:data:" followed by the type identifier, instance identifier, and field index. Each field of a record instance is stored as a separate key-value pair, with the value being the field's serialized string representation. This field-level granularity allows for efficient partial updates and enables the storage of records with arbitrary field counts without requiring fixed-size storage allocations.

Locking state is maintained under keys with the prefix "record:lock:" followed by the type identifier and instance identifier. Lock keys store lock tokens that identify the current holder of a lock on a record instance. These locks provide optimistic concurrency control to prevent conflicting concurrent modifications to the same record instance.

### Storage Key Structure

```mermaid
graph TD
    Root[KV Store Namespace]
    Root --> Meta[record:meta:]
    Root --> Data[record:data:]
    Root --> Lock[record:lock:]
    
    Meta --> M1["record:meta:user<br/>(stores schema)"]
    Meta --> M2["record:meta:config<br/>(stores schema)"]
    
    Data --> D1["record:data:user:alice:0<br/>(field 0 value)"]
    Data --> D2["record:data:user:alice:1<br/>(field 1 value)"]
    Data --> D3["record:data:config:app:0<br/>(field 0 value)"]
    
    Lock --> L1["record:lock:user:alice<br/>(lock token)"]
    Lock --> L2["record:lock:config:app<br/>(lock token)"]
```

## Schema Management

The record system enforces schema validation through integration with the sconf configuration system. Every record type must provide a schema definition that describes its structure, field types, and constraints. When a record manager encounters a new type identifier, it validates the schema and registers it in the metadata storage before allowing any operations on records of that type.

Schema validation occurs in two phases. First, the schema string is parsed using the slp parser to verify syntactic correctness. This ensures the schema is well-formed and can be processed by the sconf system. Second, the schema is validated by the sconf configuration builder which checks type correctness, constraint validity, and structural integrity.

Once a schema is successfully validated and stored, it becomes the canonical definition for that record type. All subsequent attempts to create records of that type must provide an identical schema. This strict schema immutability prevents data corruption that could arise from schema evolution without migration. If schema changes are needed, applications must implement their own migration logic, potentially using a new type identifier for the evolved schema.

The schema registration process is idempotent. If a schema is already registered for a type identifier, the manager verifies that the provided schema exactly matches the existing schema. This allows multiple components to safely attempt registration of the same record type without coordination, as long as they agree on the schema definition.

### Schema Validation Flow

```mermaid
sequenceDiagram
    participant App as Application
    participant Mgr as record_manager_c
    participant Store as KV Store
    participant SLP as slp::parse
    participant Sconf as sconf_builder_c
    
    App->>Mgr: get_or_create<T>("instance_id")
    Mgr->>Mgr: Extract type_id and schema
    Mgr->>Mgr: ensure_schema_registered()
    
    Mgr->>Store: exists("record:meta:type_id")?
    
    alt Schema Already Registered
        Store-->>Mgr: Yes, return existing schema
        Mgr->>Mgr: Compare schemas
        alt Schemas Match
            Mgr-->>Mgr: Continue
        else Schema Mismatch
            Mgr-->>App: Error: Schema conflict
        end
    else New Schema
        Mgr->>SLP: parse(schema)
        alt Parse Success
            SLP-->>Mgr: Parsed structure
            Mgr->>Sconf: from(schema).Parse()
            alt Validation Success
                Sconf-->>Mgr: Valid
                Mgr->>Store: set("record:meta:type_id", schema)
                Store-->>Mgr: Success
            else Validation Failed
                Sconf-->>Mgr: Error
                Mgr-->>App: Error: Invalid schema
            end
        else Parse Failed
            SLP-->>Mgr: Error
            Mgr-->>App: Error: Malformed schema
        end
    end
```

## Record Lifecycle

Record instances progress through a well-defined lifecycle managed by the record_manager_c. The lifecycle begins when an application requests a record through the manager's get_or_create method, providing the desired instance identifier. This method is templated on the record type, enabling type-safe creation while allowing the manager to work with records through the common record_if interface.

When get_or_create is invoked, the manager first instantiates a new object of the requested type and extracts its type identifier and schema definition. It then ensures the schema is registered, failing the operation if schema validation or registration fails. The manager configures the record instance by setting its manager reference and instance identifier, establishing the bidirectional relationship needed for the record to perform storage operations.

The manager then checks whether data already exists for the specified instance identifier under the given type. If existing data is found, the manager invokes the record's load method which retrieves all field values from storage and populates the record's in-memory state. If no existing data is found, the record is returned in its initial state, ready to have fields set and then saved.

Throughout its active lifetime, a record instance can be read from and written to through its field accessor methods. These accessors operate on in-memory state and do not trigger storage operations. When changes need to be persisted, the application explicitly invokes the save method which writes all field values to storage atomically under lock protection.

When a record is no longer needed, the application can delete it from storage by calling the del method, or simply allow the record object to go out of scope if persistence is desired. Record objects do not maintain long-lived locks or require explicit cleanup beyond normal object destruction.

### Record Lifecycle Diagram

```mermaid
sequenceDiagram
    participant App as Application
    participant Mgr as record_manager_c
    participant Rec as Record Instance
    participant Store as KV Store
    
    App->>Mgr: get_or_create<UserRecord>("user_123")
    Mgr->>Rec: new UserRecord()
    Mgr->>Rec: set_manager(this)
    Mgr->>Rec: set_instance_id("user_123")
    
    Mgr->>Store: exists("record:data:user:user_123:...")?
    
    alt Record Exists
        Store-->>Mgr: Yes
        Mgr->>Rec: load()
        Rec->>Store: get("record:data:user:user_123:0")
        Store-->>Rec: field_0_value
        Rec->>Store: get("record:data:user:user_123:1")
        Store-->>Rec: field_1_value
        Note over Rec: Populate field_values_
    else New Record
        Store-->>Mgr: No
        Note over Rec: Return with default values
    end
    
    Mgr-->>App: unique_ptr<UserRecord>
    
    App->>Rec: set_field(0, "new_value")
    Note over Rec: Update in-memory state
    
    App->>Rec: save()
    Rec->>Rec: acquire_lock()
    Rec->>Store: set_nx("record:lock:user:user_123", token)
    Store-->>Rec: true (acquired)
    Rec->>Rec: verify_lock()
    Rec->>Store: get("record:lock:user:user_123")
    Store-->>Rec: token (matches)
    Rec->>Store: set("record:data:user:user_123:0", "new_value")
    Rec->>Store: set("record:data:user:user_123:1", value)
    Rec->>Rec: release_lock()
    Rec->>Store: del("record:lock:user:user_123")
    Rec-->>App: true
    
    App->>Rec: del()
    Rec->>Rec: acquire_lock()
    Rec->>Store: set_nx("record:lock:user:user_123", token)
    Store-->>Rec: true (acquired)
    Note over Rec: No verify_lock() call
    Note over Rec: Build vector of field keys
    Rec->>Store: delete_batch(["record:data:user:user_123:0",<br/>"record:data:user:user_123:1"])
    Store-->>Rec: true (batch deleted)
    Note over Rec: Direct lock cleanup
    Rec->>Store: del("record:lock:user:user_123")
    Note over Rec: Clear lock_token_
    Rec-->>App: true
```

## Concurrency Control

The record system implements optimistic locking to prevent conflicting concurrent modifications to the same record instance. The locking mechanism is transparent to most operations but provides critical safety guarantees when multiple components may access the same record.

Each record instance generates a unique lock token when it attempts to acquire a lock. The token combines a high-resolution timestamp with a cryptographically random value, making collision virtually impossible even across multiple processes and hosts. This token is generated fresh for each operation sequence and is cleared after the operation completes.

When a record needs to perform a mutating operation such as save or delete, it first attempts to acquire a lock by writing its token to the lock key in storage. The acquire_lock method uses the atomic set_nx operation provided by the kvds layer to perform an atomic set-if-not-exists on the lock key. This operation succeeds only if no lock currently exists, eliminating the race condition that would exist with separate read-then-write operations. If the lock is successfully created with the record's token, the acquisition succeeds immediately. If a lock already exists, the method checks whether it contains this record's own token, enabling multiple operations from the same record object without lock contention.

If a lock exists with a different token, the acquisition fails and the operation is aborted. This indicates another record object currently holds the lock on that instance, and concurrent modification is not safe. The application must retry the operation later or handle the conflict according to its business logic.

The save operation uses a two-phase locking pattern: after acquiring the lock, it calls verify_lock to confirm the lock is still held before writing field data. While the atomic set_nx primitive eliminates the primary race condition in lock acquisition, the verification step provides an additional safety check before committing field modifications. The del operation, in contrast, uses only acquire_lock without verification since it deletes the entire record and performs direct lock cleanup rather than calling release_lock.

The use of atomic set_nx for lock acquisition makes the record system safe for distributed and multi-process scenarios. Multiple processes can safely attempt to acquire locks on the same record instance, with the kvds layer ensuring only one succeeds atomically.

After completing a mutating operation, the record releases its lock. The save operation calls release_lock which deletes the lock key from storage and clears the in-memory lock token. The del operation performs direct cleanup by deleting the lock key and clearing the token inline rather than calling release_lock. Both approaches ensure that even if an operation fails partway through, locks do not leak and deadlock the system. Subsequent operations will generate a fresh lock token, preventing token reuse across operations.

The record manager performs lock cleanup during initialization, scanning for and removing all existing locks in the system. This recovery mechanism handles cases where locks were not properly released due to crashes or abnormal termination, ensuring a clean starting state.

### Lock Acquisition Flow

```mermaid
sequenceDiagram
    participant R1 as Record Object A
    participant R2 as Record Object B
    participant Store as KV Store
    
    Note over R1: First save attempt
    R1->>R1: save() calls acquire_lock()
    R1->>R1: Generate token<br/>(timestamp_random)
    R1->>Store: set_nx("record:lock:type:id", "token_A")
    Store-->>R1: true (lock acquired atomically)
    Note over R1: acquire_lock() succeeded
    R1->>R1: save() calls verify_lock()
    R1->>Store: get("record:lock:type:id")
    Store-->>R1: "token_A"
    Note over R1: Lock verified, write fields
    R1->>Store: Write all fields
    R1->>R1: release_lock()
    R1->>Store: del("record:lock:type:id")
    Note over R1: Token cleared in memory
    
    Note over R2: Concurrent save attempt
    R2->>R2: save() calls acquire_lock()
    R2->>R2: Generate token<br/>(timestamp_random)
    R2->>Store: set_nx("record:lock:type:id", "token_B")
    Store-->>R2: false (key exists)
    Note over R2: Atomic check failed!
    R2->>Store: get("record:lock:type:id")
    Store-->>R2: "token_A" (different token)
    R2-->>R2: acquire_lock() returns false
    Note over R2: save() aborts, no verify step
    
    Note over R1: Later, R1 tries again
    R1->>R1: acquire_lock()
    Note over R1: Token was cleared,<br/>generate new one
    R1->>R1: Generate NEW token
    R1->>Store: get("record:lock:type:id")
    Store-->>R1: Key not found
    R1->>Store: set("record:lock:type:id", "token_A_new")
    Note over R1: Success with new token
```

### Lock Token Lifecycle

```mermaid
stateDiagram-v2
    [*] --> NoToken: Record object created
    NoToken --> TokenGenerated: acquire_lock() called
    TokenGenerated --> LockAcquired: Write token to storage
    LockAcquired --> LockVerified: verify_lock() succeeds
    LockVerified --> OperationComplete: Field operations done
    OperationComplete --> NoToken: release_lock() clears token
    NoToken --> [*]: Record object destroyed
    
    LockAcquired --> NoToken: Operation fails, release_lock()
    TokenGenerated --> NoToken: Lock conflict, abort
    
    note right of NoToken
        lock_token_ is empty string
        Ready for next operation
    end note
    
    note right of TokenGenerated
        lock_token_ = "timestamp_random"
        Unique per attempt
    end note
```

## Field Storage Model

Individual record types define their fields and the mapping between field indices and field values. The record_base_c class maintains a field_values_ vector that stores the string representation of each field in index order. Derived record classes are responsible for serializing and deserializing their native field types to and from these string representations.

The save operation iterates through all fields in index order, constructing a storage key for each field and writing its string value to the key-value store. This field-by-field approach means each field is written in a separate storage operation. While this provides granularity, it also means save operations must acquire a lock to ensure consistency across all field writes.

The load operation, which must be implemented by derived record classes, performs the inverse transformation. It retrieves each field's string value from storage using the field index, then deserializes the string into the appropriate native type for that field. Load operations do not require locks as they are read-only and operate on a consistent snapshot provided by the underlying storage layer.

The delete operation removes all field keys for the record instance along with its lock key if present. The operation first builds a vector of all field keys, then uses the kvds delete_batch method to remove them atomically in a single operation. This is more efficient than individual deletions, especially for disk-backed stores where RocksDB can batch the deletions into a single write operation. The del operation executes under lock protection to prevent concurrent modification attempts from interfering with deletion, but unlike save, del uses only acquire_lock without the subsequent verify_lock step since it is deleting the entire record. After deleting all field keys, del performs direct cleanup by deleting the lock key and clearing the lock token, rather than calling the release_lock helper method.

## Query Capabilities

The record system provides several methods for discovering and iterating over stored records beyond simple existence checks. These query capabilities enable applications to enumerate records without knowing all instance identifiers in advance.

The exists method checks whether a specific record instance exists by searching for any keys matching that instance's storage prefix. It uses the efficient prefix iteration provided by kvds to quickly determine presence without retrieving all field values. The exists_any variant searches across all registered record types to determine if any record exists with the given instance identifier, regardless of type.

The iterate_type method enables enumeration of all instances of a particular record type. It scans the storage keys under that type's data prefix and extracts unique instance identifiers from the key structure. The method invokes a callback for each discovered instance identifier, allowing early termination if the callback returns false. This supports patterns like searching for specific instances or collecting a limited result set.

The iterate_all method provides global enumeration across all record types and instances in the system. It first iterates over all registered schemas to discover type identifiers, then for each type iterates over all instances of that type. The callback receives both the type identifier and instance identifier, enabling comprehensive traversal of the entire record space.

These iteration methods leverage the structured key namespace and the prefix-based iteration capabilities of the underlying kvds system. They are consistent with concurrent modifications, though they may see a snapshot that includes partially completed operations depending on the timing of lock acquisition and release.

### Query Operations Flow

```mermaid
sequenceDiagram
    participant App as Application
    participant Mgr as record_manager_c
    participant Store as KV Store
    
    Note over App,Store: exists() - Check single instance
    App->>Mgr: exists("user", "alice")
    Mgr->>Store: iterate("record:data:user:alice:")
    Store-->>Mgr: Found keys
    Mgr-->>App: true
    
    Note over App,Store: exists_any() - Check across all types
    App->>Mgr: exists_any("alice")
    Mgr->>Store: iterate("record:meta:")
    Store-->>Mgr: "record:meta:user"<br/>"record:meta:config"
    loop For each type
        Mgr->>Store: iterate("record:data:user:alice:")
        Store-->>Mgr: Found/Not found
    end
    Mgr-->>App: true if found in any type
    
    Note over App,Store: iterate_type() - Enumerate instances
    App->>Mgr: iterate_type("user", callback)
    Mgr->>Store: iterate("record:data:user:")
    Store-->>Mgr: "record:data:user:alice:0"<br/>"record:data:user:alice:1"<br/>"record:data:user:bob:0"
    Note over Mgr: Extract unique instance IDs
    Mgr->>App: callback("alice")
    Mgr->>App: callback("bob")
    
    Note over App,Store: iterate_all() - Global enumeration
    App->>Mgr: iterate_all(callback)
    Mgr->>Store: iterate("record:meta:")
    Store-->>Mgr: All type IDs
    loop For each type
        Mgr->>Mgr: iterate_type(type_id)
        loop For each instance
            Mgr->>App: callback(type_id, instance_id)
        end
    end
```

### Key Namespace Structure

```mermaid
graph LR
    subgraph "KV Store Key Space"
        M[record:meta:*]
        D[record:data:*]
        L[record:lock:*]
    end
    
    subgraph "Query Patterns"
        Q1[exists:<br/>Prefix scan instance]
        Q2[exists_any:<br/>Scan all types]
        Q3[iterate_type:<br/>Scan type prefix]
        Q4[iterate_all:<br/>Scan meta then types]
    end
    
    Q1 -->|"record:data:T:I:"| D
    Q2 -->|"record:meta:"| M
    Q2 -->|"then check each"| D
    Q3 -->|"record:data:T:"| D
    Q4 -->|"record:meta:"| M
    Q4 -->|"then iterate each"| D
```

## Integration Points

Schema validation integrates with both the slp parsing library and the sconf configuration system. Schemas are defined using the slp syntax and must be parseable by slp::parse. The parsed schema is then validated through sconf_builder_c to ensure semantic correctness and type system consistency. This two-phase validation provides robust error detection and clear failure messages.

The record package uses spdlog for all logging operations. Each record manager receives a logger during construction which it uses to log initialization events, schema validation results, errors, and debugging information. The logger integration allows applications to control logging verbosity and routing according to their operational requirements.

Record types themselves are defined by application code that inherits from record_base_c and implements the required interface methods. This inversion of control allows the record package to provide infrastructure while applications define schemas, types, and domain-specific behavior. The template-based factory pattern in get_or_create maintains type safety across this boundary.

## Key Features

### Type-Safe Record Creation

The record manager's get_or_create method uses C++ templates to provide compile-time type checking while working with records through a runtime interface. Applications request specific record types at compile time, and the template instantiation ensures only types inheriting from record_if can be created. This prevents incorrect types from entering the system while avoiding the overhead and limitations of runtime type identification.

### Automatic Schema Validation

Every record type must define its schema, and that schema is automatically validated before any operations occur. This validation happens transparently during the first interaction with each record type, ensuring data integrity from the start. Applications receive clear error messages if schemas are malformed or incompatible with existing registered schemas, making debugging straightforward.

### Instance Identifier Flexibility

The system places no constraints on instance identifier format or generation strategy. Applications can use UUIDs, sequential integers, composite keys, or any other string-based identifier scheme that fits their domain model. The record system treats instance identifiers as opaque strings and uses them only for storage key construction and equality comparison.

### Optimistic Concurrency Control

The lock token mechanism provides safety against concurrent modifications without requiring long-held locks or complex coordination protocols. Records acquire locks only during actual mutation operations and release them immediately after, minimizing contention. The token-based approach naturally handles distributed scenarios where multiple processes may access the same underlying storage.

### Extensible Field Serialization

While the storage layer works with string representations of field values, the record system does not dictate how native types are serialized to strings. Derived record classes implement their own serialization logic appropriate to their field types. This allows for human-readable serialization for debugging, compact binary encoding for efficiency, or any other format the application requires.

### Comprehensive Discovery

The iteration and existence checking capabilities allow applications to treat the record system as a queryable data store rather than just a simple persistence layer. Applications can enumerate records by type, search for specific instances, or scan the entire record space. These capabilities support use cases like administrative tools, debugging interfaces, and migration utilities.

