#include "duck_vita_executor.h"
int duck_vita_execute_step(DuckVitaScheduler *scheduler,DuckVitaContextStore *store,
                           DuckVitaGuestStep step,void *opaque) {
 if(!scheduler||!store||!step)return DUCK_EXEC_BAD_ARG;
 /* A running or newly selected ready thread must have a registered context.
    Prevent uninitialized game code from being invoked. */
 int selected=duck_vita_scheduler_next(scheduler);
 if(selected<0)return DUCK_EXEC_NO_THREAD;
 DuckVitaGuestContext state;
 if(duck_vita_context_load(store,scheduler,(unsigned)selected,&state)!=0)
  return DUCK_EXEC_NO_CONTEXT;
 int result=step(&state,opaque);
 /* Persist register changes even on an execution fault, for diagnostics.
    GPR0 is normalized by the context store. */
 if(duck_vita_context_save(store,scheduler,(unsigned)selected,&state)!=0)
  return DUCK_EXEC_BAD_ARG;
 return result;
}
