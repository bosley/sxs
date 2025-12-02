# Next (Language)

Once i finish this fucking refactoring i want to get started on the byte generator.
The plan is to follow the pattern i established with typechecking and interpreting to call into
the compiler context and push up "micro programs"

If we assume any given object can be considered a "program" to be executed, then base types would be a function
call to "make object." then things like `[() () ()..]` would translate to a list of programs to run. we translate this
to MLL RISC language for SSA etc. 

Might just run on vm from there, but its a good candidate to translate to actual backend

# Next (Application)

I started this project to build out a dynamic framework for a project i wanted to build. I didn't know it would
drift into being a compiler/interpretr but thats the way it goes

I need to decide if I want to actually start doing code gen or keep tree walking/dylib to build out the app. 
Im leaning towards that then letting compilation happen later but at the same time I REALLY WANT TO BUILD A COMPILER lmao fuckin clearly