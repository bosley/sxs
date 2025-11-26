This is to markup the types of all builtin cmmands based on #lists for clarification sake


core/kv

`#[core/kv/set (:symbol :any) #int]`
`#[core/kv/get (:symbol) #any]`
`#[core/kv/del (:symbol) #int]`
`#[core/kv/exists (:symbol) #int]`
`#[core/kv/snx (:symbol :any) #int]`
`#[core/kv/cas (:symbol :any :any) #int]`
`#[core/kv/iterate (:symbol :int :int :list-c) #int]`
`#[core/kv/load (:symbol) #any]`

core/event

`#[core/event/pub (:symbol :int :any) #int]`
`#[core/event/sub (:symbol :int :symbol :list-c) #int]`

core/expr

`#[core/expr/eval (:any) #any]`
`#[core/expr/proc (:int :list-c) #int]`

core/util

`#[core/util/log (:any) #int]`
`#[core/util/insist (:list-p) #any]`
