# janet-jcjit

A jit C compiler for janet, built using libtcc.

Functions are compiled directly in memory, they are never written to disk.

The janet header is embedded in the binary, so janet headers should not be needed.

# Install

Requires tiny c compiler at build time and libcc at runtime, then 'jpm install' as usual.

# Quick example

```
(import /jcjit :as jit)

(def myfn 
  (jit/compile `

    Janet f(int argc, Janet *argv) {
      int i = janet_getinteger(argv, 0);
      return janet_wrap_integer(i+1);
    }

  `))

(assert (= (jit/call myfn 3) 4))
```

# Limitations

- The Janet headers are embedded preprocessed, so they don't use macro versions
  of the janet api.


# TODO

- We should deal with the tcc error callbacks properly, instead of just printing
  to stderr.
- When a jit function is gc'd, we should set the memory to non executable again.