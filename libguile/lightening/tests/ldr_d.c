#include "test.h"

static double data[] = { -1.0, 0.0, 0.5 };

static void
run_test(jit_state_t *j, uint8_t *arena_base, size_t arena_size)
{
  jit_begin(j, arena_base, arena_size);
  jit_load_args_1(j, jit_operand_gpr (JIT_OPERAND_ABI_POINTER, JIT_R1));

  jit_ldr_d(j, JIT_F0, JIT_R1);
  jit_retr_d(j, JIT_F0);

  double (*f)(void*) = jit_end(j, NULL);

  ASSERT(f(&data[0]) == data[0]);
  ASSERT(f(&data[1]) == data[1]);
  ASSERT(f(&data[2]) == data[2]);
}

int
main (int argc, char *argv[])
{
  return main_helper(argc, argv, run_test);
}
