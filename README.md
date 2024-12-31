## AnyConnect for the Command Line

### Introduction

The comment at the top of the file `acng.c` has verbose information.

### Build

To compile from scratch, execute:

```sh
cc -O2 acng.c -o acng -lexpat
strip acng
```

Then you can install it to your `PATH`.

It links to `libexpat`, so also make sure that it is installed.
