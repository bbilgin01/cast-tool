#ifdef PLUGINS_NEW


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <inttypes.h>
#include <stdbool.h>

#include "../plugins.h"


#define WINDOW_SIZE 16
#define MAX_GAP     4

typedef enum { INST_OTHER=0, INST_LONG=1, INST_EXPENSIVE=2 } inst_class_t;

typedef struct {
  inst_class_t iclass;
  int          dest_reg;
  int          src_reg[2];
  uintptr_t    pc;
  char         text[80];
} inst_info_t;

/*per-thread data*/
typedef struct {
        uint64_t total_instr;
        uint64_t total_long;
        uint64_t total_expensive;
        inst_info_t window[WINDOW_SIZE];
        int window_count;
} thread_data_t;


/* for readable output */
static const char *const rv_reg_abi[32] = {
  "zero","ra","sp","gp","tp","t0","t1","t2",
  "s0",  "s1","a0","a1","a2","a3","a4","a5",
  "a6",  "a7","s2","s3","s4","s5","s6","s7",
  "s8",  "s9","s10","s11","t3","t4","t5","t6"
};
#define REG_NAME(r) (((r) >= 0 && (r) < 32) ? rv_reg_abi[(r)] : "?")

static void decode_inst_info(mambo_context *ctx, inst_info_t *info) {
    info->iclass     = INST_OTHER;
    info->dest_reg   = -1;
    info->src_reg[0] = -1;
    info->src_reg[1] = -1;
    info->pc         = (uintptr_t)mambo_get_source_addr(ctx);
    info->text[0]    = '\0';    
    int inst = mambo_get_inst(ctx);
    unsigned int rd_f, rs1_f, rs2_f, imm_f; 
    switch (inst) {
    case RISCV_LB: case RISCV_LH: case RISCV_LW:
    case RISCV_LBU: case RISCV_LHU: case RISCV_LWU:
    case RISCV_LD: case RISCV_FLW: case RISCV_FLD: {
      riscv_lw_decode_fields((uint16_t *)ctx->code.read_address,
                             &rd_f, &rs1_f, &imm_f);
      info->iclass = INST_LONG;
      info->dest_reg = (int)rd_f; info->src_reg[0] = (int)rs1_f;
      const char *mn =
        (inst==RISCV_LB)?"lb":(inst==RISCV_LH)?"lh":(inst==RISCV_LW)?"lw":
        (inst==RISCV_LBU)?"lbu":(inst==RISCV_LHU)?"lhu":(inst==RISCV_LWU)?"lwu":
        (inst==RISCV_FLW)?"flw":(inst==RISCV_FLD)?"fld":"ld";
      snprintf(info->text, sizeof(info->text), "%s %s, %d(%s)",
               mn, REG_NAME(rd_f), (int)imm_f, REG_NAME(rs1_f));
      break;
    }
    case RISCV_C_FLD: case RISCV_C_LW: case RISCV_C_LD:
    case RISCV_C_FLDSP: case RISCV_C_LWSP:
    case RISCV_C_FLWSP: case RISCV_C_LDSP:
      info->iclass = INST_LONG;
      snprintf(info->text, sizeof(info->text), "c.load@0x%"PRIxPTR, info->pc);
      break;
    case RISCV_MUL: case RISCV_MULH: case RISCV_MULHSU: case RISCV_MULHU:
    case RISCV_DIV: case RISCV_DIVU: case RISCV_REM:  case RISCV_REMU:
    case RISCV_MULW: case RISCV_DIVW: case RISCV_DIVUW:
    case RISCV_REMW: case RISCV_REMUW:
    case RISCV_FMUL_S: case RISCV_FDIV_S:
    case RISCV_FMUL_D: case RISCV_FDIV_D: {
      riscv_add_decode_fields((uint16_t *)ctx->code.read_address,
                              &rd_f, &rs1_f, &rs2_f);
      info->iclass = INST_EXPENSIVE;
      info->dest_reg = (int)rd_f; info->src_reg[0] = (int)rs1_f; info->src_reg[1] = (int)rs2_f;
      const char *mn =
        (inst==RISCV_MUL)?"mul":(inst==RISCV_MULH)?"mulh":
        (inst==RISCV_MULHSU)?"mulhsu":(inst==RISCV_MULHU)?"mulhu":
        (inst==RISCV_DIV)?"div":(inst==RISCV_DIVU)?"divu":
        (inst==RISCV_REM)?"rem":(inst==RISCV_REMU)?"remu":
        (inst==RISCV_MULW)?"mulw":(inst==RISCV_DIVW)?"divw":
        (inst==RISCV_DIVUW)?"divuw":(inst==RISCV_REMW)?"remw":
        (inst==RISCV_REMUW)?"remuw":(inst==RISCV_FMUL_S)?"fmul.s":
        (inst==RISCV_FDIV_S)?"fdiv.s":(inst==RISCV_FMUL_D)?"fmul.d":"fdiv.d";
      snprintf(info->text, sizeof(info->text), "%s %s, %s, %s",
               mn, REG_NAME(rd_f), REG_NAME(rs1_f), REG_NAME(rs2_f));
      break;
    }
    case RISCV_ADD:  case RISCV_SUB:  case RISCV_SLL:
    case RISCV_SLT:  case RISCV_SLTU: case RISCV_XOR:
    case RISCV_SRL:  case RISCV_SRA:  case RISCV_OR:  case RISCV_AND:
    case RISCV_ADDW: case RISCV_SUBW: case RISCV_SLLW:
    case RISCV_SRLW: case RISCV_SRAW:
    case RISCV_FADD_S: case RISCV_FSUB_S: case RISCV_FMIN_S: case RISCV_FMAX_S:
    case RISCV_FSGNJ_S: case RISCV_FSGNJN_S: case RISCV_FSGNJX_S:
    case RISCV_FADD_D: case RISCV_FSUB_D: case RISCV_FMIN_D: case RISCV_FMAX_D:
    case RISCV_FSGNJ_D: case RISCV_FSGNJN_D: case RISCV_FSGNJX_D: {
      riscv_add_decode_fields((uint16_t *)ctx->code.read_address,
                              &rd_f, &rs1_f, &rs2_f);
      info->dest_reg = (int)rd_f; info->src_reg[0] = (int)rs1_f; info->src_reg[1] = (int)rs2_f;
      snprintf(info->text, sizeof(info->text), "r-op %s, %s, %s",
               REG_NAME(rd_f), REG_NAME(rs1_f), REG_NAME(rs2_f));
      break;
    }
    default:
      snprintf(info->text, sizeof(info->text), "other@0x%"PRIxPTR, info->pc);
      break;
    }
    if (info->dest_reg == 0) info->dest_reg = -1;
}


static bool detect_lej(
    const inst_info_t *window, int window_count,
    int l_idx, int e_idx,
    const inst_info_t *j,
    int *out_rd_l, int *out_rd_e)
    {

    const inst_info_t *L = &window[l_idx];
    const inst_info_t *E = &window[e_idx];
    if (L->iclass != INST_LONG || E->iclass != INST_EXPENSIVE) return false;
    int rd_l = L->dest_reg, rd_e = E->dest_reg;
    if (rd_l < 0 || rd_e < 0)          return false;
    if (rd_l == rd_e)                   return false;
    if (abs(l_idx - e_idx) > MAX_GAP)  return false;
    bool j_reads_rdl = (j->src_reg[0]==rd_l || j->src_reg[1]==rd_l);
    bool j_reads_rde = (j->src_reg[0]==rd_e || j->src_reg[1]==rd_e);
    if (!j_reads_rdl || !j_reads_rde)        return false;
    if (j->src_reg[0] < 0 || j->src_reg[1] < 0) return false;
    int first = (l_idx<e_idx)?l_idx:e_idx;
    int second= (l_idx<e_idx)?e_idx:l_idx;
    if (e_idx>l_idx && (E->src_reg[0]==rd_l||E->src_reg[1]==rd_l)) return false;
    if (l_idx>e_idx && L->src_reg[0]==rd_e)                         return false;
    int first_rd = window[first].dest_reg;
    for (int k = first+1; k < second; k++) {
      const inst_info_t *mid = &window[k];
      if (mid->dest_reg==rd_l || mid->dest_reg==rd_e) return false;
      if (first_rd>=0 &&
          (mid->src_reg[0]==first_rd||mid->src_reg[1]==first_rd) &&
          mid->dest_reg>=0 &&
          (window[second].src_reg[0]==mid->dest_reg||
           window[second].src_reg[1]==mid->dest_reg)) return false;
    }
    for (int k = second+1; k < window_count; k++) {
      if (window[k].dest_reg==rd_l || window[k].dest_reg==rd_e) return false;
    }
    *out_rd_l = rd_l; *out_rd_e = rd_e;
    return true;
}


int dependency_checker_pre_thread(mambo_context *ctx){
    thread_data_t *t_data = (thread_data_t *)mambo_alloc(ctx, sizeof(thread_data_t));
    assert(t_data != NULL);
    assert(mambo_set_thread_plugin_data(ctx, t_data) == MAMBO_SUCCESS);
    return 0;
}

int dependency_checker_post_thread(mambo_context *ctx){
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
}

int dependency_checker_pre_bb(mambo_context *ctx) {
  thread_data_t *t_data = (thread_data_t *)mambo_get_thread_plugin_data(ctx);
  if (t_data) t_data->window_count = 0;
  return 0;
}


int dependency_checker_pre_inst(mambo_context *ctx) {
    thread_data_t *t_data = (thread_data_t *)mambo_get_thread_plugin_data(ctx);
    if (t_data == NULL) return 0;

    void *pc = mambo_get_source_addr(ctx);
    int   inst = mambo_get_inst(ctx);
    inst_info_t curr_inst;

    emit_counter64_incr(ctx, &t_data->total_instr, 1);

    decode_inst_info(ctx, &curr_inst);
    if (curr_inst.iclass == INST_LONG) {
      emit_counter64_incr(ctx, &t_data->total_long, 1);
    } else if (curr_inst.iclass == INST_EXPENSIVE) {
      emit_counter64_incr(ctx, &t_data->total_expensive, 1);
    }


      /* Chain detection — fprintf only, no hash map yet */
    if (curr_inst.src_reg[0] >= 0 && curr_inst.src_reg[1] >= 0) {
     int wc = t_data->window_count;
     for (int long_idx = 0; long_idx < wc; long_idx++) {
       if (t_data->window[long_idx].iclass != INST_LONG) continue;
       for (int expensive_idx = 0; expensive_idx < wc; expensive_idx++) {
         if (expensive_idx == long_idx || t_data->window[expensive_idx].iclass != INST_EXPENSIVE) continue;
         int rd_l = -1, rd_e = -1;
         if (!detect_lej(t_data->window, wc, long_idx, expensive_idx, &curr_inst, &rd_l, &rd_e)) continue;
         fprintf(stderr,
           "[dep_chain] CHAIN FOUND (scan time):\n"
           "  L: %-36s @ 0x%"PRIxPTR"\n"
           "  E: %-36s @ 0x%"PRIxPTR"\n"
           "  J: %-36s @ 0x%"PRIxPTR"\n"
           "  dep_regs: %s, %s\n",
           t_data->window[long_idx].text, t_data->window[long_idx].pc,
           t_data->window[expensive_idx].text, t_data->window[expensive_idx].pc,
           curr_inst.text, curr_inst.pc,
           REG_NAME(rd_l), REG_NAME(rd_e));
       }
     }
    }

    /* Slide the window */
    if (t_data->window_count < WINDOW_SIZE) {
      t_data->window[t_data->window_count++] = curr_inst;
    } else {
      memmove(&t_data->window[0], &t_data->window[1],
              (WINDOW_SIZE - 1) * sizeof(inst_info_t));
      t_data->window[WINDOW_SIZE - 1] = curr_inst;
    }

    return 0;
}

__attribute__((constructor)) void dependency_checker_init(void) {

    mambo_context *ctx = mambo_register_plugin();
    assert(ctx != NULL);
    fprintf(stderr, "[dep_chain] plugin loaded\n");
    int ret;
    ret = mambo_register_pre_thread_cb(ctx, dependency_checker_pre_thread);
    assert(ret == MAMBO_SUCCESS);
    ret = mambo_register_post_thread_cb(ctx, dependency_checker_post_thread);
    assert(ret == MAMBO_SUCCESS);
    ret = mambo_register_pre_bb_cb(ctx, dependency_checker_pre_bb);
    assert(ret == MAMBO_SUCCESS);
    ret = mambo_register_pre_inst_cb(ctx, dependency_checker_pre_inst);
    assert(ret == MAMBO_SUCCESS);

}

#endif /* PLUGINS_NEW */