# Vision: SXS Distributed Runtime Infrastructure


`Note:` I described what I was working on to a claude and it made this. The goal of the project is pretty open ended and is mostly an exercise in modularity and a way for
me to scratch some itches i have. Below is the "vision" conceptualized into overly bubbly ai speech but it does a good job describing the overall intent im working toward.

## The Core Concept

SXS is a programmable distributed runtime system where individual instances federate into a managed infrastructure. Each instance transforms its capabilities dynamically by loading different kernels - no redeployment, no restart, no orchestration platform required.

The fundamental insight: **runtime polymorphism through kernel loading**. An HTTP frontend becomes a database replica, a compute worker, or a message queue by loading different capability sets. The instance itself is just a runtime; the kernels define what it can do.

## Architectural Philosophy

### Runtime as Infrastructure Primitive

Traditional infrastructure separates concerns: web servers, databases, caches, workers. Each requires different software, different configuration, different deployment processes. SXS collapses this complexity into a single runtime that adopts different personalities through kernel loading.

An instance running `sup` is not a "web server" or a "database server" - it's a programmable runtime that can become either, or both.

### Code as Persistent Data

Instead of deploying code to filesystems, SXS stores executable code in RocksDB. HTTP endpoints, request handlers, background jobs - all live in the database as executable SLP expressions. Update behavior by writing to the database, not by pushing files and restarting processes.

This inverts the traditional deployment model. Code doesn't live in version control and get deployed to servers. Code lives in the runtime's persistent store and evolves there. Version control tracks schemas and infrastructure configuration; the runtime tracks executable behavior.

### Federation Without Orchestration

Instances connect to each other directly via identity credentials and backchannel networking. No central orchestrator, no service mesh, no control plane dictating behavior. Each node knows its role, knows its peers, and coordinates through direct communication.

The "control plane" is just another node - one that happens to have visibility into the cluster and can issue commands. It's not architecturally special; it's functionally distinguished.

## The Problem Space

Managing distributed infrastructure typically requires:
- Multiple specialized services (nginx, postgres, redis, etc.)
- Container orchestration platforms (kubernetes, nomad, etc.)
- Service discovery and configuration management
- Complex deployment pipelines
- Separate development and production environments

For operators running multiple user-facing sites, APIs, or access points, this complexity compounds. Each service needs its own deployment pipeline, its own scaling strategy, its own monitoring stack.

SXS aims to collapse this complexity by providing a single runtime that:
- Serves HTTP endpoints defined in the database
- Processes requests with code stored as data
- Transforms capabilities by loading kernels
- Federates instances without orchestration platforms
- Updates behavior without deployment cycles

## Key Concepts

### Kernels as Capability Sets

A kernel is not just a library or an FFI binding. It's a capability extension that adds new primitives to the language runtime. Loading the `http` kernel doesn't give you HTTP client functions - it makes the instance capable of serving HTTP. Loading the `database` kernel doesn't give you a database client - it turns the instance into a database node.

This enables runtime polymorphism. An instance with kernels `[http, routing, auth]` is a web frontend. The same instance with kernels `[database, replication, query]` is a database node. Change kernels, change capabilities, no restart required.

Project-specific kernels extend this further. A project can define custom kernels that represent domain-specific capabilities. A game server project might have kernels for `physics`, `rendering`, `networking`. A financial platform might have `ledger`, `settlement`, `risk`. The runtime becomes whatever the project needs it to be.

### Federation Model

**Root Node:**
A standalone instance representing a single logical entity. Can be a personal server, a development environment, or a self-contained service. Has no dependencies on other instances but can accept connections from network nodes.

**Network Node:**
An instance that joins an existing infrastructure by connecting to a root node or other network nodes. Authenticates via identity files and establishes backchannel communication. Extends the distributed environment rather than creating a new one.

**Backchannel:**
Private networking between instances for state synchronization, distributed queries, and command propagation. Separate from public-facing HTTP interfaces. Enables coordinated behavior without exposing internal communication.

### Dynamic Behavior Through Persistent Storage

Traditional model:
```
Code in git → CI/CD pipeline → Deploy to servers → Restart processes
```

SXS model:
```
Define behavior in database → Runtime executes immediately → Update persists
```

HTTP endpoints, request handlers, background jobs - all defined as SLP expressions stored in RocksDB. The filesystem contains the runtime binary and kernel libraries. Everything else lives in the database.

This enables:
- Zero-downtime updates (write new code, runtime executes it)
- A/B testing (route requests to different code paths in the database)
- Rollback by database transaction (revert to previous code version)
- Multi-tenant isolation (per-tenant code stored in tenant-specific keys)

## Vision for Usage

### Distributed Web Infrastructure

Deploy `sup` instances across multiple VPS machines:
- 2 instances running HTTP + routing kernels (web frontends)
- 1 instance running database + persistence kernels (data layer)
- Backchannel connects all three for state synchronization

Define HTTP endpoints in the database. Route traffic based on load. Need more capacity? Spin up another instance, load the HTTP kernel, it joins the cluster automatically. Need a database replica? Take one HTTP node, unload HTTP kernel, load database kernel, point it at primary. Same instance, different purpose.

### Multi-Tenant API Platform

Each tenant gets isolated code paths stored under tenant-specific database keys. A single instance serves multiple tenants, routing requests based on authentication. Add new tenants by writing their code to the database - no deployment, no restart.

Scale by adding instances. Distribute load by routing tenant requests across the cluster. Update tenant code independently without affecting others. The platform is the runtime; tenants are just different code paths in the database.

### Edge Computing

Deploy `sup` instances at different geographical locations. Each runs the same kernels but serves local traffic. Code updates propagate through the backchannel - write once, deployed everywhere. Transform node purpose based on regional load patterns by loading different kernel combinations.

### Personal Infrastructure Orchestration

Run multiple instances representing different services: a web frontend, an API backend, a data processing worker. All managed from a single control node. Update all instances by pushing kernel changes or code updates through the control plane. Your personal infrastructure fleet, no cloud provider required.

## Building Toward This Vision

### Current Foundation

The interpreter provides a minimal language runtime with:
- Symbol definition and scoping (`def`, `fn`)
- Import system for code modularity
- Kernel loading for C++ extensions
- Binary serialization of s-expressions
- Smart pointer memory management

The project system (`sup`) enables:
- Custom kernel development per-project
- Automatic kernel building and caching
- Module organization for code structure
- Dependency tracking and management

Standard library kernels demonstrate extensibility:
- `alu`: Arithmetic operations
- `io`: Input/output primitives
- `kv`: Key-value storage (RocksDB)
- `random`: Random number generation

### Next Steps

**HTTP Server Kernel:**
Turn instances into web servers capable of serving requests. Define routes as code stored in the database. Parse requests, execute handlers, return responses. This is the foundation for web infrastructure use cases.

**Networking and Federation:**
Enable instances to discover and communicate with each other. Implement backchannel protocols for state synchronization and command propagation. Handle identity, authentication, and secure communication between nodes.

**Cluster Management:**
Build control plane capabilities for managing distributed instances. Query cluster state, distribute commands, monitor health, balance load. Enable the root node to orchestrate network nodes without heavyweight orchestration platforms.

**Dynamic Routing:**
Implement load balancing and request distribution across multiple instances. Route based on load, tenant, geography, or custom rules. Enable horizontal scaling by adding instances that automatically join the routing mesh.

### Future Capabilities

**Distributed Queries:**
Execute database queries across federated instances. Aggregate results, handle failures, maintain consistency. Enable applications to treat the cluster as a single logical database even when data is distributed.

**Live Kernel Swapping:**
Hot-swap kernels in running instances without restart. Transform an HTTP frontend into a database replica by unloading one set of kernels and loading another. Instance identity persists; capabilities change.

**Persistent Workflows:**
Define multi-step processes that survive instance restarts. Store workflow state in the database, resume execution after failures. Enable long-running operations without requiring dedicated orchestration systems.

**Event-Driven Architecture:**
Implement pub/sub messaging between instances. Define event handlers as code in the database. React to state changes across the distributed system without manual coordination.

## The End Goal

A runtime infrastructure system that collapses the complexity of distributed computing into a programmable substrate. Operators manage capabilities (kernels), define behavior (code in database), and coordinate instances (federation) without navigating the maze of specialized services and orchestration platforms.

An instance of `sup` is not a web server, not a database, not a worker - it's all of them, or none of them, depending on which kernels it loads and what code lives in its database.

The system becomes what you need it to be. The runtime is the infrastructure.

