have a list of states
and a list of tasks

each task defined a state it starts on and a state that can't go past untill the task is done.
Task has a flag to say if it is completed or not.
task has child tasks that also need to be completed.

store a thread pool
each pool item has a list of tasks to run.

store a map of tasks that block each state, and advance the state only when all tasks of that dependency are complete