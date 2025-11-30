# Event kernel

THe event kernel on initialization should setup and configure an event manager. use a default thread count and max capacatiy

The events will be subscribed to via lambdas. example:


```

#(load "io" "event")

(def sub_id (event/subscribe someTopicSymbolNotString (fn (event :any) :any [
    (io/put "Got event %s\n" event)
])))

; publish takes :symboil :any 
; we evaluate the param so we quote ensuring direct list made;
; publisher will publish raw object to event to be handled
(event/publish someTopicSymbolNotString '(1 2 3))

(event/unsubscribe sub_id)
; not unsubbed wont get any events



```