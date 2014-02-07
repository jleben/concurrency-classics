## Dining Philosophers

Solution 1 (Jyoti)

Solution 2 (Jakob):

Philosophers are either left handed or right handed, which means
they will always pick their left fork first or their right fork first.
Moreover, they are seated in alternating order depeding on main hand:
left-handed, right-handed, left-handed, ... This prevents deadlock where
all philosophers would pick their left fork first and wait for their right
fork.

Solution 3 (Jakob):

Forks decide which philosopher can pick them up. Each fork
alternates between offering itself to the philosopher on the left or on the
right. In addition, forks are placed in such an order that they start their
behavior in alternating states: start-left, start-right, start-left, ...
This way, there is a constantly changing set of pairs of adjacent forks that
are offered to the same philosophers, so they can eat.

### Solution 1 Evaluation

...

### Solution 2 Evaluation

Forks are represented simply by mutexes, and when a philosopher wants to eat,
he tries to acquire two mutexes representing his left and right forks.
This solution is faster than the CSP solution (3), because it uses minimum
amount of synchronization, but it doesn't treat all philosophers equally, when
there is an odd number of philosophers. Due to assymetry, some philosophers
are at disadvantage.

Distribution and total timing of 10.000 cycles of thinking and eating
performed collectively by all philosophers:
```
0: 2464
1: 2266
2: 2315
3: 1282
4: 1673
Total time = 928.186 ms
```

With 6 philosophers instead of 5, the distribution is more even:
```
0: 1612
1: 1742
2: 1634
3: 1716
4: 1672
5: 1624
Total time = 925.85 ms
```

### Solution 3 Evaluation

In this CSP solution, forks are processes (threads) on their own.
They repeat the cycle of awaiting 4 different signals in this order:
- left-philosopher-picked
- left-philosopher-dropped
- right-philosopher-picked
- right-philosopher-dropped

Philosophers try to send signals to their left and right forks in this order:
- left fork: right-philosopher-picked
- right fork: left-philosopher-picked
- left fork: right-philosopher-dropped
- right fork: left-philosopher-dropped

Because in CSP writes are blocking, whenever a philosopher tries to send
a signal, he will block until the fork is awaiting that particular signal.

Because forks are being offered exactly to the philosopher on their left or
right in alternation, there is never any racing for forks, and the
distribution among philosophers is completely fair.

However, as usual in CSP solutions, the extra threads for forks and
extra synchronization slows down the implementation:
```
0: 2000
1: 2000
2: 2000
3: 2000
4: 2000
Total time = 1.74805 s
```

## Cigarette Smokers

Solution 1 (Jyoti)

Solution 2 (Jakob):

The table is considered a monitor – since a lock is always owned by anyone
operating on smoking ingredients on the table (and for the whole of that time),
it is of little importance that smokers need two individual ingredients: while
having exclusive access to the table, they can always check whether both of the
2 ingredients match their needs, and then either pick both or none.

Solution 3 (Jakob):

A CSP solution: there are 4 processes: the dealer, and 3 smokers. The table is
abstracted away: the dealer has access to each smokers' channel, and writes
ingredients to that particular channel, after having randomly selected a smoker.
Again, it is of no importance that there exist different ingredients:
The individual smokers' channels could have different data types (tuples of 2
different ingredients), but that would only artificially complicate the
implementation without actually modifying the concurrent structure. Smokers
simply loop waiting for anything to be written on their single input channel.



### Solution 2 Evaluation:

The problem (and implementation) is so simple, that as long as the distribution
of the dealer's random process of ingredient selection is uniform, the smoker
threads will be treated farily.

Distribution and total time of 10000 cigarettes smoked collectively by all
smokers, averaged across 5 runs:
```
1: 3330.8
2: 3317.6
3: 3350.6
Total Time = 1827.412 ms
```

### Solution 3 Evaluation:

Again, fairness is simply ensured by the uniformity of the random ingredient
selection process. In this particular case, the concurrent structure is also
very simple, so the overhead of CSP (extra synchronization for blocking
writes) is almost negligible compared to the monitor-type solution 2.

Distribution and total time of 10000 cigarettes smoked collectively by all
smokers, averaged across 5 runs:
```
1: 3333
2: 3345.2
3: 3320.8
Total Time = 1906.374 ms
```

## Barber Shop

Solution 1 (Jyoti)

Solution 2 (Jakob):

I took it as a requirement that each customer is a thread that must wait until
having been treated by the barber, and only then continue.
The barber shop is a monitor with a queue of customers: each new customer first
"enters" the monitor (locks the global lock), checks whether the queue is full,
and leaves immediately if it is, else puts itself into the queue, and signals
the global shop condition that a new customer has arrived, in case the barber
is sleeping. Each customer also has own condition variable which it waits on
after enqueueing themselves. The barber keeps waiting on the shop's condition
to be signalled, and when it is dequeues next customer, serves them, and then
signals the particular customer's own condition to notify them that the job is
done and they can leave. After that the barber checks whether the queue is
empty and if so continues to wait on the shop's condition, else serves next
customer.


Solution 3 (Jakob):

This is a CSP solution. It is complicated by the fact that no data should be
shared among threads except over channels, but the customers must somehow
examine whether the shop's queue is full and in that case leave. Moreover,
I wanted to use a channel as a way of enqueueing customers instead of actually
placing them into a queue data structure. So the room here is another process
and serves only as a mechanism to accept or reject customers based on already
waiting customers.

Each customer first writes themselves to a room's channel, and awaits either
confirmation or rejection from the room on their own channel. The room awaits
customers on its channel; when receiving a customer checks internal counter:
if it's at the limit, sends the customer a rejection signal, else increments
the counter and sends a confirmation signal. The room then awaits next customer.

When a customer receives the signal of acceptance they write themselves to the
barber's input channel - that's a method of actually enqueuing customers -
which will keep them waiting until the barber receives them, and then await
end-of-service signal from the barber.

The barber simply keeps receiving customers on own input channel, and when
they are serviced, signals this fact back to the customer's other channel for
this purpose.

### Evaluation

For evaluation, I decided measured the time it takes for the barber to process
10000 customers. There are two issues with this kind of measurement on this
kind of problem.
1. I wanted the system to run as fast as possible, but if each
customer represents a thread, we would soon limit the system-limit on number
of threads. Instead, I decided to represent all customers only with 20 threads
which repetedly loop the single-customer procedure.
2. How do we handle rejections when barber shop is full? Do we just let the
thread spin until a place in the shop is available? So for the purpose of
this measurement, I decided to remove the limit on queue size altogether, so
each customer that arrives would get queued. Because we also have a limited
amount of total threads (20), the maximum queue length is bounded.

Again, the non-CSP version proved faster than the CSP one.

Solution 2: Total time = 1509.6 ms

Solution 3: Total time = 2234.03 ms


## Santa Claus

Solution 1 (Jyoti)

Solution 2 (Jakob)

Classical Monitor approach: There is only one global lock, and four condition
variables. One condition is for elves to wait until there are at least 3 who
need help. Another is for elves that have visited santa to wait until santa
has helped the whole group. Another condition variable is for elks to wait
until all have returned from the tropics. Finally, one condition is for santa
to wait and get signalled once a group of 3 elves is ready to see him, or all
elks have returned from the tropics. All conditions are always signalled in
broadcast mode, and each awaken thread checks shared monitor state to
determine whether they can continue, and modifies the state accordingly.

Solution 3 (Jakob)

A CSP solution. In order not to wake up santa for each individual elf or
each individual elk, there are intermediate processes called “elf barrier”
and “elk barrier” - they have an input channel of type elf or elk, they
assemble elves or elks into groups, and write to santas input channels of
type array-of-elves and array-of-elks.




## Building H20

Solution 1 (Jyoti)

Solution 2 (Jakob)

If an element can fill an empty spot in the currently built molecule, it will
do so right away without waiting, else it will wait until a spot for its type
becomes empty.

Whenever a molecule is complete, the last element to complete it will wake up
all the waiting threads, which will again race for the empty spots in the next
molecule.

This is again a classical monitor approach with one global mutex and a single
condition variable for elements to wait if there's no spot for their type in
the molecule.

Solution 3 (Jakob)

A CSP solution. We've got one process for each element, and another process
to have the role of the molecule builder. The latter has two input channels:
one for the H and one for the O elements. The elements simply try to write
to this channel, blocking until their write is accepted. The builder proces
simply repeatedly awaits inputs in the following pattern: two times on the
H channel and one time on the O channel...



## River Crossing

Solution 1 (Jyoti)

Solution 2 (Jakob)

This is a very similar approach to the H2O problem, with additional
requirement that threads that form a group need to wait for all their
group-mate threads before continueing. In addition, the grouping rules are
more complicated, so threads can not know whether they will get a spot in the
boat until at least 4 threads have arrived at the borading barrier. So no
thread can ever advance past the barrier without checking that there is enough
and the right combination of threads to form a boatload. Only the one to arrive
and find a plausible combination of other threads waiting will continue; it
will first set shared monitor state so that other threads know how many of
each type can advance, and wake up all other threads. The last thread to board
the boat will be the captain, and will wake up all the waiting threads again,
in case another boat can be filled right after, without another thread arriving.

Solution 3 (Jakob)

This is another CSP solution. Here, just like in the H2O CSP solution, the
barrier (in this case representing the boat) is another process of its own,
waiting for passenger threads to write to its two input channels. This time
however, the situation is complicated because the boat process must know
how many of each type of passengers are ready to board, but the only way for
it to get the information is via an input channel: hence, it has to receive
information from passenger threads *before* actually allowing them to board.
Therefore, a completed write of a passenger thread can not be interpreted as
boarding.

In this solution, the boat thread manages two queue data structures: one for
each type of passengers. It has a single input channel where passenger
processes write *themselves* (both passenger types being represented by the
same C++ type). When the boat process receives a passenger process, it
enqueues it according to its passenger type, and checks how many of each type
are enqueued. If a valid boatload can be formed, the boat dequeues desired
passengers and writes *back* to their own channels on which they have been
awaiting confirmation to board after having been enqueued.

Another interesting issue is the choice of the captain. There is no
shared information among passenger threads (like the number of threads that
have boarded) for them to know which one was the last to board and be selected
as a captain. Since communication channels are always one-to-one, it would
also be complicated for them to have enough conversation to figure that out.
Instead, when the boat signals passengers that they are ready to board, it
writes meaningful information to their channel, which gives each one a role.
Only one will be given the role of a captain. Moreover, the captain must only
execute "row" after all co-passengers have boarded. For that purpose, once
threads have received a role in the boat, they execute "board" and then signal
back to the boat that they have done so. The boat then waits for those signals
in such an order that the captain thread's signal is awaited last.

There's actually 2 more slightly different implementations in the same file
that try to shuffle around who writes to whom and who reads from whom in
attempts to make the whole concurrent structure more efficient. One of those
is correct, but proved no faster under time-measurement. Another one is a
little faster but may deadlock.
Have a look at the code...
