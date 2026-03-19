2/13/2025
Game loads from FEN string.

![My game board after loading from FEN](./docs/setup.png)

I compared the assembly code in the piece checking in `FENtoBoard()` in two different cases: (1) using a case-switch (and no variable to store the lower-case version of the piece char), and (2) using my original implementation with the variable + an if-else-if structure. I found that in case (2), there were ten less machine instructions. It's not a huge difference, but it validated my original direction, which I was unsure about due to the variable declaration. I also tried a version of the if-else-if without the variable declration, and it added five lines in addition to the case structure. So it seems that version (2) is, by a tiny margin, the preference. 

**Moves**
Full disclosure: last week, I had playtests in 171, so I didn't have a lot of bandwidth to work on the first move sets (pawns, knights, kings). I followed along with lecture recordings. I will hopefully have time soon to watch Sebastian Lague's chess videos and maybe improve my code. I made some notes with comments in areas where I plan to replace stuff with templates so my code is faster.

![Initial moves for pawns, knights, and kings totaling to 20](./docs/pawnsknightkings_initmoves.png)
![Bishops, rooks, queens too](./docs/bishopsrooksqueens.png)

**AI**
The AI uses negamax search implemented in `Chess::negamax()`. At each node, the algorithm generates all pseudo-legal moves, filters to legal moves (that don't leave the king in check), and recursively evaluates the resulting positions. AB pruning is applied to cut off branches that cant improve on the current best score.

- runs at a default depth of 4 
- eval function is implemented in `Evaluate.h` using piece square tables provided in class. It scores the board based on material balance using piece values
- plays better than random: consistently beats me, a chess novice who basically moves pieces at random

**future fixes**
- improve filtering the pseudo legal moves to legal moves bottleneck
- add en passant, castling, and pawn promotion