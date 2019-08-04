#include <inttypes.h>
#include <cassert>
#include <stdio.h>
#include <iostream>
#include <renamer.h>
#define DEBUG 1
#define DEBUGM 0

#define CHECKPOINT_FIFO_SIZE 20
#define MAX_INSTR 32

renamer::renamer(uint64_t n_log_regs,
	uint64_t n_phys_regs,
	uint64_t n_branches)
{
//<<<<<<< HEAD
	PRF_reclaim_counter=0;
//=======
	
//>>>>>>> 4f751980690d6293df8659bec8d053c49b0cd1c7
	assert(n_log_regs < n_phys_regs);
	assert(n_branches <= 64 && n_branches >= 1);
	//AMT_size = n_log_regs;
	RMT_size = n_log_regs;
	FL_size = n_phys_regs - n_log_regs;
	PRF_size = n_phys_regs;
	//AL_size = FL_size;
	CHK_size = n_branches;
	//GBM = 0;
	inst_count=0;

	RMT = new uint64_t[RMT_size];
	for (uint64_t i = 0; i < RMT_size; i++)
	{
		RMT[i] = i;
	}
	//AMT = new uint64_t[AMT_size];
	/*for (uint64_t i = 0; i < AMT_size; i++)
	{
		AMT[i] = i;
	}*/

	free_list.list = new uint64_t[FL_size];
	for (uint64_t i = 0; i < FL_size; i++)
	{
		free_list.list[i] = i+RMT_size;
	}
	free_list.head = 0;
	free_list.tail = 0;
	free_list.empty = false;
	free_list.full = true;
	/*active_list.list = new AL_entry[AL_size];
	active_list.empty = true;
	active_list.full = false;
	active_list.head = 0;
	active_list.tail = 0;
	for (uint64_t i = 0; i < AL_size; i++)
	{
		(active_list.list + i)->branch_flag = false;
		(active_list.list + i)->complete = false;
		(active_list.list + i)->dest_flag = false;
		(active_list.list + i)->exception = false;
		(active_list.list + i)->load_flag = false;
		(active_list.list + i)->log_n_reg = 0;
		(active_list.list + i)->misprediction = false;
		(active_list.list + i)->PC = 0;
		(active_list.list + i)->phy_n_reg = 0;
		(active_list.list + i)->store_flag = false;
	}

	checkpoints = new CHK[CHK_size];
	for (uint64_t i = 0; i < CHK_size; i++)
	{
		checkpoints[i].SMT = new uint64_t[RMT_size];
		checkpoints[i].check_head = 0;
		checkpoints[i].check_GBM = 0;
	}*/

	PRF = new uint64_t[PRF_size];
	PRF_ready = new bool[PRF_size];
	/////////////////////////////////////////////////////////////
	PRF_usage_counter = new uint64_t[PRF_size];
	PRF_unmapped = new bool[PRF_size];
	PRF_free= new bool [PRF_size];
	////////////////////////////////////////////////////////////
	for (uint64_t i = 0; i < PRF_size; i++)
	{
		PRF[i] = 0;
		PRF_ready[i] = false;
		////////////////////////////////////////////////////////////////////////////
		PRF_usage_counter[i]=0;
		PRF_unmapped[i]=true;
		PRF_free[i]=true;
		if(i<RMT_size)
		{
			PRF_unmapped[i]=false;
			PRF_free[i]=false;
		}
		////////////////////////////////////////////////////////////////////////////
		
	}
	//print_FL();
	//instr_between_CHK 
	checkpoint_fifo = new CHK_instr[CHECKPOINT_FIFO_SIZE];
	for (uint64_t i = 0; i < CHECKPOINT_FIFO_SIZE; i++)
	{
		checkpoint_fifo[i].SMT = new uint64_t[RMT_size];
		checkpoint_fifo[i].check_head = 0;
		checkpoint_fifo[i].num_instr = 0;
		checkpoint_fifo[i].PC = 0;
		checkpoint_fifo[i].branch_mispredicted = false;
		checkpoint_fifo[i].mispred_inst_num = 0;
		checkpoint_fifo[i].max_instr = 0;
		checkpoint_fifo[i].num_store=0;
	}

	checkpoint_fifo_head=0;
	checkpoint_fifo_tail=0;
	checkpoint_ID = 0;


	#if DEBUG
	//printf("renamer\n");
	#endif
}

void renamer::copy(uint64_t* target, const uint64_t* source, uint64_t size)
{
	for (uint64_t i = 0; i < size; i++)
	{
		target[i] = source[i];
	}
}

/*void renamer::empty_AL_entry(uint64_t AL_index)
{
	//(active_list.list + AL_index)->branch_flag = false;
	(active_list.list + AL_index)->complete = false;
	//(active_list.list + AL_index)->dest_flag = false;
	(active_list.list + AL_index)->exception = false;
	//(active_list.list + AL_index)->load_flag = false;
	//(active_list.list + AL_index)->log_n_reg = 0;
	(active_list.list + AL_index)->misprediction = false;
	//(active_list.list + AL_index)->PC = 0;
	//(active_list.list + AL_index)->phy_n_reg = 0;
	//(active_list.list + AL_index)->store_flag = false;
}*/

bool renamer::stall_reg(uint64_t bundle_dst)
{

	assert(bundle_dst < FL_size);
	assert(!free_list.empty);
	uint64_t avaliable_space;
	if (free_list.full)
	{
		avaliable_space = FL_size;
	}
	else 
	{ 
		if(free_list.tail < free_list.head)
		{
			avaliable_space = free_list.tail + FL_size - free_list.head;
		}
		else
		{
			avaliable_space = free_list.tail - free_list.head;
		}
	}
	if (avaliable_space <= bundle_dst)
	{
		#if DEBUG
		//printf("stall_reg_true\n");
		#endif
		return true;
	}
	else
	{
		#if DEBUG
		//printf("stall_reg_false\n");
		#endif
		return false;
	}

}

/*bool renamer::stall_branch(uint64_t bundle_branch)
{
	////printf("stall_branch\n");
	assert(bundle_branch <= CHK_size);
	uint64_t one = 1;
	uint64_t blank_space=0;
	for (uint64_t i = 0; i < CHK_size; i++)
	{
		if ((GBM & (one << i)) == 0)
		{
			blank_space++;
		}
	}
	if (blank_space < bundle_branch)
	{
		#if DEBUG
		//printf("stall_branch_true\n");
		#endif
		return true;
	}
	else
	{
		#if DEBUG
		//printf("stall_branch_false\n");
		#endif
		return false;
	}

}*/
bool renamer::stall_branch(bool amo)
{
	if(!amo)
		return false;
	else
        {
		uint64_t blank_space;
		if (checkpoint_fifo_head <= checkpoint_fifo_tail)
		{
			blank_space = checkpoint_fifo_head + CHECKPOINT_FIFO_SIZE - checkpoint_fifo_tail;
		}
		else
		{
			blank_space = checkpoint_fifo_head - checkpoint_fifo_tail;
		}
		if(blank_space>2)
		{
			return false;
		}
		else 
		{
			return true;
		}
	}
}
uint64_t renamer::get_branch_mask()
{
	#if DEBUG
//<<<<<<< HEAD
	//printf("get_branch_mask:%X\n",checkpoint_ID);
//=======
	printf("get_branch_mask:%X\n",checkpoint_ID);
//>>>>>>> 4f751980690d6293df8659bec8d053c49b0cd1c7
	#endif
	return checkpoint_ID;

}

uint64_t renamer::rename_rsrc(uint64_t log_reg)
{
	#if DEBUG
	//printf("rename_rsrc:	src:r%d	rename:p%d\n",log_reg,RMT[log_reg]);
	#endif
	return RMT[log_reg];
}

uint64_t renamer::rename_rdst(uint64_t log_reg)
{

	uint64_t name_dest = free_list.list[free_list.head];
	#if DEBUG
	//printf("rename_rdst:	dst:r%d	rename:p%d\n",log_reg,name_dest);
	#endif
	RMT[log_reg]=name_dest;
	PRF_free[RMT[log_reg]]=false;
	if(free_list.full){free_list.full=false;}
	free_list.head = (free_list.head + 1) % FL_size;
	return name_dest;
}

/*uint64_t renamer::checkpoint()
{

	uint64_t free_location;
	uint64_t one = 1;
	for (uint64_t i = 0; i < CHK_size; i++)
	{
		if ((GBM & (one << i)) == 0)
		{
			free_location = i;
			break;
		}
	}
	copy(checkpoints[free_location].SMT, RMT, RMT_size);
	checkpoints[free_location].check_head = free_list.head;
	checkpoints[free_location].check_GBM = GBM;

	GBM = GBM | (one << free_location);
	#if DEBUG
	//printf("checkpoint:	branch_ID:%d\n",free_location);
	#endif
	return free_location;
}*/

void renamer::increment_usage_counter(uint64_t log_reg)
{
	
	PRF_usage_counter[RMT[log_reg]]++;
	#if DEBUG
	//printf("increase usage counter of PRF[%d], usage counter is %d\n ",RMT[log_reg],PRF_usage_counter[RMT[log_reg]]);
	#endif
}

void renamer::reset_unmap_bit(uint64_t log_reg)
{	
	#if DEBUG
	//printf("reset unmap_bit of PRF[%d]\n",RMT[log_reg]);
	#endif
	PRF_unmapped[RMT[log_reg]]=false;
}

void renamer::set_unmap_bit(uint64_t log_reg)
{
	#if DEBUG
	//printf("set unmap_bit of PRF[%d]\n",RMT[log_reg]);
	#endif
	PRF_unmapped[RMT[log_reg]]=true;
}

/*bool renamer::add_checkpoint()
{
	assert((checkpoint_fifo_tail+1)%CHECKPOINT_FIFO_SIZE!=checkpoint_fifo_head);
	copy(RMT, checkpoint_fifo[checkpoint_fifo_tail].SMT, RMT_size);
}*/
void renamer::add_checkpoint(uint64_t current_pc,uint64_t& current_insn_checkpoint_ID,bool amo)
{
	bool checkpoint_assigned=false;
	bool current_checkpoint_full=false;
	uint64_t current_checkpoint_ID;
	if(checkpoint_fifo_tail!=checkpoint_fifo_head)
	{
		if(checkpoint_fifo_tail==0)
		{current_checkpoint_ID=CHECKPOINT_FIFO_SIZE-1;}
		else
		{current_checkpoint_ID=checkpoint_fifo_tail-1;}
		current_checkpoint_full=checkpoint_fifo[current_checkpoint_ID].max_instr>=MAX_INSTR;
	}
	else if(checkpoint_fifo_tail == checkpoint_fifo_head)
	{
		current_checkpoint_full=true;
	}
	
	
	if(current_checkpoint_full || amo)
	{
		uint64_t blank_space;
		if (checkpoint_fifo_head <= checkpoint_fifo_tail)
		{
			blank_space = checkpoint_fifo_head + CHECKPOINT_FIFO_SIZE - checkpoint_fifo_tail;
		}
		else
		{
			blank_space = checkpoint_fifo_head - checkpoint_fifo_tail;
		}
		
		if (blank_space>1)
		{
			copy(checkpoint_fifo[checkpoint_fifo_tail].SMT, RMT,  RMT_size);
			for(uint64_t i=0;i<RMT_size;i++)
			{
				PRF_usage_counter[RMT[i]]++;
			}
			checkpoint_fifo[checkpoint_fifo_tail].num_instr=0;
			checkpoint_fifo[checkpoint_fifo_tail].max_instr=0;
			checkpoint_fifo[checkpoint_fifo_tail].num_instr++;
			checkpoint_fifo[checkpoint_fifo_tail].max_instr++;
			checkpoint_fifo[checkpoint_fifo_tail].PC=current_pc;
			current_checkpoint_ID=checkpoint_fifo_tail;
			current_insn_checkpoint_ID=current_checkpoint_ID;
			checkpoint_fifo_tail=(checkpoint_fifo_tail+1)%CHECKPOINT_FIFO_SIZE;
			//printf("new_checkpoint\n");
		}
		else 
		{
			assert(!amo);
			current_insn_checkpoint_ID=current_checkpoint_ID;
			checkpoint_fifo[current_checkpoint_ID].num_instr++;
			checkpoint_fifo[current_checkpoint_ID].max_instr++;
		}
	}
	
	else //if not full
	{
		current_insn_checkpoint_ID=current_checkpoint_ID;
		checkpoint_fifo[current_checkpoint_ID].num_instr++;
		checkpoint_fifo[current_checkpoint_ID].max_instr++;
	}
}
bool renamer::add_checkpoint()
{
      uint64_t blank_space;
      bool checkpoint_assigned;
//      printf("instr_between_CHK: %d", instr_between_CHK);
      if(instr_between_CHK <= MAX_INSTR)
      {
	instr_between_CHK++;
	checkpoint_fifo[checkpoint_ID].num_instr++;
	checkpoint_fifo[checkpoint_ID].max_instr++;
	checkpoint_assigned = false;
      }
      else
      {
	if (checkpoint_fifo_head <= checkpoint_fifo_tail)
	{
		blank_space = checkpoint_fifo_head + CHECKPOINT_FIFO_SIZE - checkpoint_fifo_tail;
	}
	else
	{
		blank_space = checkpoint_fifo_head - checkpoint_fifo_tail;
	}
	if (blank_space > 1)
//<<<<<<< HEAD
	{	
		#if DEBUG
		//printf("add_checkpoint\n");
		#endif
//=======
	//{
//		printf("checkpoint added: checkpoint_fifo_tail = %d", checkpoint_fifo_tail);
//>>>>>>> 4f751980690d6293df8659bec8d053c49b0cd1c7
		copy(checkpoint_fifo[checkpoint_fifo_tail].SMT, RMT,  RMT_size);
		for (int i=0; i<RMT_size; i++)
		{
			PRF_usage_counter[RMT[i]]++;
		}
		instr_between_CHK = 1;
		checkpoint_ID = checkpoint_fifo_tail;
		checkpoint_fifo[checkpoint_ID].num_instr++;
		checkpoint_fifo[checkpoint_ID].max_instr++;
		checkpoint_fifo_tail = (checkpoint_fifo_tail + 1)%CHECKPOINT_FIFO_SIZE;
		checkpoint_assigned = true;
	}
	else
	{
		instr_between_CHK++;
		checkpoint_fifo[checkpoint_ID].num_instr++;
		checkpoint_fifo[checkpoint_ID].max_instr++;
		checkpoint_assigned = false;
	}

      }
      return checkpoint_assigned;
}

void renamer::assign_PC(uint64_t current_pc)
{
	checkpoint_fifo[checkpoint_ID].PC = current_pc;
}

/*bool renamer::stall_dispatch(uint64_t bundle_inst)
{
	#if DEBUG
	//printf("stall_dispatch\n");
	#endif
	assert(bundle_inst < AL_size);
	uint64_t blank_space;
	if (active_list.head <= active_list.tail)
	{
		blank_space = active_list.head + AL_size - active_list.tail;
	}
	else
	{
		blank_space = active_list.head - active_list.tail;
	}
	if (blank_space <= bundle_inst)
	{
		#if DEBUG
		//printf("stall_dispatch_true\n");
		#endif
		return true;
	}
	else
	{
		#if DEBUG
		//printf("stall_dispatch_false\n");
		#endif
		return false;
	}
}*/

/*uint64_t renamer::dispatch_inst(bool dest_valid,
	uint64_t log_reg,
	uint64_t phys_reg,
	bool load,
	bool store,
	bool branch,
	uint64_t PC)
{

	assert(active_list.full == false);
	uint64_t location;
	location = active_list.tail;
	active_list.list[location].log_n_reg = log_reg;
	active_list.list[location].phy_n_reg = phys_reg;
	active_list.list[location].load_flag = load;
	active_list.list[location].store_flag = store;
	active_list.list[location].branch_flag = branch;
	active_list.list[location].dest_flag = dest_valid;
	active_list.list[location].PC = PC;
	active_list.tail = (active_list.tail + 1) % AL_size;
	//if(dest_valid==true)
	//{
	//	clear_ready(phys_reg);
	//}
	if(dest_valid){
	#if DEBUG
	//printf("dispatch_inst %d:	dest:%d	current_mapping:%d	location_in_AL:%d	PC=%d\n",inst_count,log_reg,phys_reg,location,PC);
	#endif
	}
	else{
	#if DEBUG
	//printf("dispatch_inst %d:	dest:-1	current_mapping:%d	location_in_AL:%d	PC=%d\n",inst_count,log_reg,location,PC);
	#endif
	}
	inst_count++;
	return location;
}*/

bool renamer::is_ready(uint64_t phys_reg)
{
	#if DEBUG
	//printf("PRF[%d] is_ready:%d\n",phys_reg,PRF_ready[phys_reg]);
	#endif
	return PRF_ready[phys_reg];
}

void renamer::clear_ready(uint64_t phys_reg)
{
	#if DEBUG
	//printf("PRF[%d] clear_ready\n",phys_reg);
	#endif
	PRF_ready[phys_reg] = false;
}

void renamer::set_ready(uint64_t phys_reg)
{
	#if DEBUG
	//printf("PRF[%d] set_ready	value=%d\n",phys_reg,PRF[phys_reg]);
	#endif
	PRF_ready[phys_reg] = true;
}

uint64_t renamer::read(uint64_t phys_reg)
{
	#if DEBUG
	//printf("read PRF[%d]=%d		is it ready?:%d\n",phys_reg,PRF[phys_reg],PRF_ready[phys_reg]);
	#endif
	return PRF[phys_reg];
}

void renamer::write(uint64_t phys_reg, uint64_t value)
{
	#if DEBUG
	//printf("write PRF[%d]=%d\n",phys_reg,value);
	#endif
	PRF[phys_reg] = value;
	set_ready(phys_reg);

}

/*void renamer::set_complete(uint64_t AL_index)
{
	#if DEBUG
	//printf("set_complete for AL[%d]		Now PC=%d complete!     value in PRF[%d]\n",AL_index,active_list.list[AL_index].PC,active_list.list[AL_index].phy_n_reg);
	#endif
	active_list.list[AL_index].complete = true;
}*/

void renamer::set_complete(bool src_valid, unsigned int src_phy_index)
{
	#if DEBUG
	//printf("set_complete for p%d\n",src_phy_index);
	#endif
	if(src_valid)
	{
		PRF_usage_counter[src_phy_index]--;
	}	
//	printf("set complete");
}

void renamer::set_complete_instr_decrement(uint64_t chk_fifo_index)
{
	
	#if DEBUG
	//printf("set_complete_instr_decrement in fifo index %d\n",chk_fifo_index);
	#endif
	checkpoint_fifo[chk_fifo_index].num_instr--;
}

/*void renamer::resolve(uint64_t AL_index,
	uint64_t branch_ID,
	bool correct)
{

	uint64_t one = 1;
	if (correct)
	{
		GBM = GBM & (~(one << branch_ID));
		for (uint64_t i = 0; i < CHK_size; i++)
		{
			if ((GBM & (one << i)) != 0)
			{
				checkpoints[i].check_GBM = checkpoints[i].check_GBM & (~(one << branch_ID));
			}
		}
		#if DEBUG
		//printf("resolve for branch_ID %d	correct! this branch is in AL[%d]\n",branch_ID,AL_index);
		#endif
	}

	else
	{
		GBM = checkpoints[branch_ID].check_GBM;
		GBM = GBM & (~(one << branch_ID));
		copy(RMT, checkpoints[branch_ID].SMT, RMT_size);
		free_list.head = checkpoints[branch_ID].check_head;
		//set_misprediction(AL_index);
		#if DEBUG
		//printf("set mispre for AL[%d], Branch ID is %d\n",AL_index,branch_ID);
		#endif
		uint64_t old_tail=active_list.tail;
		uint64_t step;

		active_list.tail=(AL_index+1)%AL_size;
		if(old_tail>=active_list.tail)
		{step=old_tail-active_list.tail;}
		else
		{step=old_tail+AL_size-active_list.tail;}
		for(uint64_t i=0;i<step;i++)
		{
			empty_AL_entry((active_list.tail+i)%AL_size);
		}

		#if DEBUG
		//printf("resolve for branch ID %d	mispre! AL_tail back from %d to %d\n",branch_ID,old_tail,active_list.tail);
		#endif
	}
}*/

/*bool renamer::precommit(bool &completed, bool &misprediction, bool &exception,
	bool &load, bool &store, bool &branch, uint64_t &PC)
{

	if (active_list.head == active_list.tail)
	{
		#if DEBUG
		//printf("precommit false (AL is empty)\n");
		#endif
		return false;
	}
	else
	{
		completed = active_list.list[active_list.head].complete;
		misprediction = active_list.list[active_list.head].misprediction;
		exception = active_list.list[active_list.head].exception;
		load = active_list.list[active_list.head].load_flag;
		store = active_list.list[active_list.head].store_flag;
		branch = active_list.list[active_list.head].branch_flag;
		PC = active_list.list[active_list.head].PC;
		#if DEBUG
		//printf("precommit true		head=%d\n",active_list.head);
		#endif
		return true;
	}
}*/

bool renamer::precommit_chk()
{
	
	//#//if DEBUG
	
	//endif
	if(checkpoint_fifo[checkpoint_fifo_head].num_instr==0) {
		//printf("precommit_chk true head=%d\n",checkpoint_fifo_head);
//		printf("the checkpoint number is %d and the number of instructions in %d\n", checkpoint_fifo_head, checkpoint_fifo[checkpoint_fifo_head].num_instr);
		return true;
	}
	else {
		//printf("precommit_chk false head=%d\n",checkpoint_fifo_head);
		return false;
	}
}

void renamer::commit_chk()
{
	#if DEBUG
	//printf("commit_chk\n");
	#endif

	//bool latest_chk;
	//latest_chk=(((checkpoint_fifo_head+1)%CHECKPOINT_FIFO_SIZE)==checkpoint_fifo_tail);
	//if(checkpoint_fifo[checkpoint_fifo_head].num_instr==0 && !latest_chk)
	//{
		for (uint64_t i=0;i<RMT_size;i++)
		{
			PRF_usage_counter[checkpoint_fifo[checkpoint_fifo_head].SMT[i]]--;
		}
		checkpoint_fifo[checkpoint_fifo_head].max_instr = 0;
		checkpoint_fifo_head=(checkpoint_fifo_head+1)%CHECKPOINT_FIFO_SIZE;
		
		//return true;
	
	//else
	//{
		//return false;
	//}
}

void renamer::print_FL()
{
	for(uint64_t i=0;i<FL_size;i++)
	{
		printf("%d:%d free=%d",i,free_list.list[i],PRF_free[free_list.list[i]]);
		if(i==free_list.head) {printf("<------HEAD");}
		if(i==free_list.tail) {printf("<------TAIL");}
		printf("\n");
	}
}

void renamer::reclaim_PRF()
{
	#if DEBUG
	//printf("reclaim_PRF\n");
	#endif
//	printf("------------------------------------before reclaim-----------------------------------------\n");
//	print_FL();
	if(!free_list.full){
	for(uint64_t i=0;i<PRF_size;i++)
		{
			if(PRF_unmapped[i] && PRF_usage_counter[i]==0 && !PRF_free[i])
			{
				PRF_free[i]=true;
				free_list.list[free_list.tail] = i;
				free_list.tail = (free_list.tail + 1) % FL_size;
				if(free_list.tail==free_list.head)
				{
					free_list.full=true;
					break;
				}
			}
		}
	}
//	printf("------------------------------------after reclaim-----------------------------------------\n");
//	print_FL();
	//PRF_reclaim_counter++;
	////printf("PRF called %d times\n",PRF_reclaim_counter);
}

/*bool renamer::commit(bool &completed, bool &misprediction, bool &exception,
	bool &load, bool &store, bool &branch, uint64_t &PC)
{

	if (active_list.head == active_list.tail)
	{
		#if DEBUG
		//printf("commit false AL empty\n");
		#endif
		return false;
	}
	else
	{
		completed = active_list.list[active_list.head].complete;
		misprediction = active_list.list[active_list.head].misprediction;
		exception = active_list.list[active_list.head].exception;
		load = active_list.list[active_list.head].load_flag;
		store = active_list.list[active_list.head].store_flag;
		branch = active_list.list[active_list.head].branch_flag;
		PC = active_list.list[active_list.head].PC;

		if (active_list.list[active_list.head].complete == false)
		{
			#if DEBUG
			//printf("commit true head not complete\n");
			#endif
			return true;
		}
		else
		{
			if (active_list.list[active_list.head].exception || (active_list.list[active_list.head].misprediction && active_list.list[active_list.head].branch_flag == false))
			{
				#if DEBUG
				//printf("exception occur at head:%d\n",active_list.head);
				#endif
				squash();
			}
			else
			{
				if (active_list.list[active_list.head].dest_flag)
				{
					free_list.tail = (free_list.tail + 1) % FL_size;
					free_list.list[free_list.tail] = AMT[active_list.list[active_list.head].log_n_reg];
					//PRF[AMT[active_list.list[active_list.head].log_n_reg]]=0;
					AMT[active_list.list[active_list.head].log_n_reg] = active_list.list[active_list.head].phy_n_reg;
					#if DEBUG
					//printf("commit head: %d		free_list_tail move to %d  PC=%d\n",active_list.head,free_list.tail,active_list.list[active_list.head].PC);
					#endif
					empty_AL_entry(active_list.head);
					active_list.head=(active_list.head+1)%AL_size;

				}
				else
				{
					#if DEBUG
					//printf("commit head: %d PC=%d\n",active_list.head,active_list.list[active_list.head].PC);
					#endif
					empty_AL_entry(active_list.head);
					active_list.head=(active_list.head+1)%AL_size;
				}
			}
		}
		return true;
	}
}*/

void renamer::set_exception(uint64_t AL_index)
{
	#if DEBUG
//<<<<<<< HEAD
	////printf("set_exception for AL[%d]\n",AL_index);
	//printf("set_exception\n");
//=======
	//printf("set_exception for AL[%d]\n",AL_index);
	printf("set_exception\n");
//>>>>>>> 4f751980690d6293df8659bec8d053c49b0cd1c7
	#endif
	//active_list.list[AL_index].exception = true;
}
bool renamer::get_exception(uint64_t AL_index)
{
	#if DEBUG
//<<<<<<< HEAD
	////printf("get_exception of AL[%d]:%d\n",AL_index,active_list.list[AL_index].exception);
	//printf("get_exception\n");
//=======
	//printf("get_exception of AL[%d]:%d\n",AL_index,active_list.list[AL_index].exception);
	printf("get_exception\n" );
//>>>>>>> 4f751980690d6293df8659bec8d053c49b0cd1c7
	#endif
	return false;
	//return active_list.list[AL_index].exception;
}
void renamer::squash()
{
	#if DEBUG
	//printf("squash\n");
	#endif
	//active_list.tail = active_list.head;
	/*for(uint64_t i=0;i<AL_size;i++)
	{
		empty_AL_entry(i);
	}*/
	copy(RMT, checkpoint_fifo[checkpoint_fifo_head].SMT, RMT_size);
	checkpoint_fifo_tail = checkpoint_fifo_head;
	free_list.head=free_list.tail;
	for(int i=0; i<PRF_size; i++)
	{
		PRF_unmapped[i] = true;
		PRF_free[i] = true;
	}
	for (int i=0; i<RMT_size; i++)
	{
		PRF_unmapped[checkpoint_fifo[checkpoint_fifo_head].SMT[i]] = false;
		PRF_free[checkpoint_fifo[checkpoint_fifo_head].SMT[i]] = false;
		assert(PRF_unmapped[checkpoint_fifo[checkpoint_fifo_head].SMT[i]]!=true);
	}
	for(int i =0; i<PRF_size; i++)
	{
		if(PRF_unmapped[i])
		{
			PRF_free[i] = true;
			free_list.list[free_list.head] = i;
			free_list.head = (free_list.head + 1)%FL_size;
		}
			
	}
	for(int i=0; i<CHECKPOINT_FIFO_SIZE; i++)
	{
		checkpoint_fifo[i].max_instr = 0;
		checkpoint_fifo[i].num_instr = 0;
	}
	checkpoint_fifo_head = 0;
	checkpoint_fifo_tail = 0;
	free_list.head=free_list.tail=0;
	free_list.full=true;
}
void renamer::set_misprediction(uint64_t AL_index)
{
	#if DEBUGM
	//printf("set_misprediction\n");
	#endif
	//active_list.list[AL_index].misprediction = true;
}

void renamer::store_increment()
{
	checkpoint_fifo[checkpoint_fifo_tail].num_store++;
}
