"""homm3.cleanliness - source-quality scoreboard and its down-only ratchets.

The gruntz cleanliness area ported at the size we need today: one board
(`board.py`) counting banned/debt constructs over the hand-owned tree
(src/ + include/), a committed baseline (config/cleanliness-baseline.tsv)
that ratcheted metrics can only push DOWN, and a fatal gate in the
`homm3 build` tail when a protected metric rises above its floor.

First metric: C-style casts (banned outright - the tree starts at 0 and
stays there; every cast is a named C++ cast). Further metrics accrete as
rows in board.METRICS, not as new packages.
"""
