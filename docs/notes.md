

## build step sup

we have a build step in sup for kernels. we are most likely going to have special kernels that need building always.
once we get the core functions in (the ones that the core binary will always have available) implemented and interpreted
successfully we can make another "interpreter" like object that just does the type checking and symbol resolution during build
step. then, we could run a "vm" that takes the hydrated slp object raw from a .bin and executres it without all the safety checks.

The reason im not targeting this immediatly is that this is not meant to be "a language" so much as a "modular plug-and-play platform" 
centered around an entity (user) on a repl in a network/social setting.

Its a bit different in needs than a language, but for the sake of speed and performance one day I would LIKE to do this.