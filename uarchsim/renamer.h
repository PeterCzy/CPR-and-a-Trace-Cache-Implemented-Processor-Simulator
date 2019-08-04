#include <inttypes.h>
#include <cassert>
#include <stdio.h>
#include <iostream>
#include <vector>
struct FL
{
	uint64_t* list;
	uint64_t head;
	uint64_t tail;
	bool empty;
	bool full;
	uint64_t FL_size;
};

struct AL_entry
{
	bool dest_flag;
	uint64_t log_n_reg;
	uint64_t phy_n_reg;
	bool complete;
	bool misprediction;
	bool exception;
	bool load_flag;
	bool store_flag;
	bool branch_flag;
	uint64_t PC;
};

struct AL
{
	AL_entry* list;
	uint64_t head;
	uint64_t tail;
	bool full;
	bool empty;
	uint64_t AL_size;
};

struct CHK
{
	uint64_t *SMT;
	uint64_t check_head;
	uint64_t check_GBM;
};

struct CHK_instr
{
	uint64_t *SMT;
	uint64_t check_head;
	uint64_t num_instr;
	uint64_t PC;
	bool branch_mispredicted;
	uint64_t mispred_inst_num;
	uint64_t max_instr;
	uint64_t num_store;
};


class renamer {
   private:

      /////////////////////////////////////////////////////////////////////
      // structures used for project4 CPR
      /////////////////////////////////////////////////////////////////////
	

	

      /////////////////////////////////////////////////////////////////////
      // Put private class variables here.
      /////////////////////////////////////////////////////////////////////

	uint64_t inst_count;
      /////////////////////////////////////////////////////////////////////
      // Structure 1: Rename Map Table
      // Entry contains: physical register mapping
      /////////////////////////////////////////////////////////////////////
	   uint64_t* RMT;
	   uint64_t RMT_size;
      /////////////////////////////////////////////////////////////////////
      // Structure 2: Architectural Map Table
      // Entry contains: physical register mapping
      /////////////////////////////////////////////////////////////////////
	   //uint64_t* AMT;
	   //uint64_t AMT_size;
      /////////////////////////////////////////////////////////////////////
      // Structure 3: Free List
      //
      // Entry contains: physical register number
      //
      // Notes:
      // * Structure includes head, tail, and possibly other variables
      //   depending on your implementation.
      /////////////////////////////////////////////////////////////////////

	   FL free_list;
	   uint64_t FL_size;
      /////////////////////////////////////////////////////////////////////
      // Structure 4: Active List
      //
      // Entry contains:
      // 1. destination flag (indicates whether or not the instr. has a
      //    destination register)
      // 2. logical register number of the instruction's destination
      // 3. physical register number of the instruction's destination
      // 4. completed bit
      // 5. misprediction bit
      // 6. exception bit
      // 7. load flag (indicates whether or not the instr. is a load)
      // 8. store flag (indicates whether or not the instr. is a store)
      // 9. branch flag (indicates whether or not the instr. is a branch)
      // 10. program counter of the instruction
      //
      // Notes:
      // * Structure includes head, tail, and possibly other variables
      //   depending on your implementation.
      /////////////////////////////////////////////////////////////////////


	   //AL active_list;
	   //uint64_t AL_size;
      /////////////////////////////////////////////////////////////////////
      // Structure 5: Physical Register File
      // Entry contains: value
      //
      // Notes:
      // * The value must be of the following type: uint64_t
      //   (#include <inttypes.h>, already at top of this file)
      /////////////////////////////////////////////////////////////////////
	   uint64_t* PRF;
	   uint64_t PRF_size;
      /////////////////////////////////////////////////////////////////////
      // Structure 6: Physical Register File Ready Bit Array
      // Entry contains: ready bit
      /////////////////////////////////////////////////////////////////////
	   bool* PRF_ready;
      /////////////////////////////////////////////////////////////////////
      // Structure 7: Global Branch Mask (GBM)
      //
      // The Global Branch Mask (GBM) is a bit vector that keeps track of
      // all unresolved branches. A '1' bit corresponds to an unresolved
      // branch. The "branch ID" of the unresolved branch is its position
      // in the bit vector.
      //
      // The GBM serves two purposes:
      //
      // 1. It provides a means for allocating checkpoints to unresolved
      //    branches. There are as many checkpoints as there are bits in
      //    the GBM. If all bits in the GBM are '1', then there are no
      //    free bits, hence, no free checkpoints. On the other hand, if
      //    not all bits in the GBM are '1', then any of the '0' bits
      //    are free and the corresponding checkpoints are free.
      //
      // 2. Each in-flight instruction needs to know which unresolved
      //    branches it depends on, i.e., which unresolved branches are
      //    logically before it in program order. This information
      //    makes it possible to squash instructions that are after a
      //    branch, in program order, and not instructions before the
      //    branch. This functionality will be implemented using
      //    branch masks, as was done in the MIPS R10000 processor.
      //    An instruction's initial branch mask is the value of the
      //    the GBM when the instruction is renamed.
      //
      // The simulator requires an efficient implementation of bit vectors,
      // for quick copying and manipulation of bit vectors. Therefore, you
      // must implement the GBM as type "uint64_t".
      // (#include <inttypes.h>, already at top of this file)
      // The "uint64_t" type contains 64 bits, therefore, the simulator
      // cannot support a processor configuration with more than 64
      // unresolved branches. The maximum number of unresolved branches
      // is configurable by the user of the simulator, and can range from
      // 1 to 64.
      /////////////////////////////////////////////////////////////////////
	   //uint64_t GBM;
      /////////////////////////////////////////////////////////////////////
      // Structure 8: Branch Checkpoints
      //
      // Each branch checkpoint contains the following:
      // 1. Shadow Map Table (checkpointed Rename Map Table)
      // 2. checkpointed Free List head index
      // 3. checkpointed GBM
      /////////////////////////////////////////////////////////////////////
	   //CHK* checkpoints;
	   uint64_t CHK_size;
      /////////////////////////////////////////////////////////////////////
      // Private functions.
      // e.g., a generic function to copy state from one map to another.
      /////////////////////////////////////////////////////////////////////
	   void copy(uint64_t* target, const uint64_t* source, uint64_t size);
	//void empty_AL_entry(uint64_t AL_index);
   public:
      ////////////////////////////////////////
      // Public functions.
      ////////////////////////////////////////
	uint64_t PRF_reclaim_counter;
	CHK_instr* checkpoint_fifo;
	uint64_t* PRF_usage_counter;
	bool* PRF_unmapped;
	bool* PRF_free;
	uint64_t checkpoint_fifo_head;
	uint64_t checkpoint_fifo_tail;
	uint64_t checkpoint_ID;
      uint64_t instr_between_CHK; //number of instr between checkpoints

      /////////////////////////////////////////////////////////////////////
      // This is the constructor function.
      // When a renamer object is instantiated, the caller indicates:
      // 1. The number of logical registers (e.g., 32).
      // 2. The number of physical registers (e.g., 128).
      // 3. The maximum number of unresolved branches.
      //    Requirement: 1 <= n_branches <= 64.
      //
      // Tips:
      //
      // Assert the number of physical registers > number logical registers.
      // Assert 1 <= n_branches <= 64.
      // Then, allocate space for the primary data structures.
      // Then, initialize the data structures based on the knowledge
      // that the pipeline is intially empty (no in-flight instructions yet).
      /////////////////////////////////////////////////////////////////////
      renamer(uint64_t n_log_regs,
            uint64_t n_phys_regs,
            uint64_t n_branches);

      /////////////////////////////////////////////////////////////////////
      // This is the destructor, used to clean up memory space and
      // other things when simulation is done.
      // I typically don't use a destructor; you have the option to keep
      // this function empty.
      /////////////////////////////////////////////////////////////////////
      ~renamer();


      //////////////////////////////////////////
      // Functions related to Rename Stage.   //
      //////////////////////////////////////////

      /////////////////////////////////////////////////////////////////////
      // The Rename Stage must stall if there aren't enough free physical
      // registers available for renaming all logical destination registers
      // in the current rename bundle.
      //
      // Inputs:
      // 1. bundle_dst: number of logical destination registers in
      //    current rename bundle
      //
      // Return value:
      // Return "true" (stall) if there aren't enough free physical
      // registers to allocate to all of the logical destination registers
      // in the current rename bundle.
      /////////////////////////////////////////////////////////////////////
      bool stall_reg(uint64_t bundle_dst);

      /////////////////////////////////////////////////////////////////////
      // The Rename Stage must stall if there aren't enough free
      // checkpoints for all branches in the current rename bundle.
      //
      // Inputs:
      // 1. bundle_branch: number of branches in current rename bundle
      //
      // Return value:
      // Return "true" (stall) if there aren't enough free checkpoints
      // for all branches in the current rename bundle.
      /////////////////////////////////////////////////////////////////////
      //bool stall_branch(uint64_t bundle_branch);

      /////////////////////////////////////////////////////////////////////
      // This function is used to get the branch mask for an instruction.
      /////////////////////////////////////////////////////////////////////
      uint64_t get_branch_mask();

      /////////////////////////////////////////////////////////////////////
      // This function is used to rename a single source register.
      //
      // Inputs:
      // 1. log_reg: the logical register to rename
      //
      // Return value: physical register name
      /////////////////////////////////////////////////////////////////////
      uint64_t rename_rsrc(uint64_t log_reg);

      /////////////////////////////////////////////////////////////////////
      // This function is used to rename a single destination register.
      //
      // Inputs:
      // 1. log_reg: the logical register to rename
      //
      // Return value: physical register name
      /////////////////////////////////////////////////////////////////////
      uint64_t rename_rdst(uint64_t log_reg);

      /////////////////////////////////////////////////////////////////////
      // This function creates a new branch checkpoint.
      //
      // Inputs: none.
      //
      // Output:
      // 1. The function returns the branch's ID. When the branch resolves,
      //    its ID is passed back to the renamer via "resolve()" below.
      //
      // Tips:
      //
      // Allocating resources for the branch (a GBM bit and a checkpoint):
      // * Find a free bit -- i.e., a '0' bit -- in the GBM. Assert that
      //   a free bit exists: it is the user's responsibility to avoid
      //   a structural hazard by calling stall_branch() in advance.
      // * Set the bit to '1' since it is now in use by the new branch.
      // * The position of this bit in the GBM is the branch's ID.
      // * Use the branch checkpoint that corresponds to this bit.
      //
      // The branch checkpoint should contain the following:
      // 1. Shadow Map Table (checkpointed Rename Map Table)
      // 2. checkpointed Free List head index
      // 3. checkpointed GBM
      /////////////////////////////////////////////////////////////////////
      //uint64_t checkpoint();

      /////////////////////////////////////////////////////////////////////////////
      // Function related to placing a checkpoing after N number of instructions //
      /////////////////////////////////////////////////////////////////////////////
      bool add_checkpoint();
      void add_checkpoint(uint64_t current_pc,uint64_t& current_insn_checkpoint_ID, bool amo);
      void increment_usage_counter(uint64_t log_reg);

      void reset_unmap_bit(uint64_t log_reg);

      void set_unmap_bit(uint64_t log_reg);

      void assign_PC(uint64_t current_PC);

      //////////////////////////////////////////
      // Functions related to Dispatch Stage. //
      //////////////////////////////////////////

      /////////////////////////////////////////////////////////////////////
      // The Dispatch Stage must stall if there are not enough free
      // entries in the Active List for all instructions in the current
      // dispatch bundle.
      //
      // Inputs:
      // 1. bundle_inst: number of instructions in current dispatch bundle
      //
      // Return value:
      // Return "true" (stall) if the Active List does not have enough
      // space for all instructions in the dispatch bundle.
      /////////////////////////////////////////////////////////////////////
      //bool stall_dispatch(uint64_t bundle_inst);

      /////////////////////////////////////////////////////////////////////
      // This function dispatches a single instruction into the Active
      // List.
      //
      // Inputs:
      // 1. dest_valid: If 'true', the instr. has a destination register,
      //    otherwise it does not. If it does not, then the log_reg and
      //    phys_reg inputs should be ignored.
      // 2. log_reg: Logical register number of the instruction's
      //    destination.
      // 3. phys_reg: Physical register number of the instruction's
      //    destination.
      // 4. load: If 'true', the instr. is a load, otherwise it isn't.
      // 5. store: If 'true', the instr. is a store, otherwise it isn't.
      // 6. branch: If 'true', the instr. is a branch, otherwise it isn't.
      // 7. PC: Program counter of the instruction.
      //
      // Return value:
      // Return the instruction's index in the Active List.
      //
      // Tips:
      //
      // Before dispatching the instruction into the Active List, assert
      // that the Active List isn't full: it is the user's responsibility
      // to avoid a structural hazard by calling stall_dispatch()
      // in advance.
      /////////////////////////////////////////////////////////////////////
      //uint64_t dispatch_inst(bool dest_valid,
            //uint64_t log_reg,
            //uint64_t phys_reg,
            //bool load,
          //bool store,
            //bool branch,
            //uint64_t PC);


      //////////////////////////////////////////
      // Functions related to Schedule Stage. //
      //////////////////////////////////////////

      /////////////////////////////////////////////////////////////////////
      // Test the ready bit of the indicated physical register.
      // Returns 'true' if ready.
      /////////////////////////////////////////////////////////////////////
      bool is_ready(uint64_t phys_reg);

      /////////////////////////////////////////////////////////////////////
      // Clear the ready bit of the indicated physical register.
      /////////////////////////////////////////////////////////////////////
      void clear_ready(uint64_t phys_reg);

      /////////////////////////////////////////////////////////////////////
      // Set the ready bit of the indicated physical register.
      /////////////////////////////////////////////////////////////////////
      void set_ready(uint64_t phys_reg);

      //////////////////////////////////////////
      // Functions related to Reg. Read Stage.//
      //////////////////////////////////////////

      /////////////////////////////////////////////////////////////////////
      // Return the contents (value) of the indicated physical register.
      /////////////////////////////////////////////////////////////////////
      uint64_t read(uint64_t phys_reg);


      //////////////////////////////////////////
      // Functions related to Writeback Stage.//
      //////////////////////////////////////////

      /////////////////////////////////////////////////////////////////////
      // Write a value into the indicated physical register.
      /////////////////////////////////////////////////////////////////////
      void write(uint64_t phys_reg, uint64_t value);

      /////////////////////////////////////////////////////////////////////
      // Set the completed bit of the indicated entry in the Active List.
      /////////////////////////////////////////////////////////////////////
      //void set_complete(uint64_t AL_index);

      /////////////////////////////////////////////////////////////////////
      // This function is for handling branch resolution.
      //
      // Inputs:
      // 1. AL_index: Index of the branch in the Active List.
      // 2. branch_ID: This uniquely identifies the branch and the
      //    checkpoint in question.  It was originally provided
      //    by the checkpoint function.
      // 3. correct: 'true' indicates the branch was correctly
      //    predicted, 'false' indicates it was mispredicted
      //    and recovery is required.
      //
      // Outputs: none.
      //
      // Tips:
      //
      // While recovery is not needed in the case of a correct branch,
      // some actions are still required with respect to the GBM and
      // all checkpointed GBMs:
      // * Remember to clear the branch's bit in the GBM.
      // * Remember to clear the branch's bit in all checkpointed GBMs.
      //
      // In the case of a misprediction:
      // * Restore the GBM from the checkpoint. Also make sure the
      //   mispredicted branch's bit is cleared in the restored GBM,
      //   since it is now resolved and its bit and checkpoint are freed.
      // * You don't have to worry about explicitly freeing the GBM bits
      //   and checkpoints of branches that are after the mispredicted
      //   branch in program order. The mere act of restoring the GBM
      //   from the checkpoint achieves this feat.
      // * Restore other state using the branch's checkpoint.
      //   In addition to the obvious state ...  *if* you maintain a
      //   freelist length variable (you may or may not), you must
      //   recompute the freelist length. It depends on your
      //   implementation how to recompute the length.
      //   (Note: you cannot checkpoint the length like you did with
      //   the head, because the tail can change in the meantime;
      //   you must recompute the length in this function.)
      // * Set the corresponding misprediction bit in the active list.
      //   (While not needed in the context of checkpoint-based recovery,
      //   the misprediction bit can be used to model other branch
      //   misprediction recovery implementations if so desired.)
      /////////////////////////////////////////////////////////////////////
      void resolve(uint64_t AL_index,
            uint64_t branch_ID,
            bool correct);

      //////////////////////////////////////////
      // Functions related to Retire Stage.   //
      //////////////////////////////////////////

      /////////////////////////////////////////////////////////////////////
      // This function allows the caller to examine the instruction at the head
      // of the Active List.
      //
      // Input arguments: none.
      //
      // Return value:
      // * Return "true" if the Active List is NOT empty, i.e., there
      //   is an instruction at the head of the Active List.
      // * Return "false" if the Active List is empty, i.e., there is
      //   no instruction at the head of the Active List.
      //
      // Output arguments:
      // Simply return the following contents of the head entry of
      // the Active List.  These are don't-cares if the Active List
      // is empty (you may either return the contents of the head
      // entry anyway, or not set these at all).
      // * completed bit
      // * misprediction bit
      // * exception bit
      // * load flag (indicates whether or not the instr. is a load)
      // * store flag (indicates whether or not the instr. is a store)
      // * branch flag (indicates whether or not the instr. is a branch)
      // * program counter of the instruction
      ///////////////////////////////////////////////////////////////////
      //bool precommit(bool &completed, bool &misprediction, bool &exception,
            //bool &load, bool &store, bool &branch, uint64_t &PC);

      /////////////////////////////////////////////////////////////////////
      // This function attempts to commit the instruction at the head
      // of the Active List.
      //
      // Input arguments: none.
      //
      // Return value:
      // * Return "true" if the Active List is NOT empty, i.e., there
      //   is an instruction at the head of the Active List.
      // * Return "false" if the Active List is empty, i.e., there is
      //   no instruction at the head of the Active List.
      //
      // Output arguments:
      // Simply return the following contents of the head entry of
      // the Active List.  These are don't-cares if the Active List
      // is empty (you may either return the contents of the head
      // entry anyway, or not set these at all).
      // * completed bit
      // * misprediction bit
      // * exception bit
      // * load flag (indicates whether or not the instr. is a load)
      // * store flag (indicates whether or not the instr. is a store)
      // * branch flag (indicates whether or not the instr. is a branch)
      // * program counter of the instruction
      //
      // When this function is called:
      // 1. Set the return value and output arguments, as discussed above,
      //    before attempting to commit the head instruction (if any).
      // 2. If the Active List is empty, return from the function.
      //    Otherwise continue...
      // 3. If the head instruction is not completed yet, return from
      //    the function.  Otherwise continue...
      // 4. If this step is reached, the Active List has a head instr.
      //    and it is completed.  Either squash or commit:
      //    * Squash if either (a) the exception bit is true, OR
      //      (b) the misprediction bit is true AND this is NOT
      //      a branch instruction (since a mispredicted branch
      //      was recovered earlier from its checkpoint).
      //    * Otherwise commit the instruction.
      //
      // Note: Squashing from the head instruction means rolling back to
      // the committed state of the machine just prior to the head
      // instruction.  Think about which structures in your renamer
      // need to be restored, and how.
      /////////////////////////////////////////////////////////////////////
      //bool commit(bool &completed, bool &misprediction, bool &exception,
            //bool &load, bool &store, bool &branch,
            //uint64_t &PC);

      //////////////////////////////////////////
      // Functions not tied to specific stage.//
      //////////////////////////////////////////

      /////////////////////////////////////////////////////////////////////
      // Set the exception bit of the indicated entry in the Active List.
      /////////////////////////////////////////////////////////////////////
      void set_exception(uint64_t AL_index);

      /////////////////////////////////////////////////////////////////////
      // Query the exception bit of the indicated entry in the Active List.
      /////////////////////////////////////////////////////////////////////
      bool get_exception(uint64_t AL_index);

      //////////////////////////////////////////////////////////////////////
      // Squash the renamer, exactly the same as if there were an exception
      // at the head of the Active List.  After this function is called,
      // all renamer state should be consistent with an empty pipeline.
      /////////////////////////////////////////////////////////////////////
      void squash();

      /////////////////////////////////////////////////////////////////////
      // Set the misprediction bit of the indicated entry in the Active
      // List.  For example, a store may detect a prematurely executed
      // dependent load instruction and want to set the load's
      // misprediction bit.  Or, an instruction may have been
      // value-predicted incorrectly.
      /////////////////////////////////////////////////////////////////////
      void set_misprediction(uint64_t AL_index);
	void set_complete_instr_decrement(uint64_t chk_fifo_index);
	void set_complete(bool src_valid, unsigned int src_phy_index);
	bool precommit_chk();
	void commit_chk();
	void reclaim_PRF();
	void store_increment();
	void print_FL();
	bool stall_branch(bool amo);
};
