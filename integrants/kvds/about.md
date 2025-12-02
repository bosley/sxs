# KVDS: Key-Value Datastore System

## Package Overview

The kvds package provides a unified abstraction layer for key-value storage operations within the system. It enables applications to work with string-based key-value pairs through a consistent interface while supporting multiple backend storage engines. The package is designed with flexibility in mind, allowing clients to choose between in-memory storage for speed or persistent disk storage for durability without changing their interaction patterns.

The kvds package is tightly integrated with components from the types package, specifically leveraging lifetime management and reference counting primitives to ensure proper resource cleanup and enable safe sharing of store instances across multiple consumers.

## Core Architecture

The package defines a hierarchical interface structure that separates concerns between different storage operations. At the base level, the kv_stat interface provides status information about a store, primarily whether it is currently open and ready for operations. Building on this, the kv_reader_c interface defines read-only operations including single key retrieval, existence checks, and prefix-based iteration over stored entries. Complementing this is the kv_writer_c interface which handles all modification operations such as setting values, deleting entries, and performing batch writes.

The kv_c interface unifies these three separate concerns into a single complete interface that provides full read-write access to a key-value store. This separation allows for future extension where read-only or write-only views of a store might be useful, while the current implementation primarily uses the complete kv_c interface.

The architecture supports two distinct backend implementations, each optimized for different use cases. The memory-based backend provides fast, volatile storage suitable for temporary data and caching scenarios. The disk-based backend utilizes RocksDB to provide persistent storage with durability guarantees suitable for long-lived application state.

## Storage Backends

### Memory Store

The memory store backend maintains all key-value pairs in process memory using a standard map structure. Operations are extremely fast as they involve no disk I/O, but all data is lost when the store is closed or the process terminates. The memory store is thread-safe through internal mutex protection, making it suitable for concurrent access patterns. This backend is ideal for session data, temporary caches, or scenarios where data loss on restart is acceptable or even desired.

### Disk Store

The disk store backend leverages RocksDB to provide persistent key-value storage with strong durability guarantees. Data written to a disk store survives process restarts and system reboots. The RocksDB engine provides efficient storage, compression, and indexing capabilities that make it suitable for large datasets. Like the memory store, the disk store is thread-safe and suitable for concurrent access. This backend should be used for application state that must survive restarts, user data, configuration information, and any other data requiring persistence.

The disk store requires a filesystem path where it will create its database directory structure. Multiple disk stores can coexist as long as they use different directory paths.

## The Distributor Component

The distributor serves as a centralized factory and registry for key-value store instances within the application. Rather than creating and managing store instances directly, clients interact with the distributor to request access to stores by unique identifier. This pattern provides several important benefits.

When a client requests a store through the distributor, they provide a unique string identifier and specify which backend type they require. If a store with that identifier already exists in the distributor's registry, the existing instance is returned. If no such store exists, the distributor creates a new store instance, initializes it appropriately for the chosen backend, and caches it for future requests before returning it to the caller.

For memory stores, the distributor simply creates the store with an empty identifier since no filesystem path is needed. For disk stores, the distributor combines its configured base path with the unique identifier to construct a full filesystem path where the RocksDB database will be stored. This ensures each disk store has its own isolated directory within a common root.

The distributor maintains separate registries for memory and disk stores, allowing the same unique identifier to be used for both a memory store and a disk store without conflict. This enables patterns where an application might maintain both an in-memory cache and a persistent copy of the same logical datastore.

## Lifetime Management Integration

The kvds package employs sophisticated lifetime tracking to ensure proper resource cleanup when store references are no longer needed. Each store instance returned by the distributor is wrapped in a container that includes a lifetime_tagged_c object from the types package.

The lifetime_tagged_c provides an observer pattern where an observer is notified when the lifetime object is destroyed. Each wrapper is assigned a unique numeric tag when created, and this tag is provided to the observer upon destruction. The distributor acts as the lifetime observer for all wrappers it creates.

When a wrapper object is destroyed, typically because all references to it have been released, its lifetime_tagged_c destructor executes and calls the distributor's death notification handler with the wrapper's unique tag. The distributor maintains a mapping from tags to store identifiers, allowing it to determine which store has reached end-of-life. This mechanism provides the distributor with visibility into when stores are no longer in use, enabling it to perform cleanup operations or remove entries from its registry.

The lifetime tracking operates transparently to clients of the kvds system. Clients simply work with store references and the underlying machinery automatically handles notification and cleanup when references are released.

## Reference Counting with shared_obj_c

Store instances are distributed to clients wrapped in shared_obj_c containers from the types package. The shared_obj_c provides reference counting semantics similar to standard shared pointers but with a different implementation optimized for size and specific use cases within this system.

When a client receives a shared_obj_c containing a store wrapper, they can freely copy that object to create additional references to the same underlying store. All references point to the same store instance and see the same data. The reference count tracks how many shared_obj_c objects currently reference the store.

When a shared_obj_c is destroyed, it decrements the reference count of the underlying object. When the reference count reaches zero, meaning no more references exist, the shared_obj_c automatically deletes the wrapped object. This triggers the wrapper's destructor which in turn triggers the lifetime notification described in the previous section.

This automatic memory management means clients do not need to explicitly close or release stores. They simply hold references through shared_obj_c objects for as long as they need access to the store, and cleanup happens automatically when the last reference is released. Multiple components can safely share references to the same store without coordination about who is responsible for cleanup.

## Usage Patterns

### Requesting a Store

Clients begin by creating or obtaining access to a distributor instance, providing a base filesystem path if disk stores will be used. To obtain a store, the client calls the distributor's get_or_create method with a unique identifier string and specifies whether they want a memory or disk backend.

The method returns an optional containing either a shared store reference or no value if store creation failed. The failure case is rare but can occur if filesystem operations fail for disk stores or if store initialization encounters problems.

Once obtained, the store reference can be copied and shared freely. All copies reference the same underlying store instance.

### Performing Read Operations

The store interface provides several read operations. The get method retrieves a value for a given key, returning a boolean indicating success and writing the value to an output parameter. The exists method simply checks whether a key is present without retrieving its value. Both methods are thread-safe and can be called concurrently with other operations.

The iterate method provides prefix-based traversal of stored keys. The caller provides a prefix string and a callback function. The store invokes the callback for each key-value pair whose key starts with the specified prefix, in lexicographic order. The callback returns a boolean indicating whether iteration should continue. This allows early termination when the desired entry is found or when enough results have been collected.

Iteration with an empty prefix traverses the entire store. The iteration is atomic from the caller's perspective, providing a consistent view of the data even if concurrent writes are occurring.

### Performing Write Operations

The set method writes or updates a single key-value pair. If the key already exists, its value is replaced. The del method removes a key and its associated value from the store. Both operations return a boolean indicating success.

For writing multiple key-value pairs, the set_batch method provides a more efficient alternative to multiple individual set calls. It accepts a map of key-value pairs and writes them all atomically. For disk stores, this leverages RocksDB's batch write capabilities for better performance. For memory stores, it simply applies the writes sequentially under a single mutex lock.

All write operations are thread-safe and can be performed concurrently with reads and other writes. The underlying stores ensure proper synchronization.

### Atomic Write Operations

The kvds package provides atomic operations that enable safe coordination and synchronization patterns across concurrent processes accessing the same store.

The set_nx method performs an atomic set-if-not-exists operation. It attempts to write a key-value pair only if the key does not already exist in the store. If the key exists, the operation fails and returns false without modifying the existing value. If the key does not exist, it is created with the provided value and the operation returns true. This primitive is essential for implementing distributed locking mechanisms, generating unique identifiers, and coordinating resource allocation across multiple processes.

The compare_and_swap method performs an atomic compare-and-swap operation. It accepts a key, an expected value, and a new value. The operation succeeds only if the key exists and currently contains the expected value, in which case it atomically updates the value to the new value and returns true. If the key does not exist or contains a different value than expected, the operation fails and returns false without modification. This primitive enables optimistic concurrency control, version tracking, and lock-free data structure implementations.

Both atomic operations maintain strong consistency guarantees within a single store instance. For memory stores, atomicity is ensured through mutex protection that spans the entire check-and-modify sequence. For disk stores, the current implementation uses a read-then-write approach which provides basic atomicity for single-process scenarios but may exhibit race conditions under high-concurrency multi-process access. Future enhancements may leverage RocksDB's transaction API for stronger guarantees in distributed environments.

Applications should use these atomic primitives when coordination between concurrent processes is required. The record package, for example, uses set_nx to implement its distributed locking mechanism, ensuring only one process can acquire a lock on a given record instance at a time.

### Managing Store Lifecycle

Store lifecycle is managed primarily through reference counting rather than explicit open and close operations. When the distributor creates a new store instance, it opens the store before returning it to the caller. The store remains open as long as references exist.

When the last reference to a store is released, the wrapper is destroyed and its destructor performs cleanup. For disk stores, this ensures the RocksDB database is properly closed and resources are released. For memory stores, this ensures the map data structure is destroyed and memory is freed.

Clients can check if a store is still open by calling the is_open method, though in normal usage this should always return true as long as the client holds a valid reference. Most operations check the open state internally and fail safely if called on a closed store.

## Key Features

### Thread Safety

All store implementations provide thread-safe operations through internal synchronization. The memory store uses a mutex to protect its internal map structure. The disk store relies on RocksDB's internal concurrency control. Clients can safely access the same store instance from multiple threads without external synchronization.

The distributor itself is also thread-safe. Multiple threads can concurrently request stores, and the distributor ensures only one instance is created for each unique identifier even if multiple threads request it simultaneously.

### Prefix-Based Iteration

The iteration capability supports efficient traversal of keys sharing a common prefix. This is particularly useful for hierarchical key naming schemes or when keys are logically grouped. The implementation leverages the underlying ordered storage in both backends to provide efficient prefix scans without needing to examine every key in the store.

For the memory store, iteration uses the standard map's lower_bound operation to jump directly to the first key with the given prefix. For the disk store, RocksDB's iterator seek capability provides similar efficiency.

### Batch Write Operations

The batch write capability allows multiple key-value pairs to be written in a single operation. This is more efficient than individual writes and provides atomicity guarantees. Either all writes in the batch succeed or none do.

For disk stores backed by RocksDB, batch writes are implemented using RocksDB's WriteBatch facility which provides true atomic batch semantics with minimal overhead. For memory stores, batch writes are applied sequentially under a single lock acquisition, providing atomicity within the context of the in-memory store.

### Automatic Resource Cleanup

The integration of lifetime tracking and reference counting ensures resources are automatically released when no longer needed. Clients do not need to explicitly close stores or worry about resource leaks. When the last reference to a store is released, all associated resources including file handles, memory buffers, and internal data structures are automatically cleaned up.

The distributor receives notification when store wrappers are destroyed, allowing it to maintain accurate registry state and potentially perform additional cleanup operations. This architecture ensures predictable resource management without requiring manual cleanup code throughout the application.
