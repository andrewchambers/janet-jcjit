# janet-jcjit

A C extension jit compiler for janet.

Functions are compiled directly in memory, they are never written to disk.
In fact, jitted functions can be garbage collected.

The janet header is embedded in the binary, so janet headers should not be needed.

# Install

Requires tiny c compiler at build time and libtcc at runtime, then 'jpm install' as usual.

# Quick example

```
(import jcjit :as jit)

(def myfn 
  (jit/compile `

    Janet f(int argc, Janet *argv) {
      janet_fixarity(argc, 1);
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

- Use proper build rules for generated source code.