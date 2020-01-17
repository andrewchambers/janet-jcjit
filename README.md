# janet-jcjit

A fully functional tiny jit for janet, built using libtcc.

Functions are jit compiled directly in memory, they are never
written to disk.

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