// #ifdef PLUGINS_NEW


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "../plugins.h"

typedef struct {
        uint64_t total_instr;
        uint64_t total_long;
        uint64_t total_expensive;

} thread_data_t;

//Forward declarations added
int dep_chain_pre_thread(mambo_context *ctx);
int dep_chain_post_thread(mambo_context *ctx);
int dep_chain_pre_inst(mambo_context *ctx);

int dependency_checker_pre_thread(mambo_context *ctx){

    fprintf(stderr, "dependency_checker: pre thread callback\n");
    thread_data_t *t_data = (thread_data_t *)mambo_alloc(ctx, sizeof(thread_data_t));

    assert(t_data != NULL);
    memset(t_data, 0, sizeof(thread_data_t));

    int ret = mambo_set_thread_plugin_data(ctx,t_data);
    assert(ret == MAMBO_SUCCESS);

    fprintf(stderr, "[dep_chain] thread %d started — data allocated at %p\n",
        mambo_get_thread_id(ctx), (void *)t_data);

    return 0;
}

int dependency_checker_post_thread(mambo_context *ctx){

   fprintf(stderr, "dependency_checker: post thread callback\n");

   thread_data_t *t_data = (thread_data_t *)mambo_get_thread_plugin_data(ctx);
   assert(t_data != NULL);

   fprintf(stderr,
        "[dep_chain] thread %d exited — "
        "total=%"PRIu64" long=%"PRIu64" expensive=%"PRIu64"\n",
        mambo_get_thread_id(ctx),
        t_data->total_instr,
        t_data->total_long,
        t_data->total_expensive);


return 0;

int dep_chain_pre_inst(mambo_context *ctx) {
  thread_data_t *td = (thread_data_t *)mambo_get_thread_plugin_data(ctx);
  if (td == NULL) return 0;

  void *pc = mambo_get_source_addr(ctx);
  int   inst = mambo_get_inst(ctx);

  /* ----------------------------------------------------------------
   * Classification switch — mirrors instruction_mix.c structure.
   * Uses ctx->code.inst (PIE enum) for classification.
   * No register extraction yet — that is Block 5.
   * ---------------------------------------------------------------- */
  switch (inst) {

  /* LONG: all memory load instructions */
  case RISCV_LB:
  case RISCV_LH:
  case RISCV_LW:
  case RISCV_LBU:
  case RISCV_LHU:
  case RISCV_LWU:
  case RISCV_LD:
  case RISCV_FLW:
  case RISCV_FLD:
  /* compressed loads (compressed instructions are not in the scope but handled anyways relevant for future work) */
  case RISCV_C_FLD:
  case RISCV_C_LW:
  case RISCV_C_LD:
  case RISCV_C_FLDSP:
  case RISCV_C_LWSP:
  case RISCV_C_FLWSP:
  case RISCV_C_LDSP:
    fprintf(stderr, "[dep_chain] LONG      at %p (scan time)\n", pc);
    break;

  /* EXPENSIVE: integer mul/div (M-extension) */
  case RISCV_MUL:
  case RISCV_MULH:
  case RISCV_MULHSU:
  case RISCV_MULHU:
  case RISCV_DIV:
  case RISCV_DIVU:
  case RISCV_REM:
  case RISCV_REMU:
  case RISCV_MULW:
  case RISCV_DIVW:
  case RISCV_DIVUW:
  case RISCV_REMW:
  case RISCV_REMUW:
  /* EXPENSIVE: FP mul/div */
  case RISCV_FMUL_S:
  case RISCV_FDIV_S:
  case RISCV_FMUL_D:
  case RISCV_FDIV_D:
    fprintf(stderr, "[dep_chain] EXPENSIVE at %p (scan time)\n", pc);
    break;

  default:
    break;
  }

  return 0;
}



__attribute__((constructor)) void dependency_checker_init(void) {

    mambo_context *ctx = mambo_register_plugin();
    int ret;
    ret = mambo_register_pre_thread_cb(ctx, dependency_checker_pre_thread);
    assert(ret == MAMBO_SUCCESS);


    ret = mambo_register_post_thread_cb(ctx, dependency_checker_post_thread);
    assert(ret == MAMBO_SUCCESS);


    ret = mambo_register_pre_inst_cb(ctx, dep_chain_pre_inst);
    assert(ret == MAMBO_SUCCESS);
}




// #endif /* PLUGINS_NEW */