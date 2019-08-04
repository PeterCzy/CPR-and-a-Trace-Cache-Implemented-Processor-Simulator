// See LICENSE for license details.

#include "pipeline.h"
#include "CacheClass.h"
#include "extension.h"
#include "common.h"
#include "config.h"
#include "sim.h"
#include "htif.h"
#include "disasm.h"
#include <cinttypes>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <climits>
#include <stdexcept>
#include <algorithm>
#include <sys/stat.h>
#include "parameters.h"

#undef STATE
#define STATE state

static unsigned int count_bits32(unsigned int val)
{
  unsigned int count = 0;
  fprintf(stderr,"Counting bits for 0x%x\n",val);
  for(unsigned int i=0;i<32;i++)
  {
    //Mask each bit and increment count if masked value not 0
    count = (val & (1<<i)) ? count + 1: count;
  }
  return count;
}

pipeline_t::pipeline_t(
    sim_t*    _sim,
    mmu_t*    _mmu,
    uint32_t  _id,
    uint32_t  fq_size,
    uint32_t  num_chkpts,
    uint32_t  rob_size,
    uint32_t  iq_size,
    uint32_t  iq_num_parts,
    uint32_t  lq_size,
    uint32_t  sq_size,
    uint32_t  fetch_width,
    uint32_t  dispatch_width,
    uint32_t  issue_width,
    uint32_t  retire_width,
    uint32_t  fu_lane_matrix[]
):
  processor_t(_sim,_mmu,_id),
  statsModule(this),
  BP(),
  FQ(fq_size,this),
  IQ(iq_size,iq_num_parts,this),
  LSU(lq_size, sq_size, Tid, _mmu, this)
{
  unsigned int i;

  // Initialize the thread id.
  this->Tid = _id;

  // Initialize simulator time:
  cycle = 0;
  sequence = 0;

  // Initialize number of retired instructions.
  num_insn = 0;
  num_insn_split = 0;


  /////////////////////////////////////////////////////////////
  // Pipeline widths.
  /////////////////////////////////////////////////////////////
  this->fetch_width = fetch_width;
  this->dispatch_width = dispatch_width;
  this->issue_width = issue_width;
  this->retire_width = retire_width;

  #ifdef RISCV_MICRO_DEBUG
    mkdir("micros_log",S_IRWXU);
    this->fetch_log     = fopen("micros_log/fetch.log", "w")  ;
    this->decode_log    = fopen("micros_log/decode.log", "w")  ;
    this->rename_log    = fopen("micros_log/rename.log", "w")  ;
    this->dispatch_log  = fopen("micros_log/dispatch.log", "w")  ;
    this->issue_log     = fopen("micros_log/issue.log", "w")  ;
    this->regread_log   = fopen("micros_log/regread.log", "w")  ;
    this->execute_log   = fopen("micros_log/execute.log", "w")  ;
    this->lsu_log       = fopen("micros_log/lsu.log", "w")  ;
    this->wback_log     = fopen("micros_log/wback.log", "w")  ;
    this->retire_log    = fopen("micros_log/retire.log", "w")  ;
    this->program_log   = fopen("micros_log/program.log", "w")  ;
    this->cache_log     = fopen("micros_log/cache.log", "w")  ;
  #endif

  /////////////////////////////////////////////////////////////
  // Statistics unit
  /////////////////////////////////////////////////////////////

  // stats must be constructed first as other classes use them
  this->stats       = &statsModule;
  this->stats_log   = fopen("stats.log", "w")  ;
  this->phase_log   = fopen("phase.log", "w")  ;
  stats->set_log_files(stats_log,phase_log);
  stats->set_phase_interval("commit_count",phase_interval);

  /////////////////////////////////////////////////////////////
  // Fetch unit.
  /////////////////////////////////////////////////////////////
  
  // the ISA guarantees on boot that the PC is 0x2000 and the the processor
  // is in supervisor mode, and in 64-bit mode, if supported, with traps
  // and virtual memory disabled.
  pc = 0x2000;
  next_fetch_cycle = 0;

  if(L2_PRESENT){
    L2C = new CacheClass( L2_SETS,
                        L2_ASSOC,
                        L2_LINE_SIZE,
                        L2_HIT_LATENCY,
                        L2_MISS_LATENCY,
                        L2_NUM_MHSRs,
                        L2_MISS_SRV_PORTS,
                        L2_MISS_SRV_LATENCY,
                        this,
                        "l2_c",
                        NULL);
  } else {
    L2C = NULL;
  }

  IC = new CacheClass(  L1_IC_SETS,
                        L1_IC_ASSOC,
                        L1_IC_LINE_SIZE,
                        L1_IC_HIT_LATENCY,
                        L1_IC_MISS_LATENCY,
                        L1_IC_NUM_MHSRs,
                        L1_IC_MISS_SRV_PORTS,
                        L1_IC_MISS_SRV_LATENCY,
                        this,
                        "l1_ic",
                        L2C);

  LSU.set_l2_cache(L2C);

  /////////////////////////////////////////////////////////////
  // Pipeline register between the Fetch and Decode Stages.
  /////////////////////////////////////////////////////////////
  DECODE = new pipeline_register[fetch_width];

  /////////////////////////////////////////////////////////////
  // Pipeline register between the Rename1 and Rename2
  // sub-stages (within the Rename Stage).
  /////////////////////////////////////////////////////////////
  RENAME2 = new pipeline_register[dispatch_width];

  ////////////////////////////////////////////////////////////
  // Set up the register renaming modules.
  ////////////////////////////////////////////////////////////
  REN = new renamer(NXPR+NFPR, (NXPR + NFPR + rob_size), num_chkpts);

  /////////////////////////////////////////////////////////////
  // Pipeline register between the Rename and Dispatch Stages.
  /////////////////////////////////////////////////////////////
  DISPATCH = new pipeline_register[dispatch_width];

  /////////////////////////////////////////////////////////////
  // Execution Lanes.
  /////////////////////////////////////////////////////////////
  Execution_Lanes = new lane[issue_width];

  for (i = 0; i < (unsigned int)NUMBER_FU_TYPES; i++) {
    this->fu_lane_matrix[i] = fu_lane_matrix[i];
    this->fu_lane_ptr[i] = 0;
    fprintf(stderr,"FU %d -> 0x%x\n",i,fu_lane_matrix[i]);
  }

  /////////////////////////////////////////////////////////////
  // Load-Store Unit.
  /////////////////////////////////////////////////////////////


  // Declare and set the various knobs in the knobs database.
  // These will be printed in the stats.log file at the end of the run.

  DECLARE_KNOB(get_stats(), perfect_branch_pred, PERFECT_BRANCH_PRED, proc);
  DECLARE_KNOB(get_stats(), perfect_fetch, PERFECT_FETCH, proc);
  DECLARE_KNOB(get_stats(), oracle_disambig, ORACLE_DISAMBIG, proc);
  DECLARE_KNOB(get_stats(), perfect_icache, PERFECT_ICACHE, proc);
  DECLARE_KNOB(get_stats(), perfect_dcache, PERFECT_DCACHE, proc);

  DECLARE_KNOB(get_stats(), fetch_queue_size, fq_size, proc);
  DECLARE_KNOB(get_stats(), num_checkpoints, num_chkpts, proc);
  DECLARE_KNOB(get_stats(), activelist_size, rob_size ,proc);
  DECLARE_KNOB(get_stats(), iq_size, iq_size, proc);
  DECLARE_KNOB(get_stats(), iq_num_parts, iq_num_parts, proc);
  DECLARE_KNOB(get_stats(), presteer, PRESTEER, proc);
  DECLARE_KNOB(get_stats(), lq_size, lq_size, proc);
  DECLARE_KNOB(get_stats(), sq_size, sq_size, proc);

  DECLARE_KNOB(get_stats(), fetch_width, fetch_width, proc);
  DECLARE_KNOB(get_stats(), dispatch_width, dispatch_width, proc);
  DECLARE_KNOB(get_stats(), issue_width, issue_width, proc);
  DECLARE_KNOB(get_stats(), retire_width, retire_width, proc);

  DECLARE_KNOB(get_stats(), num_br_lanes, count_bits32(fu_lane_matrix[FU_BR]), proc);
  DECLARE_KNOB(get_stats(), num_ls_lanes, count_bits32(fu_lane_matrix[FU_LS]), proc);
  DECLARE_KNOB(get_stats(), num_alu_s_lanes, count_bits32(fu_lane_matrix[FU_ALU_S]), proc);
  DECLARE_KNOB(get_stats(), num_alu_c_lanes, count_bits32(fu_lane_matrix[FU_ALU_C]), proc);
  DECLARE_KNOB(get_stats(), num_ls_fp_lanes, count_bits32(fu_lane_matrix[FU_LS_FP]), proc);
  DECLARE_KNOB(get_stats(), num_alu_fp_lanes, count_bits32(fu_lane_matrix[FU_ALU_FP]), proc);
  DECLARE_KNOB(get_stats(), num_mtf_lanes, count_bits32(fu_lane_matrix[FU_MTF]), proc);

  DECLARE_KNOB(get_stats(), L1D_size, (L1_DC_SETS*L1_DC_ASSOC*(1<<L1_DC_LINE_SIZE)), proc);
  DECLARE_KNOB(get_stats(), L1D_assoc, L1_DC_ASSOC, proc);
  DECLARE_KNOB(get_stats(), L1D_block_size, (1<<L1_DC_LINE_SIZE), proc);
  DECLARE_KNOB(get_stats(), L1D_hit_latency, L1_DC_HIT_LATENCY, proc);
  DECLARE_KNOB(get_stats(), L1D_mhsr, L1_DC_NUM_MHSRs, proc);
  if (!L2_PRESENT) DECLARE_KNOB(get_stats(), L1D_miss_latency, L1_DC_MISS_LATENCY, proc);

  DECLARE_KNOB(get_stats(), L1I_size, (L1_IC_SETS*L1_IC_ASSOC*(1<<L1_IC_LINE_SIZE)), proc);
  DECLARE_KNOB(get_stats(), L1I_assoc, L1_IC_ASSOC, proc);
  DECLARE_KNOB(get_stats(), L1I_block_size, (1<<L1_IC_LINE_SIZE), proc);
  DECLARE_KNOB(get_stats(), L1I_hit_latency, L1_IC_HIT_LATENCY, proc);
  DECLARE_KNOB(get_stats(), L1I_mhsr, L1_IC_NUM_MHSRs, proc);
  if (!L2_PRESENT) DECLARE_KNOB(get_stats(), L1I_miss_latency, L1_IC_MISS_LATENCY, proc);

  if (L2_PRESENT) {
     DECLARE_KNOB(get_stats(), L2_size, (L2_SETS*L2_ASSOC*(1<<L2_LINE_SIZE)), proc);
     DECLARE_KNOB(get_stats(), L2_assoc, L2_ASSOC, proc);
     DECLARE_KNOB(get_stats(), L2_block_size, (1<<L2_LINE_SIZE), proc);
     DECLARE_KNOB(get_stats(), L2_hit_latency, L2_HIT_LATENCY, proc);
     DECLARE_KNOB(get_stats(), L2_mhsr, L2_NUM_MHSRs, proc);
     DECLARE_KNOB(get_stats(), L2_miss_latency, L2_MISS_LATENCY, proc);
  }

  DECLARE_KNOB(get_stats(), ctiq_size, CTIQ_SIZE, proc);
  DECLARE_KNOB(get_stats(), btb_size, BTB_SIZE, proc);
  DECLARE_KNOB(get_stats(), bp_table_size, BP_TABLE_SIZE, proc);
  DECLARE_KNOB(get_stats(), ras_size, RAS_SIZE, proc);


  // Provide each module with a pointer to the stats object
  LSU.set_stats(get_stats());
  //REN_INT->set_stats(get_stats());
  BP.set_stats(get_stats());

  ///////////////////////////////////////////////////
  // Set up the memory system.
  ///////////////////////////////////////////////////

  reset(true);
  mmu->set_processor(this);

}


pipeline_t::~pipeline_t()
{
  stats->dump_knobs();
  stats->dump_counters();
  stats->dump_rates();
  stats->dump_pc_histogram();
  stats->dump_br_histogram();
#ifdef RISCV_ENABLE_HISTOGRAM
  if (histogram_enabled)
  {
    ifprintf(logging_on,stderr, "PC Histogram size:%lu\n", pc_histogram.size());
    for(auto iterator = pc_histogram.begin(); iterator != pc_histogram.end(); ++iterator) {
      ifprintf(logging_on,stderr, "%0lx %lu\n", (iterator->first << 2), iterator->second);
    }
  }
#endif

  #ifdef RISCV_MICRO_DEBUG
    fclose(this->fetch_log    );
    fclose(this->decode_log   );
    fclose(this->rename_log   );
    fclose(this->dispatch_log );
    fclose(this->issue_log    );
    fclose(this->regread_log  );
    fclose(this->execute_log  );
    fclose(this->lsu_log      );
    fclose(this->wback_log    );
    fclose(this->retire_log   );
    fclose(this->program_log  );
    fclose(this->cache_log    );
  #endif

  fclose(this->stats_log   );
  fclose(this->phase_log   );
}

inline void pipeline_t::update_histogram(size_t pc)
{
#ifdef RISCV_ENABLE_HISTOGRAM
  size_t idx = pc >> 2;
  pc_histogram[idx]++;
#endif
}


static void commit_log(state_t* state, reg_t pc, insn_t insn)
{
#ifdef RISCV_ENABLE_COMMITLOG
  if (state->sr & SR_EI) {
    uint64_t mask = (insn.length() == 8 ? uint64_t(0) : (uint64_t(1) << (insn.length() * 8))) - 1;
    if (state->log_reg_write.addr) {
      ifprintf(logging_on,stderr, "0x%016" PRIx64 " (0x%08" PRIx64 ") %c%2" PRIu64 " 0x%016" PRIx64 "\n",
              pc,
              insn.bits() & mask,
              state->log_reg_write.addr & 1 ? 'f' : 'x',
              state->log_reg_write.addr >> 1,
              state->log_reg_write.data);
    } else {
      ifprintf(logging_on,stderr, "0x%016" PRIx64 " (0x%08" PRIx64 ")\n", pc, insn.bits() & mask);
    }
  }
  state->log_reg_write.addr = 0;
#endif
}


//Scope of this function is just this file
static reg_t execute_insn(pipeline_t* p, reg_t pc, insn_fetch_t fetch)
{
  reg_t npc = fetch.func(p, fetch.insn, pc);
  commit_log(p->get_state(), pc, fetch.insn);
  p->update_histogram(pc);
  return npc;
}

//Scope of this function is just this file
void pipeline_t::update_timer(state_t* state, size_t instret)
{
  uint64_t count0 = (uint64_t)(uint32_t)state->count;
  state->count += instret;
  uint64_t before = count0 - state->compare;
  if (int64_t(before ^ (before + instret)) < 0)
    state->sr |= (1 << (IRQ_TIMER + SR_IP_SHIFT));
}

//Scope of this function is just this file
static size_t next_timer(state_t* state)
{
  ifprintf(logging_on,stderr,"MICRO_SIM next timer: %u\n",(state->compare - (uint32_t)state->count));
  return state->compare - (uint32_t)state->count;
}

bool pipeline_t::step_micro(size_t n,size_t& instret)
{
  instret = 0;
  prev_instret = 0;

  //TODO: This is needed for functional simulator
  //reg_t pc = state.pc;
  mmu_t* _mmu = mmu;

  if (unlikely(!run || !n)) {
    return false;
  }
  n = std::min(n, next_timer(&state) | 1U);

  try
  {
    take_interrupt();

    if (unlikely(debug))
    {
      //TODO: If the system suddenly enters debug mode, 
      //1) Truncate execution at the AL head only once, set a flag to indicate this has already happened
      //2) If all state is maintained, this should just work.
      while (instret < n)
      {
        instret++;
        insn_fetch_t fetch = mmu->load_insn(pc);
        disasm(fetch.insn);
        pc = execute_insn(this, pc, fetch);
        update_timer(&state, instret-prev_instret);
        prev_instret = instret;
        // If this was an idle cycle break
        if(!instret)
          break;
      }
    }
    else {
      //instret is updated in retire
      while (instret < n)
      {

        /////////////////////////////////////////////////////////////
        // Pipeline.
        /////////////////////////////////////////////////////////////

        size_t lane_number;

        unsigned int prev_commit_count = counter(commit_count);
        //for (lane_number = 0; lane_number < RETIRE_WIDTH; lane_number++) {
          //retire(instret);            // Retire Stage
          //update_timer(&state, instret-prev_instret);
          //prev_instret = instret;
          // Halt retirement if its time for an HTIF tick as this will change state
	  //printf("cycle %d: instret = %d\n",cycle, instret);
          if(instret == n)
            break;
          // Stop simulation if limit reached
          if((counter(commit_count) >= stop_amt) && use_stop_amt){
            //stats->dump_knobs();
            //stats->dump_counters();
            //stats->dump_rates();
            return true;
          }
        //}
        // Increment the retired bundle count if even a single instruction retired
        if(counter(commit_count) > prev_commit_count)
          inc_counter(retired_bundle_count);
	REN->reclaim_PRF();
        //REN_INT->dump_al(this,PAY,2,regread_log);
        for (lane_number = 0; lane_number < ISSUE_WIDTH; lane_number++) {
          writeback(lane_number, instret);    // Writeback Stage
        }
        load_replay();
        for (lane_number = 0; lane_number < ISSUE_WIDTH; lane_number++) {
          execute(lane_number);    // Execute Stage
        }
        for (lane_number = 0; lane_number < ISSUE_WIDTH; lane_number++) {
          register_read(lane_number);    // Register Read Stage
        }
        schedule();           // Schedule Stage
        dispatch();           // Dispatch Stage
        rename2();            // Rename Stage
        rename1();            // Rename Stage
        decode();             // Decode Stage
        //// FETCH will insert NOPs instead of fetching real instructions
        //// from cache if a fetch_exception is pending. This is to make
        //// dispatch never gets stalled due to the absense of a full bundle
        //// in the FETCH QUEUS.his is sort of like a stall.
        //if(!fetch_exception){
          fetch();            // Fetch Stage
        //}

        /////////////////////////////////////////////////////////////
        // Miscellaneous stuff that must be processed every cycle.
        /////////////////////////////////////////////////////////////

        // Go to the next simulator cycle.
        //next_cycle();
        cycle++;
//<<<<<<< HEAD
//	printf("----------------cycle is %d----------------------\n",cycle);
/*	if(cycle >8099)
	{
		printf("assertion about to be set");
	}*/
//=======
//	printf("cycle is %d",cycle);
//>>>>>>> 4f751980690d6293df8659bec8d053c49b0cd1c7
        inc_counter(cycle_count);

        if(cycle > (uint64_t)logging_on_at)
          logging_on = true;


        // For detecting deadlock, and monitoring progress.
        if (MOD(cycle, 0x40000) == 0) {
          INFO("(cycle = %" PRIcycle " ) num_insn = %.0f\tIPC = %.2f",
               cycle,
               (double)num_insn,
               (double)num_insn/(double)cycle);
          //stats->dump_counters();
          //stats->dump_rates();
        }

        // If this was an idle cycle break so that HTIF may have a chance to tick
        if(!instret)
          break;

      }
    }
  }
  //catch(mem_trap_t& t)
  //{
  //  //TODO: This can be the point of synchronization when checking is implemented.
  //  //Only the functional simulator takes the trap and then once it is done, all 
  //  //architectural state will be copied over to this processor, including CSRs etc.
  //  ifprintf(logging_on,stderr, "MICRO_SIM: Cycle %" PRIcycle ": core %3d: mem exception %s, epc 0x%016" PRIx64 " badvaddr 0x%016" PRIx64 "\n",
  //          cycle, id, t.name(), pc, t.get_badvaddr());
  //  pc = take_trap(t, pc);
  //}
  catch(trap_t& t)
  {
    //TODO: This can be the point of synchronization when checking is implemented.
    //Only the functional simulator takes the trap and then once it is done, all 
    //architectural state will be copied over to this processor, including CSRs etc.
    pc = take_trap(t, pc);
  }
  catch(serialize_t& s) {
    ifprintf(logging_on,stderr,"MICROS: Caught serialize_t exception...........\n");
  }

  state.pc = pc;
  //update_timer(&state, instret);
  return false;
}

reg_t pipeline_t::take_trap(trap_t& t, reg_t epc)
{
  #ifdef RISCV_MICRO_DEBUG
    //TODO: Add if(debug) back
    //if (debug)
      ifprintf(logging_on,stderr, "MICRO_SIM: Cycle %" PRIcycle ": core %3d: exception %s, epc 0x%016" PRIx64 "\n",
              cycle, id, t.name(), epc);
  #endif

  // switch to supervisor, set previous supervisor bit, disable interrupts
  set_pcr(CSR_STATUS, (((state.sr & ~SR_EI) | SR_S) & ~SR_PS & ~SR_PEI) |
          ((state.sr & SR_S) ? SR_PS : 0) |
          ((state.sr & SR_EI) ? SR_PEI : 0));

  yield_load_reservation();
  state.cause = t.cause();
  state.epc = epc;
  t.side_effects(&state); // might set badvaddr etc.
  return state.evec;
}

void pipeline_t::disasm(insn_t insn)
{
  uint64_t bits = insn.bits() & ((1ULL << (8 * insn_length(insn.bits()))) - 1);
  ifprintf(logging_on,stderr, "core %3d: 0x%016" PRIx64 " (0x%08" PRIx64 ") %s\n",
          id, state.pc, bits, disassembler->disassemble(insn).c_str());
}

void pipeline_t::disasm(insn_t insn,reg_t pc)
{
  uint64_t bits = insn.bits() & ((1ULL << (8 * insn_length(insn.bits()))) - 1);
  ifprintf(logging_on,stderr, "core %3d: 0x%016" PRIx64 " (0x%08" PRIx64 ") %s\n",
          id, pc, bits, disassembler->disassemble(insn).c_str());
}

void pipeline_t::disasm(insn_t insn, reg_t pc, FILE* out)
{
  uint64_t bits = insn.bits() & ((1ULL << (8 * insn_length(insn.bits()))) - 1);
  ifprintf(logging_on,out, "Cycle %" PRIcycle ": Seq %" PRIu64 " PC 0x%016" PRIx64 " (0x%08" PRIx64 ") %s\n",
          cycle, sequence, pc, bits, disassembler->disassemble(insn).c_str());
}

void pipeline_t::disasm(insn_t insn, cycle_t cycle, reg_t pc, uint64_t seq, FILE* out)
{
  uint64_t bits = insn.bits() & ((1ULL << (8 * insn_length(insn.bits()))) - 1);
  ifprintf(logging_on,out, "Cycle %" PRIcycle ": Seq %" PRIu64 " PC 0x%016" PRIx64 " (0x%08" PRIx64 ") %s\n",
          cycle, seq, pc, bits, disassembler->disassemble(insn).c_str());
}



//void pipeline_t::copy_state_from_actual(db_t* actual) {
//
//    state_t* isa_state  = pipe.get_isa_state();
//
//   reg_t x;
//
//
//   for (unsigned int i = 0; i < NXPR; i++){
//      // Integer RF: general registers 0-31.
//      REN_INT->write(REN_INT->rename_rsrc(i), isa_state->NXPR[i]);
//      // Floating point RF: general registers 0-31.
//      REN_INT->write(REN_INT->rename_rsrc(i+NXPR), isa_state->NFPR[i]);
//   }
//
//}

void pipeline_t::copy_state_to_micro() {
   for (unsigned int i = 0; i < NXPR; i++){
      // Integer RF: general registers 0-31.
      REN->write(REN->rename_rsrc(i), get_state()->XPR[i]);
      // Floating point RF: general registers 0-31.
      REN->write(REN->rename_rsrc(i+NXPR), get_state()->FPR[i]);
   }

   pc = get_state()->pc;
}

uint64_t pipeline_t::get_arch_reg_value(int reg_id) { 

    return REN->read(REN->rename_rsrc(reg_id));
}


uint32_t pipeline_t::get_instruction(uint64_t inst_pc){
  //TODO: handle fetch exceptions
  insn_fetch_t inst_raw = mmu->load_insn(inst_pc);
  return (uint32_t)inst_raw.insn.bits();
}

void pipeline_t::set_exception(unsigned int al_index) {
   REN->set_exception(al_index);
}
