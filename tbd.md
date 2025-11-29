# TBD - Core Commands to Implement

## apply

```sxs
(def add (fn (a :int b :int) :int [(def result 0) result]))
(def args (1 2))
(apply add args)
```

```sxs
(def my-func (fn (x :int y :int z :int) :int [z]))
(apply my-func '(10 20 30))
```

## try

try handles the event of an SLP Error, not a system THROW. there is a big difference

```sxs
(def risky (fn () :int [@(something went wrong)]))
(try (risky) [
  (debug "Error caught:" $error)
  0
])
```

```sxs
(try 
  [(def x (some-undefined-func))] 
  42
)
```

```sxs
(try
  [(def result (compute-thing))]
  [
    (io/println "Failed with error:" $error)
    -1
  ]
)
```

## assert

- THROWS does NOT return error

```sxs
(assert 1 "this passes")
(assert 0 "this fails")
```

```sxs
(def x 42)
(assert (if x 1 0) "x must be non-zero")
```

```sxs
(def validate (fn (val :int) :int [
  (assert (if val 1 0) "value cannot be zero")
  val
]))
```

## recover

- used in-tandem with asserts. 

```sxs
(recover [

    ... 
    (assert 0 "oops")


] [
  (io/put "got an exception!" $exception ) ; $exception is ephemeral symbol that will return a string
])
```

## match

```sxs
(def x 2.14)
(match x
  (:real x "is a real")
  (:int x "is an int")
  (:str x "is a string")
)
```

```sxs
(def value "2.14")
(match value
  (:str value "matched string 2.14")
  (:real 2.14 "matched real 2.14")
)
```

```sxs
(def process (fn (input :aberrant) :str [
  (match input
    (:int input "got integer")
    (:real input "got real")
    (:str input "got string")
    (:aberrant input "got function")
  )
]))
```

## eval

**Note:** eval should NOT permit datum lists - the system will already fail if they run in the interpreter as they will be locked (kernels and imports) we need to ensure that eval is also "Scoped" so any defs in an eval string stay in an eval string, only the result of the evaluation should be present by-way of return

```sxs
(def code "(def x 42)")
(eval code)
(debug x)
```

```sxs
(def expr "(if 1 100 200)")
(def result (eval expr))
```

```sxs
(def make-code (fn (val :int) :str [
  (def result "(def dynamic-var ")
  result
]))
(eval (make-code 99))
```
