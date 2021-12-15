#include "qd_sched.h"

// The latest millis() value when loop() was entered
static unsigned long entry_time;
// Elapsed time (time spent doing stuff) during the last loop() run
static unsigned long elapsed_time;
// Tasks to run
static task_t tasks[SCHED_NUM_TASKS] = {0};
// Tasklets to run
static rt_tasklet_t tasklets[SCHED_NUM_TASKLETS] = {0};

int sched_put_rt_tasklet(void (*taskletFunction)(void*), void* param, unsigned long delayMillis)
{
   size_t emptyIdx = 0;
  
  // search for the first empty task entry within tasks
  while(tasklets[emptyIdx].taskletFunc != 0)
  {
    emptyIdx++;
    if(emptyIdx == SCHED_NUM_TASKLETS)
      return -1;
  }

  tasklets[emptyIdx].taskletFunc = taskletFunction;
  tasklets[emptyIdx].param = param;
  tasklets[emptyIdx].reqMillis = millis();
  tasklets[emptyIdx].delayMillis = delayMillis;
  return emptyIdx;
}

int sched_put_task(void (*taskFunction)(void), unsigned long rate, bool run_immediately)
{
  size_t emptyIdx = 0;
  
  // search for the first empty task entry within tasks
  while(tasks[emptyIdx].taskFunc != 0)
  {
    emptyIdx++;
    if(emptyIdx == SCHED_NUM_TASKS)
      return -1;
  }

  tasks[emptyIdx].taskFunc = taskFunction;
  tasks[emptyIdx].rateMillis = rate;
  tasks[emptyIdx].lastRunMillis = run_immediately ? -1*rate : 0;
  return emptyIdx;
}

int sched_put_taskID(size_t id, void (*taskFunction)(void), unsigned long rate, bool run_immediately)
{
  if (id >= SCHED_NUM_TASKS)
    return -1;

  tasks[id].taskFunc = taskFunction;
  tasks[id].rateMillis = rate;
  tasks[id].lastRunMillis = run_immediately ? -1*rate : 0;
  return id;
}

uint8_t sched_get_CPU_usage()
{
  return (100*elapsed_time)/SCHED_TICK_MS;
}

int sched_get_taskID(void (*taskFunction)(void))
{
  int id = -1;

  for (size_t i = 0; i < SCHED_NUM_TASKS; i++)
  {
    if(tasks[i].taskFunc == taskFunction)
      id = i;
  }
  return id;
}

void sched_reschedule_taskID(size_t id, unsigned long when)
{
  if ((id >= SCHED_NUM_TASKS) || (!tasks[id].taskFunc))
    return;

  when = max(when, tasks[id].rateMillis); // make sure lastRunMillis is always >0
  tasks[id].lastRunMillis = when - tasks[id].rateMillis;
}
   
void loop()
{
#ifdef SCHED_USE_BLINKENLIGHT
  digitalWrite(LED_BUILTIN, LOW);
#endif

  entry_time = millis();

  // tasklets have higher priority than tasks
  for(size_t i = 0; i < SCHED_NUM_TASKLETS; i++)
  {
    if (tasklets[i].taskletFunc && (entry_time - tasklets[i].reqMillis) >= tasklets[i].delayMillis)
    {
      (*tasklets[i].taskletFunc)(tasklets[i].param);
      tasklets[i].taskletFunc = NULL; // Once the tasklet is run forget about it
    }
  }

  for(size_t i = 0; i < SCHED_NUM_TASKS; i++)
  {
    if (tasks[i].taskFunc && (entry_time - tasks[i].lastRunMillis) >= tasks[i].rateMillis)
    {
      tasks[i].lastRunMillis = millis();
      (*tasks[i].taskFunc)();
    }
  }

#ifdef SCHED_USE_BLINKENLIGHT
  digitalWrite(LED_BUILTIN, HIGH);
#endif
  // sleep until the next full tick
  elapsed_time = millis() - entry_time;
  delay(SCHED_TICK_MS - (elapsed_time % SCHED_TICK_MS));
}
