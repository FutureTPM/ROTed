## Initialize

Before doing anything you need to initialize the repo by running:

```
./init-repo.sh
```

## Install

The code has the GMP, MPFR and CUnit libraries as depedencies. The following commands might be used to generate makefiles on Linux

```
  cmake -DCMAKE_BUILD_TYPE=Release -H. -B_builds -G "Unix Makefiles"
```

and build the code

```
  cmake --build _builds/
```

Variations to build the code on other systems should be available by consulting the manpages of cmake and changing the "-G" flag accordingly.
