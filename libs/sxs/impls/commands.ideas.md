Need to imageine some commands

Existing

`Note:` In order to create a `list-p` type at runtime one must leverage quoted

```
(@ 0 44)
(@ 0 44 127)
(@ 0)

(@ 7 '())
```

In this case object `7` will be of type list-p as its arguments are evaluated on call, meaning a list-p will be yield.

`$` check the "form" or "type" of the object in 0 (see forms.h and forms.c)

`($ <:-prefixed-symbol> <any>)`

```
($ 0 :none)     -> 0
($ 1 :none)     -> 1
($ 7 :list-p)   -> 1
```

`_` none runner. this will evaluate each object in the arguments and then yield a `:none`
the purpose of this is to serialize the execution of objects and leave yield a ":none"
`_` can take 0 arguments "(_)" to yield a `:none`

`(_)`

