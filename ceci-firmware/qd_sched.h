/*
 * Quick and Dirty Scheduler for repetitive tasks
 * 
 * As the name says, the scheduler is very basic: it decides which task to run every SCHED_RATE_MS.
 * It keeps track of the last run time of each task and,
 * if current_millis - last_run_time is greater than or equal to the time interval between task runs,
 * then the selected task is run. If no task has to be run the it sleeps until the next iteration.
 * So basically each task function is executed in a periodic fashion.
 * 
 * Given how simple the scheduler is, there's an implicit support of task priority.
 * The tasks as run in a FIFO fashion with respect to calls of the sched_put_task() function. 
 * So, the first task in the list is the first to be run.
 * 
 * NOTE: this scheduler hijacks your loop() function. So avoid writing your own and just divide your
 *       workflow into tasks.
 */
#ifndef QD_SCHED_H
#define QD_SCHED_H

/* 
 * The scheduler desicides what to do every SCHED_TICK_MS milliseconds.
 * The less frequent the ticks (the higher SCHED_TICKS_MS) te more time is spent sleeping,
 * at the expense of scheduling accuracy.
 */
#define SCHED_TICK_MS 100

/* 
 * SCHED_NUM_TASKS controls the maximum number of tasks that can be allocated.
 * If you try to allocate more than SCHED_NUM_TASKS your allocation will fail and the task will never be run.
 */
#define SCHED_NUM_TASKS 4

/*
 * Enable or disable the usage of the builtin LED to signal that the scheduler is busy.
 * Warning: photosensitive epilepsy hazard
 */
#define SCHED_USE_BLINKENLIGHT

// Data structure describing a task
struct task_t {
  void (*taskFunc)(void); // the function to run
  unsigned long rateMillis; // the rate in ms at which the task is run
  unsigned long lastRunMillis; // last time in millis() the task has been run
};
typedef struct task_t task_t;

/*
 * Put a task in the scheduler.
 * 
 * Parameters:
 *  taskFunction    - pointer to a function to be run
 *  rate            - rate in ms for the task to run
 *  run_immediately - run the task immediatelly after it has been scheduled or wait for rate to expire
 *  
 * Returns the task ID or -1 if the insertion failed.
 */
int sched_put_task(void (*taskFunction)(void), unsigned long rate, bool run_immediately); 

 /*
 * Put a task in the scheduler, specifying the task ID.
 * NOTE: if a task with the same ID already exists it will be overwritten.
 * 
 * Returns the task ID or -1 if the insertion failed.
 */
int sched_put_taskID(size_t id, void (*taskFunction)(void), unsigned long rate, bool run_immediately);

 /*
  * Returns the CPU usage as a percentage
  */
uint8_t sched_get_CPU_usage();

 /*
  * Returns the task ID for the task with the same function pointer as the one in
  * taskFunction or -1 if no task with such function exists.
  */
int sched_get_taskID(void (*taskFunction)(void));

 /*
  * Reschedule a task to be run at a specified time.
  *   id   - task id to be rescheduled
  *   when - time in millis() for the next run of the task
  *   
  * If the task does not exist this function will fail silently
  */
void sched_reschedule_taskID(size_t id, unsigned long when);

#endif
