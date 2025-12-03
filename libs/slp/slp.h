#ifndef SXS_SLP_SLP_H
#define SXS_SLP_SLP_H

/*



    start -- on '(' start consuming static types until '(' or ')'

    for every type that is found in the buffer, we allocate a new buffer
   specifically for that type so we can store the object in the box

    This is where we seperate the data from code. The initial buffer in is the
   code we extracted items off of and then when found we hand back a view into
   that buffer .

    This means that we are in full controll of the memory and where the objects
   are, the scanner just views them





*/

#endif