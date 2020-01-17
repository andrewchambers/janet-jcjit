(declare-project
  :name "cjit"
  :author "Andrew Chambers"
  :license "MIT"
  :url "https://github.com/andrewchambers/janet-jcjit"
  :repo "git+https://github.com/andrewchambers/janet-jcjit.git")

(defn- cmd-out [what]
  (def f (file/popen what))
  (def v (file/read f :all))
  (unless (zero? (file/close f))
    (error (string "command '" what "' failed!")))
  v)

(defn- lookup-prog-path
  [prog]
  (def p (string/trim (cmd-out (string "which " prog))))
  (string/trim (cmd-out (string "realpath " p))))

(def tcc-bin (lookup-prog-path "tcc"))
(def tcc-base (string/slice tcc-bin 0 -9))
(def tcc-incl-dir (string tcc-base "/include"))
(def tcc-lib-dir (string tcc-base "/lib"))

(cmd-out "mkdir -p build")
(cmd-out (string "tcc -I" JANET_HEADERPATH " -E -o build/prelude.E prelude.c"))
(spit "build/prelude.E" (string (slurp "build/prelude.E") "\x00"))
(cmd-out "xxd -i build/prelude.E build/prelude.E.inc")


(declare-native
    :name "jcjit"
    :cflags [(string "-I" tcc-incl-dir)]
    :lflags [(string "-L" tcc-lib-dir) 
             "-Wl,--rpath" tcc-lib-dir
             "-ltcc" "-ldl"]
    :source ["jcjit.c"])
