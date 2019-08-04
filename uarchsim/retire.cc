#include "pipeline.h"
#include "trap.h"
#include "mmu.h"


void pipeline_t::retire(size_t& instret, bool& amo_csr) {
//<<<<<<< HEAD
   ////printf("retire stage\n");
//=======
   //printf("retire stage\n");
//>>>>>>> 4f751980690d6293df8659bec8d053c49b0cd1c7
   bool head_valid;
   bool completed, misprediction, exception, load, store, branch;
   reg_t offending_PC;

   bool amo;
   bool amo_success;
   bool fp;
   bool csr_atomic;
   trap_t *trap = NULL; // Supress uninitialized warning.


   // FIX_ME #17a
   // Call the precommit() function of the renamer module.  This tells the renamer module to return
   // the state of the instruction at the head of its Active List, if any.  The renamer does not
   // take any action, itself.
   // Moreover:
   // * The precommit() function returns 'true' if there is a head instruction (active list not empty)
   //   and 'false' otherwise (active list empty).
   // * The precommit() function also modifies seven arguments that THIS function must examine and act upon.
   //head_valid=REN->precommit(completed, misprediction, exception, load, store, branch,offending_PC);
   load = IS_LOAD(PAY.buf[PAY.head].flags);
   store = IS_STORE(PAY.buf[PAY.head].flags);
   branch = IS_BRANCH(PAY.buf[PAY.head].flags);
   offending_PC = PAY.buf[PAY.head].pc;
   exception=false;
   misprediction=false;
   // Tips:
   // 1. Call the renamer module's precommit() function with the following arguments in this order:
   //    'completed', 'misprediction', 'exception', 'load', 'store', 'branch', 'offending_PC'.
   //    These are local variables, already declared above.
   // 2. Use the return value of the precommit() function call, to set the already-declared local
   //    variable 'head_valid'.



 //  if (head_valid && completed && !exception) {
      amo = IS_AMO(PAY.buf[PAY.head].flags);
      csr_atomic = IS_CSR(PAY.buf[PAY.head].flags);
      
      amo_csr = amo || csr_atomic;
      /*if( amo_csr)
	{
		printf("amo head index is %d\n",PAY.head);
	}*/
      if (load || store) {
         assert(load != store); // Make sure that the same instruction does not have both flags set/cleared.
         try {
            LSU.commit(load, amo, amo_success);
         }
         catch (mem_trap_t& t) {
            switch (t.cause()) {
               case CAUSE_FAULT_STORE:
                  //REN->set_exception(PAY.buf[PAY.head].AL_index);
                  PAY.buf[PAY.head].trap = new trap_store_access_fault(t.get_badvaddr());
                  break;
               case CAUSE_MISALIGNED_STORE:
                 // REN->set_exception(PAY.buf[PAY.head].AL_index);
                  PAY.buf[PAY.head].trap = new trap_store_address_misaligned(t.get_badvaddr());
                  break;
               default:
                  assert(0);
                  break;
            }
         }
         if (amo && store)
            assert(amo_success);					// assert store-conditionals (SC) are successful
      }
      else if (amo) {
         assert(execute_amo() == false);
      }
      else if (csr_atomic) {
         execute_csr();
      }
 //  }


   // FIX_ME #17b
   // Call the commit() function of the renamer module.  This tells the renamer module to examine
   // the instruction at the head of its Active List, if any, and to take appropriate action w.r.t.
   // the renamer module itself.
   // Moreover:
   // * The commit() function returns 'true' if there is a head instruction (active list not empty)
   //   and 'false' otherwise (active list empty).
   // * The commit() function also modifies seven arguments that THIS function must examine and act upon.
   //head_valid=REN->commit(completed, misprediction, exception, load, store, branch,offending_PC);
   // Tips:
   // 1. Call the renamer module's commit() function with the following arguments in this order:
   //    'completed', 'misprediction', 'exception', 'load', 'store', 'branch', 'offending_PC'.
   //    These are local variables, already declared above.
   // 2. Use the return value of the commit() function call, to set the already-declared local
   //    variable 'head_valid'.
   // 3. Study the code that follows the code that you added, and simply note the following observations:
   //    * The retire stage only does something if the renamer module's commit() function signals
   //      a non-empty active list and that the head instruction is complete: "if (head_valid && completed)".
   //    * If the completed head instruction was not an exception -- "if (!exception)" -- it is committed.
   //      If it is a memory operation -- "if (load || store)" -- we signal the LSU to commit the
   //      memory operation. For loads, this just means removing it from the LSU. For stores,
   //      this means removing it from the LSU and committing it to architectural memory state.
   //      Branches are finalized in the branch prediction unit.  Committed results are checked against
   //      the functional simulator via the checker() function call.  Certain cases, such as
   //      atomics and system instructions, dictate that the pipeline be squashed.
   //    * On the other hand, if the completed head instruction is an exception,
   //      the trap is taken (if it is a true software-visible exception) and the pipeline is squashed.



   if (true/*head_valid && completed*/) {    // AL head exists and completed
      // The misprediction flag is not currently set anywhere.
      // It can be applied to the following new functionality in the future, for example:
      // * Recovery of mispredicted branches when they reach the head of the active list, instead of immediately.
      // * Speculation of store-load dependencies (currently support only always-stall and oracle).
      // * Value prediction.
      assert(!misprediction);

      if (!exception) {
         amo = IS_AMO(PAY.buf[PAY.head].flags);
         csr_atomic = IS_CSR(PAY.buf[PAY.head].flags);
         fp = IS_FP_OP(PAY.buf[PAY.head].flags);
         insn_t inst = PAY.buf[PAY.head].inst;

#if 0
         if (load || store) {
            assert(load != store); // Make sure that the same instruction does not have both flags set/cleared
            bool amo_success;
            assert(LSU.commit(load, amo, amo_success) == false);	// assert no exception on store commit
            if (amo && store)
               assert(amo_success);					// assert store-conditionals (SC) are successful
         }
         else if (amo) {
            assert(execute_amo() == false);
         }
         else if (csr_atomic) {
            assert(0);
            //if (!exception)
            //   assert(execute_csr(PAY, get_state()) == false);
         }
#endif

         // If the committed instruction is a branch, signal the branch predictor to commit its oldest branch.
         if (branch && !PERFECT_BRANCH_PRED) {
	    // TODO (ER): Change the branch predictor interface as follows: BP.commit().
            BP.verify_pred(PAY.buf[PAY.head].pred_tag, PAY.buf[PAY.head].c_next_pc, false);
         }

         // If FP op, cheat and copy the fflags from the functional simulator.
         // TODO: fflags should be (and can be) generated by the ALU. This was done to expedite porting of 721sim to RISCV from PISA.
         if (fp) {
	    db_t *actual = pipe->peek(PAY.buf[PAY.head].db_index);	// Pointer to corresponding instruction in the functional simulator.
            get_state()->fflags = actual->a_state->fflags;
         }

	 // Check results.
	 //PAY.buf[PAY.head].
//	if(PAY.head == 18014 && amo_csr)
//	 printf("-------------------payload buffer head inst index is %d-------------------------\n",PAY.head);
         //printf("-------------------payload buffer tail inst index is %d-------------------------\n",PAY.tail);
	//printf("the checkpoint number is %d and the number of instructions in %d\n", REN->checkpoint_fifo_head, REN->checkpoint_fifo[REN->checkpoint_fifo_head].num_instr);
	 //printf("A_valid = %d\n",PAY.buf[PAY.head].A_valid);
	 //printf("A LOG num is %d\n",PAY.buf[PAY.head].A_log_reg);
	 //printf("A PRF num is %d\n",PAY.buf[PAY.head].A_phys_reg);
	 //printf("A PRF val is %d\n",PAY.buf[PAY.head].A_value.dw);
	 //printf("B_valid = %d\n",PAY.buf[PAY.head].B_valid);
	 //printf("B LOG num is %d\n",PAY.buf[PAY.head].B_log_reg);
	 //printf("B PRF num is %d\n",PAY.buf[PAY.head].B_phys_reg);
	 //printf("B PRF val is %d\n",PAY.buf[PAY.head].B_value.dw);
	 //printf("D_valid = %d\n",PAY.buf[PAY.head].D_valid);
	 //printf("D LOG num is %d\n",PAY.buf[PAY.head].D_log_reg);
	 //printf("D PRF num is %d\n",PAY.buf[PAY.head].D_phys_reg);
	 //printf("D PRF val is %d\n",PAY.buf[PAY.head].D_value.dw);
	 //printf("C_valid = %d\n",PAY.buf[PAY.head].C_valid);
	 //printf("C LOG num is %d\n",PAY.buf[PAY.head].C_log_reg);
	 //printf("C PRF num is %d\n",PAY.buf[PAY.head].C_phys_reg);
	 //printf("C PRF val is %d\n",PAY.buf[PAY.head].C_value.dw);
	//printf("----------------------------------------------------------------------------------\n");
	
       
	 checker();
	 
	 reg_t next_inst_pc = INCREMENT_PC(PAY.buf[PAY.head].pc);

	 // Keep track of the number of retired instructions.
	 num_insn++;
         instret++;
	 inc_counter(commit_count);

	 // Pop the instruction from PAY.
	 if (PAY.buf[PAY.head].split) {
	    PAY.pop();
	    // Keep track of the number of split instructions.
	    // 'num_insn_split' is the number of ISA instructions that were split into two micro-ops.
	    // Thus, this measurement is also a count of the extra number of micro-ops as compared to no splitting.
	    if (PAY.buf[PAY.head].upper) {
	       num_insn_split++;
	    }
	 }
	 else {
	    PAY.pop();
	    PAY.pop();
	 }

         // If an AMO store has been committed, unstall fetch so that
         // instructions following this S.C can be fetched. Squash the
         // pipe on AMO store retire so that all the NOPs inserted by
         // FETCH are squashed.
         if (amo || csr_atomic) {
            clear_fetch_amo();
	   // printf("------------------------ amo instrution retire stage------------------------------------\n");
            // Special next pc for SRET instruction
            if ((inst.funct3() == FN3_SC_SB) && (inst.funct12() == FN12_SRET)) {
               next_inst_pc = state.epc;
            }

            // This squash is necessary to stay in lockstep with checker.
            REN->squash();
	    //REN->print_FL();
//assert(0);
            squash_complete(next_inst_pc);
//std::cout << "RET: " << next_inst_pc << std::endl;
            // Flush PAY.
            PAY.clear();
            inc_counter(recovery_count);
         }
      }
      else {   // exception
assert(0);
         trap = PAY.buf[PAY.head].trap;

         ////////////////////////////
         // Service the trap.
         ////////////////////////////
         reg_t jump_PC;

         // CSR exceptions are micro-architectural exceptions and are
         // not defined by the ISA. These must be handled exclusively by
         // the micro-arch and is different from other exceptions specified
         // in the ISA.
         // This is a serialize trap - Refetch the CSR instruction
         if (trap->cause() == CAUSE_CSR_INSTRUCTION) {
            jump_PC = offending_PC;
         }
         else {
            jump_PC = take_trap(*trap, offending_PC);
         }

         // Keep track of the number of retired instructions.
	 instret++;
	 num_insn++;
         inc_counter(commit_count);
         inc_counter(exception_count);

         // Compare pipeline simulator against functional simulator.
         checker();

         // Step 1: Squash the pipeline.
         squash_complete(jump_PC);

         inc_counter(recovery_count);

         // Flush PAY.
         PAY.clear();
      }
   }
}


bool pipeline_t::execute_amo() {
  unsigned int index = PAY.head;
  insn_t inst = PAY.buf[index].inst;
  reg_t read_amo_value = 0xdeadbeef;

  bool exception = false;

  try{
    if(inst.funct3() == FN3_AMO_W){

      read_amo_value  = mmu->load_int32(PAY.buf[index].A_value.dw);
      uint32_t write_amo_value;
      switch(inst.funct5()){
        case FN5_AMO_SWAP:
          write_amo_value = PAY.buf[index].B_value.dw;
          break;
        case FN5_AMO_ADD:
          write_amo_value = PAY.buf[index].B_value.dw + read_amo_value;
          break;
        case FN5_AMO_XOR:
          write_amo_value = PAY.buf[index].B_value.dw ^ read_amo_value;
          break;
        case FN5_AMO_AND:
          write_amo_value = PAY.buf[index].B_value.dw & read_amo_value;
          break;
        case FN5_AMO_OR:
          write_amo_value = PAY.buf[index].B_value.dw | read_amo_value;
          break;
        case FN5_AMO_MIN:
          write_amo_value = std::min(int32_t(PAY.buf[index].B_value.dw) , int32_t(read_amo_value));
          break;
        case FN5_AMO_MAX:
          write_amo_value = std::max(int32_t(PAY.buf[index].B_value.dw) , int32_t(read_amo_value));
          break;
        case FN5_AMO_MINU:
          write_amo_value = std::min(uint32_t(PAY.buf[index].B_value.dw) , uint32_t(read_amo_value));
          break;
        case FN5_AMO_MAXU:
          write_amo_value = std::max(uint32_t(PAY.buf[index].B_value.dw) , uint32_t(read_amo_value));
          break;
        default:
          assert(0);
      }
      mmu->store_uint32(PAY.buf[index].A_value.dw,write_amo_value);

    } else if(inst.funct3() == FN3_AMO_D) {

      read_amo_value  = mmu->load_int64(PAY.buf[index].A_value.dw);
      reg_t write_amo_value;
      switch(inst.funct5()){
        case FN5_AMO_SWAP:
          write_amo_value = PAY.buf[index].B_value.dw;
          break;
        case FN5_AMO_ADD:
          write_amo_value = PAY.buf[index].B_value.dw + read_amo_value;
          break;
        case FN5_AMO_XOR:
          write_amo_value = PAY.buf[index].B_value.dw ^ read_amo_value;
          break;
        case FN5_AMO_AND:
          write_amo_value = PAY.buf[index].B_value.dw & read_amo_value;
          break;
        case FN5_AMO_OR:
          write_amo_value = PAY.buf[index].B_value.dw | read_amo_value;
          break;
        case FN5_AMO_MIN:
          write_amo_value = std::min(int64_t(PAY.buf[index].B_value.dw) , int64_t(read_amo_value));
          break;
        case FN5_AMO_MAX:
          write_amo_value = std::max(int64_t(PAY.buf[index].B_value.dw) , int64_t(read_amo_value));
          break;
        case FN5_AMO_MINU:
          write_amo_value = std::min(PAY.buf[index].B_value.dw , read_amo_value);
          break;
        case FN5_AMO_MAXU:
          write_amo_value = std::max(PAY.buf[index].B_value.dw , read_amo_value);
          break;
        default:
          assert(0);
      }
      mmu->store_uint64(PAY.buf[index].A_value.dw,write_amo_value);
    } else {
      assert(0);
    }
  }
  catch (mem_trap_t& t)
	{
#if 0
    reg_t epc = proc->PAY.buf[index].pc;
	  if//printf(logging_on,proc->lsu_log, "Cycle %" PRIcycle ": core %3d: store exception %s, epc 0x%016" PRIx64 " badvaddr 0x%16" PRIx64 "\n",
	          proc->cycle, proc->id, t.name(), epc, t.get_badvaddr());
    switch(t.cause()){
      case CAUSE_FAULT_STORE:
        proc->REN_INT->set_exception(al_head, new trap_store_access_fault(t.get_badvaddr()));
        break;
      case CAUSE_MISALIGNED_STORE:
        proc->REN_INT->set_exception(al_head, new trap_store_address_misaligned(t.get_badvaddr()));
        break;
      default:
        assert(0);
    }
#endif

    exception = true;
	}

  // Write the result to the payload buffer for checking purposes
  PAY.buf[index].C_value.dw = read_amo_value;
  // Write the result to the physical register
  assert(PAY.buf[index].C_valid);
  REN->write(PAY.buf[index].C_phys_reg, PAY.buf[index].C_value.dw);

  return exception;
}


void pipeline_t::execute_csr() {
   unsigned int index = PAY.head;
   insn_t inst = PAY.buf[index].inst;
   pipeline_t *p = this; // *p is assumed by the validate_csr and require_supervisor macros.
   int csr;

   // CSR instructions:
   // 1. read the addressed CSR and write its old value into a destination register,
   // 2. modify the old value to get a new value, and
   // 3. write the new value into the addressed CSR.
   reg_t old_value;
   reg_t new_value;

   try {
      if (inst.funct3() != FN3_SC_SB) {
         switch (inst.funct3()) {
            case FN3_CLR:
               csr = validate_csr(PAY.buf[index].CSR_addr, true);
	       old_value = get_pcr(csr);
               new_value = (old_value & ~PAY.buf[index].A_value.dw);
               set_pcr(csr, new_value);
               break;
            case FN3_RW:
               csr = validate_csr(PAY.buf[index].CSR_addr, true);
	       old_value = get_pcr(csr);
               new_value = PAY.buf[index].A_value.dw;
               set_pcr(csr, new_value);
               break;
            case FN3_SET:
               csr = validate_csr(PAY.buf[index].CSR_addr, (PAY.buf[index].A_log_reg != 0));
	       old_value = get_pcr(csr);
               new_value = (old_value | PAY.buf[index].A_value.dw);
               set_pcr(csr, new_value);
               break;
            case FN3_CLR_IMM:
               csr = validate_csr(PAY.buf[index].CSR_addr, true);
	       old_value = get_pcr(csr);
               new_value = (old_value & ~(reg_t)PAY.buf[index].A_log_reg);
               set_pcr(csr, new_value);
               break;
            case FN3_RW_IMM:
               csr = validate_csr(PAY.buf[index].CSR_addr, true);
	       old_value = get_pcr(csr);
               new_value = (reg_t)PAY.buf[index].A_log_reg;
               set_pcr(csr, new_value);
               break;
            case FN3_SET_IMM:
               csr = validate_csr(PAY.buf[index].CSR_addr, true);
	       old_value = get_pcr(csr);
               new_value = (old_value | (reg_t)PAY.buf[index].A_log_reg);
               set_pcr(csr, new_value);
               break;
            default:
               assert(0);
               break;
         }
      }
      else if (inst.funct12() == FN12_SRET) {
         // This is a macro defined in decode.h.
         // This will throw a privileged_instruction trap if processor not in supervisor mode.
         require_supervisor;
         csr = validate_csr(PAY.buf[index].CSR_addr, true);
         old_value = get_pcr(csr);
         new_value = ((old_value & ~(SR_S | SR_EI)) | ((old_value & SR_PS) ? SR_S : 0) | ((old_value & SR_PEI) ? SR_EI : 0));
         set_pcr(csr, new_value);
      }
      else {
         // SCALL and SBREAK.
         // These skip the IQ and execution lanes (completed in Dispatch Stage).
         assert(0);
      }

      if (PAY.buf[index].C_valid) {
         // Write the result (old value of CSR) to the payload buffer for checking purposes.
         PAY.buf[index].C_value.dw = old_value;
         // Write the result (old value of CSR) to the physical destination register.
         REN->write(PAY.buf[index].C_phys_reg, PAY.buf[index].C_value.dw);
      }
   }
   catch (trap_t& t) {
      switch (t.cause()) {
         case CAUSE_PRIVILEGED_INSTRUCTION:
            //REN->set_exception(PAY.buf[index].AL_index);
            PAY.buf[index].trap = new trap_privileged_instruction();
            break;
         case CAUSE_FP_DISABLED:
            //REN->set_exception(PAY.buf[index].AL_index);
            PAY.buf[index].trap = new trap_fp_disabled();
            break;
         default:
            fflush(0);
            assert(0);
            break;
      }
   }
   catch (serialize_t& s) {
      //REN->set_exception(PAY.buf[index].AL_index);
      PAY.buf[index].trap = new trap_csr_instruction();
   }
}
