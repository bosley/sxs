# SLP Parser State Machine

```mermaid
stateDiagram-v2
    [*] --> START
    
    START --> CHECK_NEWLINE: Begin processing
    
    CHECK_NEWLINE --> CLOSE_VIRTUAL: Newline at depth 0
    CHECK_NEWLINE --> SKIP_WHITESPACE: Continue
    CLOSE_VIRTUAL --> CHECK_NEWLINE: Virtual list closed
    
    SKIP_WHITESPACE --> READ_TOKEN: Non-whitespace found
    SKIP_WHITESPACE --> END: Buffer exhausted
    
    READ_TOKEN --> PROCESS_LIST_P: '('
    READ_TOKEN --> PROCESS_LIST_B: '['
    READ_TOKEN --> PROCESS_LIST_C: '{'
    READ_TOKEN --> PROCESS_LIST_S: '"'
    READ_TOKEN --> PROCESS_QUOTE: "'"
    READ_TOKEN --> OPEN_VIRTUAL: Other token at depth 0
    READ_TOKEN --> PARSE_ATOM: Other token at depth > 0
    
    OPEN_VIRTUAL --> PARSE_ATOM: Virtual list opened
    
    PROCESS_LIST_P --> RECURSE: Find ')'
    PROCESS_LIST_B --> RECURSE: Find ']'
    PROCESS_LIST_C --> RECURSE: Find '}'
    PROCESS_LIST_S --> RECURSE: Find '"'
    
    RECURSE --> EMIT_LIST: Process contents recursively
    RECURSE --> ERROR: Unclosed delimiter
    
    PROCESS_QUOTE --> SKIP_WS_QUOTE: Skip whitespace
    SKIP_WS_QUOTE --> QUOTE_LIST: Next is list delimiter
    SKIP_WS_QUOTE --> QUOTE_ATOM: Next is atom
    
    QUOTE_LIST --> EMIT_QUOTED: Capture quoted list
    QUOTE_ATOM --> EMIT_QUOTED: Capture quoted atom
    
    PARSE_ATOM --> PARSE_INTEGER: Integer pattern
    PARSE_ATOM --> PARSE_REAL: Real pattern
    PARSE_ATOM --> PARSE_SYMBOL: Symbol pattern
    PARSE_ATOM --> ERROR: Invalid token
    
    PARSE_INTEGER --> EMIT_OBJECT: Convert to int64
    PARSE_REAL --> EMIT_OBJECT: Convert to double
    PARSE_SYMBOL --> EMIT_OBJECT: Store as buffer
    
    EMIT_LIST --> CHECK_NEWLINE: Continue
    EMIT_QUOTED --> CHECK_NEWLINE: Continue
    EMIT_OBJECT --> CHECK_NEWLINE: Continue
    
    ERROR --> CLOSE_VIRTUAL_ON_ERROR: At depth 0
    ERROR --> END: At depth > 0
    CLOSE_VIRTUAL_ON_ERROR --> END: Parse failed
    
    END --> [*]
    
    note right of RECURSE
        Lists are processed
        recursively with
        depth tracking
    end note
    
    note right of PROCESS_QUOTE
        Quote operator captures
        next token without
        evaluating it
    end note
    
    note right of CHECK_NEWLINE
        Virtual lists wrap
        top-level expressions
        terminated by newlines
    end note
```

## Key Concepts

**Virtual Lists**: At depth 0, expressions not wrapped in explicit delimiters are automatically wrapped in virtual parentheses, terminated by newlines.

**Recursive Processing**: List delimiters trigger recursive descent into nested content, maintaining depth tracking.

**Quote Operator**: The `'` operator captures the next token (list or atom) as literal data without processing.

**Base Types**: Atoms are classified as integers, reals, or symbols based on their lexical pattern.

**Error Handling**: Parse errors (unclosed delimiters, invalid tokens) transition to ERROR state and halt processing.

