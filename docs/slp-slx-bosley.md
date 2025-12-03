# Forward

Over the years I've made a handful of interpreted langages and RISC virtual machines. 
I will keep the list brief and link for more details:

I fully implemented from scratch:
    - Nabla + Solace + Del   (c++, 64-bit RISC VM, assembler, and high level lanugage)
    - Sauros                 (c++, tree walking lisp with git integration for package managemnt)
    - Nibi                   (c++, best c++ tree walking lisp interpretr I've ever made. sxs stands on nibi's shoulder's)
    - Polaris                (c++, my first baby lisp i made one spring weekend between my last day at one job and my first at another)
    - Lambda VM              (rust, early -> deep winter. used to really learn rust. 64 bit RISC VM with interrupts and modules)
    - Slpx                   (go, first implementation of SLP ideation)

Ones I toyed with
    - boq           (go - modified monkey implementation)

I am now making sxs because I still want to make the programming enviornment of my dreams lmao. 

I like lisps because theres no ambiguity of of flow/structure. AS is most apparent in math: 1 + 2 -> + 1 2
It just makes more sense to me. Its kind of like this:

`()` = I am doing something

`(what_im_doing)` = what i am doing

`(what_im_doing and_what im_doing_it with)`

Thats it. Thats the bit. The coolest part is that its homoiconic meaning that the way we encode the data is the way the instruction reads to the user. For instance: C++ reads much different than the asm. In s-expressions the equivalent is saying "C++ IS ASM."
