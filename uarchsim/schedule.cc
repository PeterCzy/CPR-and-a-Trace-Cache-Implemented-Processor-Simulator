#include "pipeline.h"


void pipeline_t::schedule() {
//<<<<<<< HEAD
   //printf("--------------------------------------------------schedule stage-------------------------------------------------------------\n");
//=======
//   printf("schedule stage\n");
//>>>>>>> 4f751980690d6293df8659bec8d053c49b0cd1c7
   IQ.select_and_issue(issue_width, Execution_Lanes);	// Issue instructions from unified IQ to the Execution Lanes.
   //printf("--------------------------------------------------end schedule stage-------------------------------------------------------------\n");
}
