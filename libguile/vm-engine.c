/* Copyright (C) 2001, 2009, 2010, 2011, 2012, 2013 Free Software Foundation, Inc.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 3 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301 USA
 */

/* This file is included in vm.c multiple times */

#if (VM_ENGINE == SCM_VM_REGULAR_ENGINE)
# define VM_USE_HOOKS		0	/* Various hooks */
#elif (VM_ENGINE == SCM_VM_DEBUG_ENGINE)
# define VM_USE_HOOKS		1
#else
# error unknown debug engine VM_ENGINE
#endif

/* Assign some registers by hand.  There used to be a bigger list here,
   but it was never tested, and in the case of x86-32, was a source of
   compilation failures.  It can be revived if it's useful, but my naive
   hope is that simply annotating the locals with "register" will be a
   sufficient hint to the compiler.  */
#ifdef __GNUC__
# if defined __x86_64__
/* GCC 4.6 chooses %rbp for IP_REG and %rbx for SP_REG, which works
   well.  Tell it to keep the jump table in a r12, which is
   callee-saved.  */
#  define JT_REG asm ("r12")
# endif
#endif

#ifndef IP_REG
# define IP_REG
#endif
#ifndef SP_REG
# define SP_REG
#endif
#ifndef FP_REG
# define FP_REG
#endif
#ifndef JT_REG
# define JT_REG
#endif

#define VM_ASSERT(condition, handler)           \
  do {                                          \
    if (SCM_UNLIKELY (!(condition)))            \
      {                                         \
        SYNC_ALL();                             \
        handler;                                \
      }                                         \
  } while (0)

#ifdef VM_ENABLE_ASSERTIONS
# define ASSERT(condition) VM_ASSERT (condition, abort())
#else
# define ASSERT(condition)
#endif

#if VM_USE_HOOKS
#define RUN_HOOK(h, args, n)                            \
  do {                                                  \
    if (SCM_UNLIKELY (vp->trace_level > 0))             \
      {                                                 \
        SYNC_REGISTER ();				\
        vm_dispatch_hook (vm, h, args, n);              \
      }                                                 \
  } while (0)
#else
#define RUN_HOOK(h, args, n)
#endif
#define RUN_HOOK0(h) RUN_HOOK(h, NULL, 0)

#define APPLY_HOOK()                            \
  RUN_HOOK0 (SCM_VM_APPLY_HOOK)
#define PUSH_CONTINUATION_HOOK()                \
  RUN_HOOK0 (SCM_VM_PUSH_CONTINUATION_HOOK)
#define POP_CONTINUATION_HOOK(vals, n)  \
  RUN_HOOK (SCM_VM_POP_CONTINUATION_HOOK, vals, n)
#define NEXT_HOOK()                             \
  RUN_HOOK0 (SCM_VM_NEXT_HOOK)
#define ABORT_CONTINUATION_HOOK(vals, n)        \
  RUN_HOOK (SCM_VM_ABORT_CONTINUATION_HOOK, vals, n)
#define RESTORE_CONTINUATION_HOOK()            \
  RUN_HOOK0 (SCM_VM_RESTORE_CONTINUATION_HOOK)

#define VM_HANDLE_INTERRUPTS                     \
  SCM_ASYNC_TICK_WITH_CODE (current_thread, SYNC_REGISTER ())




/* Cache the VM's instruction, stack, and frame pointer in local variables.  */
#define CACHE_REGISTER()			\
{						\
  ip = vp->ip;					\
  sp = vp->sp;					\
  fp = vp->fp;					\
}

/* Update the registers in VP, a pointer to the current VM.  This must be done
   at least before any GC invocation so that `vp->sp' is up-to-date and the
   whole stack gets marked.  */
#define SYNC_REGISTER()				\
{						\
  vp->ip = ip;					\
  vp->sp = sp;					\
  vp->fp = fp;					\
}

/* FIXME */
#define ASSERT_VARIABLE(x)                                              \
  VM_ASSERT (SCM_VARIABLEP (x), abort())
#define ASSERT_BOUND_VARIABLE(x)                                        \
  VM_ASSERT (SCM_VARIABLEP (x)                                          \
             && !scm_is_eq (SCM_VARIABLE_REF (x), SCM_UNDEFINED),       \
             abort())

#ifdef VM_ENABLE_PARANOID_ASSERTIONS
#define CHECK_IP() \
  do { if (ip < bp->base || ip - bp->base > bp->len) abort (); } while (0)
#define ASSERT_ALIGNED_PROCEDURE() \
  do { if ((scm_t_bits)bp % 8) abort (); } while (0)
#define ASSERT_BOUND(x) \
  VM_ASSERT (!scm_is_eq ((x), SCM_UNDEFINED), abort())
#else
#define CHECK_IP()
#define ASSERT_ALIGNED_PROCEDURE()
#define ASSERT_BOUND(x)
#endif

/* Cache the object table and free variables.  */
#define CACHE_PROGRAM()							\
{									\
  if (bp != SCM_PROGRAM_DATA (program)) {                               \
    bp = SCM_PROGRAM_DATA (program);					\
    ASSERT_ALIGNED_PROCEDURE ();                                        \
    if (SCM_I_IS_VECTOR (SCM_PROGRAM_OBJTABLE (program))) {             \
      objects = SCM_I_VECTOR_WELTS (SCM_PROGRAM_OBJTABLE (program));    \
    } else {                                                            \
      objects = NULL;                                                   \
    }                                                                   \
  }                                                                     \
}

#define SYNC_BEFORE_GC()			\
{						\
  SYNC_REGISTER ();				\
}

#define SYNC_ALL()				\
{						\
  SYNC_REGISTER ();				\
}


/*
 * Error check
 */

/* Accesses to a program's object table.  */
#define CHECK_OBJECT(_num)
#define CHECK_FREE_VARIABLE(_num)


/*
 * Stack operation
 */

#ifdef VM_ENABLE_STACK_NULLING
# define CHECK_STACK_LEAKN(_n) ASSERT (!sp[_n]);
# define CHECK_STACK_LEAK() CHECK_STACK_LEAKN(1)
# define NULLSTACK(_n) { int __x = _n; CHECK_STACK_LEAKN (_n+1); while (__x > 0) sp[__x--] = NULL; }
/* If you have a nonlocal exit in a pre-wind proc while invoking a continuation
   inside a dynwind (phew!), the stack is fully rewound but vm_reset_stack for
   that continuation doesn't have a chance to run. It's not important on a
   semantic level, but it does mess up our stack nulling -- so this macro is to
   fix that. */
# define NULLSTACK_FOR_NONLOCAL_EXIT() if (vp->sp > sp) NULLSTACK (vp->sp - sp);
#else
# define CHECK_STACK_LEAKN(_n)
# define CHECK_STACK_LEAK()
# define NULLSTACK(_n)
# define NULLSTACK_FOR_NONLOCAL_EXIT()
#endif

/* For this check, we don't use VM_ASSERT, because that leads to a
   per-site SYNC_ALL, which is too much code growth.  The real problem
   of course is having to check for overflow all the time... */
#define CHECK_OVERFLOW()                                                \
  do { if (SCM_UNLIKELY (sp >= stack_limit)) goto handle_overflow; } while (0)

#ifdef VM_CHECK_UNDERFLOW
#define PRE_CHECK_UNDERFLOW(N)                  \
  VM_ASSERT (sp - (N) > SCM_FRAME_UPPER_ADDRESS (fp), vm_error_stack_underflow ())
#define CHECK_UNDERFLOW() PRE_CHECK_UNDERFLOW (0)
#else
#define PRE_CHECK_UNDERFLOW(N) /* nop */
#define CHECK_UNDERFLOW() /* nop */
#endif


#define PUSH(x)	do { sp++; CHECK_OVERFLOW (); *sp = x; } while (0)
#define DROP()	do { sp--; CHECK_UNDERFLOW (); NULLSTACK (1); } while (0)
#define DROPN(_n) do { sp -= (_n); CHECK_UNDERFLOW (); NULLSTACK (_n); } while (0)
#define POP(x)	do { PRE_CHECK_UNDERFLOW (1); x = *sp--; NULLSTACK (1); } while (0)
#define POP2(x,y) do { PRE_CHECK_UNDERFLOW (2); x = *sp--; y = *sp--; NULLSTACK (2); } while (0)
#define POP3(x,y,z) do { PRE_CHECK_UNDERFLOW (3); x = *sp--; y = *sp--; z = *sp--; NULLSTACK (3); } while (0)

/* Pop the N objects on top of the stack and push a list that contains
   them.  */
#define POP_LIST(n)				\
do						\
{						\
  int i;					\
  SCM l = SCM_EOL, x;				\
  SYNC_BEFORE_GC ();                            \
  for (i = n; i; i--)                           \
    {                                           \
      POP (x);                                  \
      l = scm_cons (x, l);                      \
    }                                           \
  PUSH (l);					\
} while (0)

/* The opposite: push all of the elements in L onto the list. */
#define PUSH_LIST(l, NILP)			\
do						\
{						\
  for (; scm_is_pair (l); l = SCM_CDR (l))      \
    PUSH (SCM_CAR (l));                         \
  VM_ASSERT (NILP (l), vm_error_improper_list (l)); \
} while (0)


/*
 * Instruction operation
 */

#define FETCH()		(*ip++)
#define FETCH_LENGTH(len) do { len=*ip++; len<<=8; len+=*ip++; len<<=8; len+=*ip++; } while (0)

#undef NEXT_JUMP
#ifdef HAVE_LABELS_AS_VALUES
# define NEXT_JUMP()		goto *jump_table[FETCH () & SCM_VM_INSTRUCTION_MASK]
#else
# define NEXT_JUMP()		goto vm_start
#endif

#define NEXT					\
{						\
  NEXT_HOOK ();					\
  CHECK_STACK_LEAK ();                          \
  NEXT_JUMP ();					\
}


/* See frames.h for the layout of stack frames */
/* When this is called, bp points to the new program data,
   and the arguments are already on the stack */
#define DROP_FRAME()                            \
  {                                             \
    sp -= 3;                                    \
    NULLSTACK (3);                              \
    CHECK_UNDERFLOW ();                         \
  }
    

static SCM
VM_NAME (SCM vm, SCM program, SCM *argv, int nargs)
{
  /* VM registers */
  register scm_t_uint8 *ip IP_REG;	/* instruction pointer */
  register SCM *sp SP_REG;		/* stack pointer */
  register SCM *fp FP_REG;		/* frame pointer */
  struct scm_vm *vp = SCM_VM_DATA (vm);

  /* Cache variables */
  struct scm_objcode *bp = NULL;	/* program base pointer */
  SCM *objects = NULL;			/* constant objects */
  SCM *stack_limit = vp->stack_limit;	/* stack limit address */

  scm_i_thread *current_thread = SCM_I_CURRENT_THREAD;

  /* Internal variables */
  int nvalues = 0;
  scm_i_jmp_buf registers;              /* used for prompts */

#ifdef HAVE_LABELS_AS_VALUES
  static const void **jump_table_pointer = NULL;
#endif

#ifdef HAVE_LABELS_AS_VALUES
  register const void **jump_table JT_REG;

  if (SCM_UNLIKELY (!jump_table_pointer))
    {
      int i;
      jump_table_pointer = malloc (SCM_VM_NUM_INSTRUCTIONS * sizeof (void*));
      for (i = 0; i < SCM_VM_NUM_INSTRUCTIONS; i++)
        jump_table_pointer[i] = &&vm_error_bad_instruction;
#define VM_INSTRUCTION_TO_LABEL 1
#define jump_table jump_table_pointer
#include <libguile/vm-expand.h>
#include <libguile/vm-i-system.i>
#include <libguile/vm-i-scheme.i>
#include <libguile/vm-i-loader.i>
#undef jump_table
#undef VM_INSTRUCTION_TO_LABEL
    }

  /* Attempt to keep JUMP_TABLE_POINTER in a register.  This saves one
     load instruction at each instruction dispatch.  */
  jump_table = jump_table_pointer;
#endif

  if (SCM_I_SETJMP (registers))
    {
      /* Non-local return.  Cache the VM registers back from the vp, and
         go to the handler.

         Note, at this point, we must assume that any variable local to
         vm_engine that can be assigned *has* been assigned. So we need to pull
         all our state back from the ip/fp/sp.
      */
      CACHE_REGISTER ();
      program = SCM_FRAME_PROGRAM (fp);
      CACHE_PROGRAM ();
      /* The stack contains the values returned to this continuation,
         along with a number-of-values marker -- like an MV return. */
      ABORT_CONTINUATION_HOOK (sp - SCM_I_INUM (*sp), SCM_I_INUM (*sp));
      NEXT;
    }

  CACHE_REGISTER ();

  /* Since it's possible to receive the arguments on the stack itself,
     and indeed the RTL VM invokes us that way, shuffle up the
     arguments first.  */
  VM_ASSERT (sp + 8 + nargs < stack_limit, vm_error_too_many_args (nargs));
  {
    int i;
    for (i = nargs - 1; i >= 0; i--)
      sp[9 + i] = argv[i];
  }

  /* Initial frame */
  PUSH (SCM_PACK (fp)); /* dynamic link */
  PUSH (SCM_PACK (0)); /* mvra */
  PUSH (SCM_PACK (ip)); /* ra */
  PUSH (boot_continuation);
  fp = sp + 1;
  ip = SCM_C_OBJCODE_BASE (SCM_PROGRAM_DATA (boot_continuation));

  /* MV-call frame, function & arguments */
  PUSH (SCM_PACK (fp)); /* dynamic link */
  PUSH (SCM_PACK (ip + 1)); /* mvra */
  PUSH (SCM_PACK (ip)); /* ra */
  PUSH (program);
  fp = sp + 1;
  sp += nargs;

  PUSH_CONTINUATION_HOOK ();

 apply:
  program = fp[-1];
  if (!SCM_PROGRAM_P (program))
    {
      if (SCM_STRUCTP (program) && SCM_STRUCT_APPLICABLE_P (program))
        fp[-1] = SCM_STRUCT_PROCEDURE (program);
      else if (SCM_HAS_TYP7 (program, scm_tc7_smob)
               && SCM_SMOB_APPLICABLE_P (program))
        {
          /* (smob arg0 ... argN) => (apply-smob smob arg0 ... argN) */
          int i;
          PUSH (SCM_BOOL_F);
          for (i = sp - fp; i >= 0; i--)
            fp[i] = fp[i - 1];
          fp[-1] = SCM_SMOB_DESCRIPTOR (program).apply_trampoline;
        }
      else
        {
          SYNC_ALL();
          vm_error_wrong_type_apply (program);
        }
      goto apply;
    }

  CACHE_PROGRAM ();
  ip = SCM_C_OBJCODE_BASE (bp);

  APPLY_HOOK ();

  /* Let's go! */
  NEXT;

#ifndef HAVE_LABELS_AS_VALUES
 vm_start:
  switch ((*ip++) & SCM_VM_INSTRUCTION_MASK) {
#endif

#include "vm-expand.h"
#include "vm-i-system.c"
#include "vm-i-scheme.c"
#include "vm-i-loader.c"

#ifndef HAVE_LABELS_AS_VALUES
  default:
    goto vm_error_bad_instruction;
  }
#endif

  abort (); /* never reached */

 vm_error_bad_instruction:
  vm_error_bad_instruction (ip[-1]);
  abort (); /* never reached */

 handle_overflow:
  SYNC_ALL ();
  vm_error_stack_overflow (vp);
  abort (); /* never reached */
}

#undef ALIGNED_P
#undef CACHE_REGISTER
#undef CHECK_OVERFLOW
#undef FREE_VARIABLE_REF
#undef FUNC2
#undef INIT
#undef INUM_MAX
#undef INUM_MIN
#undef jump_table
#undef LOCAL_REF
#undef LOCAL_SET
#undef NEXT
#undef NEXT_JUMP
#undef REL
#undef RETURN
#undef RETURN_ONE_VALUE
#undef RETURN_VALUE_LIST
#undef RUN_HOOK
#undef RUN_HOOK1
#undef SYNC_ALL
#undef SYNC_BEFORE_GC
#undef SYNC_IP
#undef SYNC_REGISTER
#undef VARIABLE_BOUNDP
#undef VARIABLE_REF
#undef VARIABLE_SET
#undef VM_DEFINE_OP
#undef VM_INSTRUCTION_TO_LABEL
#undef VM_USE_HOOKS

/*
  Local Variables:
  c-file-style: "gnu"
  End:
*/
