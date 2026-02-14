2/13/2025
Game loads from FEN string.

![My game board after loading from FEN](./docs/setup.png)

I compared the assembly code in the piece checking in `FENtoBoard()` in two different cases: (1) using a case-switch (and no variable to store the lower-case version of the piece char), and (2) using my original implementation with the variable + an if-else-if structure. I found that in case (2), there were ten less machine instructions. It's not a huge difference, but it validated my original direction, which I was unsure about due to the variable declaration. I also tried a version of the if-else-if without the variable declration, and it added five lines in addition to the case structure. So it seems that version (2) is, by a tiny margin, the preference. 