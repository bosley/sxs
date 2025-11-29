/*
sup is "sxs up"


it is made to manage the installation and packages on the target softare


./sup --install <path>
./sup --check (checks env fo SXS_HOME). if not found indicate that path for
install should be set to this, if set, print it along iwht installed kernels/
libs


 ```


    install_dir
        - kernels
            - std
                io/
                    dylib
                    kernel.sxs

        - lib
            some_lib/
                lib.sxs // just for demo purposesn this is where non-kernel
libraroes would be (sxs sourced)



 ```



*/