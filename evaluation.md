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
