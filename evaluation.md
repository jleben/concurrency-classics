## Dining Philosophers

In solution 2, philosophers are either left handed or right handed, which means
they will always pick their left fork first or their right fork first.
Moreover, they are seated in alternating order depeding on main hand:
left-handed, right-handed, left-handed, ... This prevents deadlock where
all philosophers would pick their left fork first and wait for their right
fork.
In solution 3, forks decide which philosopher can pick them up. Each fork
alternates between offering itself to the philosopher on the left or on the
right. In addition, forks are placed in such an order that they start their
behavior in alternating states: start-left, start-right, start-left, ...
This way, there is a constantly changing set of pairs of adjacent forks that
are offered to the same philosophers, so they can eat.

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
