#include "test.h"

static void
run_test(jit_state_t *j, uint8_t *arena_base, size_t arena_size)
{
  jit_begin(j, arena_base, arena_size);
  jit_load_args_1(j, jit_operand_gpr (JIT_OPERAND_ABI_INTMAX, JIT_R0));

  jit_rshi_u(j, JIT_R0, JIT_R0, 31);
  jit_retr(j, JIT_R0);

  intmax_t (*f)(intmax_t) = jit_end(j, NULL);

#if __WORDSIZE == 32
  ASSERT(f(0x80000000) == 1);
#else
  ASSERT(f(0x80000000) == 1);
  ASSERT(f(0x8000000000000000) == 0x100000000);
#endif
}

int
main (int argc, char *argv[])
{
  return main_helper(argc, argv, run_test);
}
