#include "qd_sched.h"

// The latest millis() value when loop() was entered
static unsigned long entry_time;
// The latest millis() when the idle loop was entered
static unsigned long idle_time;
// Tasks to run
static task_t tasks[SCHED_NUM_TASKS] = {0};
// number of tasks stored in tasks array
static int num_tasks = 0;
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
  if (num_tasks == SCHED_NUM_TASKS)
    return -1; // no more room for another task
    
  // For maximum scheduling accuracy, insert tasks in the order of increasing rate
  int i = num_tasks -1;

  while (i >= 0 && tasks[i].rateMillis > rate)
  {
    tasks[i+1].taskFunc = tasks[i].taskFunc;
    tasks[i+1].rateMillis = tasks[i].rateMillis;
    tasks[i+1].lastRunMillis = tasks[i].lastRunMillis;
    i--;
  }
  int task_id = i+1;
  
  tasks[task_id].taskFunc = taskFunction;
  tasks[task_id].rateMillis = rate;
  tasks[task_id].lastRunMillis = run_immediately ? -1*rate : 0;
  num_tasks++;
  return task_id;
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

  for(size_t i = 0; i < num_tasks; i++)
  {
    if ((entry_time - tasks[i].lastRunMillis) >= tasks[i].rateMillis)
    {
      tasks[i].lastRunMillis = millis();
      (*tasks[i].taskFunc)();
    }
  }

#ifdef SCHED_USE_BLINKENLIGHT
  digitalWrite(LED_BUILTIN, HIGH);
#endif

  // idle loop (seep until there's stuff to do)
  unsigned long sleep_ms = ULONG_MAX;
  for(size_t i = 0; i < num_tasks; i++)
  {
    unsigned long ms_delta = millis() - tasks[i].lastRunMillis; // always > 0
    // Make sure to compute ms_to_task correctly in case a task ran for more than its rate
    unsigned long ms_to_task = (ms_delta < tasks[i].rateMillis) ? (tasks[i].rateMillis - ms_delta) : tasks[i].rateMillis;

    if (ms_to_task < (sleep_ms - SLEEP_SLACK_MS))
      sleep_ms = ms_to_task; 
  }
  delay(sleep_ms);
}
