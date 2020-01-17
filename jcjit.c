#include <janet.h>
#include <libtcc.h>

#include "build/prelude.E.inc"

static const JanetAbstractType function_type = {
    "jcjit.function", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};

static Janet jcjit_compile(int32_t argc, Janet *argv) {
  janet_fixarity(argc, 1);
  const uint8_t *fsrc = janet_getcstring(argv, 0);
  fsrc = janet_formatc("%s\n#line 0 \"<cjit>\"\n%s", (char *)build_prelude_E, fsrc);

  TCCState *s = tcc_new();
  if (!s)
    janet_panic("unable to create tcc state");
  if (tcc_set_output_type(s, TCC_OUTPUT_MEMORY) != 0)
    janet_panic("unable to set output type");
  int r = tcc_compile_string(s, fsrc);
  int n = tcc_relocate(s, NULL);
  uint8_t *f = janet_abstract(&function_type, n+sizeof(void*));
  tcc_relocate(s, f+sizeof(void*));
  void *p = tcc_get_symbol(s, "f");
  if (!p)
    janet_panic("jit must define a function named 'f'");
  *(void**)f = p; 
  tcc_delete(s);
  return janet_wrap_abstract(f);
}

static Janet jcjit_call(int32_t argc, Janet *argv) {
  if (argc == 0)
    janet_panic("expected at least one argument");
  void **f = janet_getabstract(argv, 0, &function_type);
  return ((Janet (*)(int32_t, Janet*))(*f))(argc-1, argv+1);
}

static const JanetReg cfuns[] = {
  {"compile", jcjit_compile, NULL},
  {"call", jcjit_call, NULL},
  {NULL, NULL, NULL}};

JANET_MODULE_ENTRY(JanetTable *env) {
  janet_cfuns(env, "jcjit", cfuns);
}