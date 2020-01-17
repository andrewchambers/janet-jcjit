
(import ../build/jcjit :as jit)

(def myfn (jit/compile `

  Janet f(int argc, Janet *argv) {
    int i = janet_getinteger(argv, 0);
    return janet_wrap_integer(i+1);
  }

`))

(assert (= (jit/call myfn 3) 4))
(gccollect)