#include <errno.h>
#include <janet.h>
#include <libtcc.h>
#include <sys/mman.h>

#include "build/prelude.E.inc"

typedef struct {
  Janet (*f)(int32_t, Janet *);
  void *mem;
  size_t n;
} JitFunc;

static int jitfunc_gc(void *p, size_t s) {
  (void)s;
  JitFunc *f = (JitFunc *)p;
  if (f->mem == MAP_FAILED || f->mem == NULL)
    return 0;
  if (munmap(f->mem, f->n) != 0)
    abort();
  return 0;
}

static Janet jitfunc_call(void *p, int32_t argc, Janet *argv) {
    JitFunc *f = p;
    return f->f(argc, argv);
}

static const JanetAbstractType function_type = {"jcjit.function",
                                                jitfunc_gc,
                                                NULL,
                                                NULL,
                                                NULL,
                                                NULL,
                                                NULL,
                                                NULL,
                                                NULL,
                                                NULL,
                                                NULL,
                                                jitfunc_call};

static void finalize_tcc_state(TCCState **s) {
  if (*s)
    tcc_delete(*s);
}

static void on_compile_error(void *opaque, const char *msg) {
  Janet *errormsg = opaque;
  *errormsg = janet_cstringv(msg);
}

static Janet jcjit_compile(int32_t argc, Janet *argv) {
  janet_fixarity(argc, 1);
  const uint8_t *fsrc = janet_getstring(argv, 0);
  fsrc = janet_formatc("%s\n#line 0 \"<cjit>\"\n%s", (char *)build_prelude_E,
                       fsrc);

  TCCState **s = janet_smalloc(sizeof(struct TCCState *));
  *s = tcc_new();
  janet_sfinalizer(s, (void (*)(void *))finalize_tcc_state);

  if (!*s)
    janet_panic("unable to create tcc state");

  Janet errormsg = janet_wrap_nil();
  tcc_set_error_func(*s, &errormsg, on_compile_error);

  if (tcc_set_output_type(*s, TCC_OUTPUT_MEMORY) != 0)
    janet_panic("unable to set output type");

  if (tcc_compile_string(*s, (const char *)fsrc) != 0)
    janet_panicf("unable to compile function: %s",
                 janet_checktype(errormsg, JANET_STRING)
                     ? (char *)janet_unwrap_string(errormsg)
                     : "unknown error");

  JitFunc *f = janet_abstract(&function_type, sizeof(JitFunc));
  f->f = NULL;
  f->n = tcc_relocate(*s, NULL);
  if (f->n <= 0)
    janet_panic("unable to relocate function 1");
  f->mem = mmap(NULL, f->n, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS,
                -1, 0);

  if (f->mem == MAP_FAILED)
    janet_panicf("unable to map memory for jit function: %s", strerror(errno));

  if (tcc_relocate(*s, f->mem) != 0)
    janet_panic("unable to relocate function 2");

  if (mprotect(f->mem, f->n, PROT_READ | PROT_EXEC) != 0)
    janet_panic("unable to remote write permission on mapped memory");

  f->f = tcc_get_symbol(*s, "f");
  if (!f->f)
    janet_panic("jit must define a function named 'f'");

  janet_sfree(s);

  return janet_wrap_abstract(f);
}

static Janet jcjit_call(int32_t argc, Janet *argv) {
  if (argc == 0)
    janet_panic("expected at least one argument");
  JitFunc *f = janet_getabstract(argv, 0, &function_type);
  return f->f(argc - 1, argv + 1);
}

static const JanetReg cfuns[] = {{"compile", jcjit_compile, NULL},
                                 {"call", jcjit_call, NULL},
                                 {NULL, NULL, NULL}};

JANET_MODULE_ENTRY(JanetTable *env) { janet_cfuns(env, "jcjit", cfuns); }
