Authors:
@Tuna Çimen

AIR TRAFFIC CONTROL SIMULATOR

We implemented an air traffic control simulator with one runway and 2 seconds runway control time.
You can see the example log file Example_Log.txt for n=60 and probability 0.5.
To run you can runusing ./main <seconds> <probability>
or you can recompile from scratch.

Part 1

Planes are implemented as threads and have their control function.
Inside this function every plane with a certain probability request landing if flying and departing if they are landed.
Since there is a mutex lock called comm_lock only one plane at a time can contact the tower for landing or departing.
There is another lock for runway that allows only one plane to use the runway at a time.
Another thread is responsible for counting the seconds passed.
For fuel efficiency the initial implementation always favored landing ones and only allowed took offs after 3 planes landed.
However this caused starvation since sometimes planes could not take off since they were waiting some plane to land but no
    plane was at the queue for landing. In part 2 we solved it.

Part 2

To avoid the take-off starvation, added max waiting time of 15 seconds so after some plane waits to take off more than
 15 seconds the starve mutex lock is unlocked so planes that wait for taking off can take off. After any takeoff
 starve_lock is locked so only after either 3 planes landed or 15 seconds passed since waiting, it can be unlocked.

Part 3

At every request + at every landing/departure we log the entry, and at the end of the n seconds it is printed in a simple,
format. As demonstrated in the example log file.



