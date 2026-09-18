# Connect 4 and Tic-Tac-Toe (C++ Application)

## **Original repository can be found [HERE](https://github.com/kanitafro/ConnectXO-app)**

NOTE: This app uses ***NatID*** GUI. In order to run it through CMake, follow download instructions from [here](https://github.com/idzafic/natID).

<img width="100" height="100" alt="Image" src="https://github.com/user-attachments/assets/0ff6ce80-88b7-4e11-b1cb-3d1a9cdbf3a9" />
<img width="160" height="100" alt="Image" src="https://github.com/user-attachments/assets/6f2dc9a6-fc87-48a1-98d4-471031806bfa" />

## Concept


<img width="250" height="196" alt="Image" src="https://github.com/user-attachments/assets/bcf59c25-0499-45e1-a692-9e8f2a3e1238" />
<img width="250" height="196" alt="Image" src="https://github.com/user-attachments/assets/5c11e4b8-4352-4091-84c1-526dabfec036" />
<img width="250" height="196" alt="Image" src="https://github.com/user-attachments/assets/e6d3f630-7c5c-4bf3-a793-c27c92f8cd90" />

## Algorithm & AI Architecture
### **The AI Engine**

We implemented the Minimax algorithm with Alpha-Beta pruning. This optimization allows the AI to ignore branches in the search tree that cannot possibly affect the final decision, improving performance and allowing for deeper lookahead.

### **Difficulty Modes**

The AI intelligence is scaled by adjusting the search depth of the Minimax algorithm across five tiers:
- Very Easy: 30% search depth
- Easy: 45% search depth
- Medium: 65% search depth
- Hard: 80% search depth
- Very Hard: 100% search depth (Maximum strategic strength)

### **AIPlayer Logic (.h and .cpp)**

The implementation follows a clean Object-Oriented structure:
- Player Enum: Defines the turns and ownership of moves.
- Game Abstract Base Class: Acts as a blueprint for both Tic-Tac-Toe and Connect 4. This allows the AI to remain "game-agnostic"—it doesn't care which game it is playing; it just evaluates the board state provided by this interface.
- AIPlayer Class: The core controller that executes the Minimax logic and returns the optimal move.

Key Design Patterns & Performance:
- Strategy Pattern: Different AI difficulty levels are handled via a depth-scaling factor rather than separate algorithms, ensuring code maintainability.
- Alpha-Beta Efficiency: Pruning reduces the number of evaluated nodes by approximately 10x, allowing the "Very Hard" mode to calculate deep lookaheads in sub-200ms timeframes.
- Asynchronous Processing: AI move calculations are decoupled from the main UI thread to prevent interface freezing during high-complexity search cycles

### **Alpha-Beta Pruning Explained**
The standard Minimax algorithm evaluates every branch. Alpha-beta pruning cuts branches 
that cannot affect the final decision. Example: if we've found a move scoring +100, 
and another branch can't exceed that, we skip it. This reduces evaluated nodes by ~90% 
in practice, allowing depth 12 searches in <200ms.
Performance Metrics & Complexity Analysis

### Time Complexity:
- Standard Minimax: O(b^d) where b = branching factor, d = search depth
- Connect4: b ≈ 7 (up to 7 columns available)
- Tic-Tac-Toe: b ≈ 5 (average available cells)
- Alpha-Beta Pruning: Reduces to O(b^(d/2)) in optimal cases; ~90% node reduction in practice

Empirical Benchmarks (Mid-Game, Release Build):  
- Depth 8: ~50ms, ~12K nodes evaluated  
- Depth 10: ~150ms, ~35K nodes evaluated  
- Depth 12: ~450ms, ~98K nodes evaluated (Connect4)  
- Depth 12: <10ms, ~5K nodes evaluated (Tic-Tac-Toe)

## User Experience (UX) & Customization

### **Localization & Score Persistence**
- Languages: Full support for English, Bosnian, and Spanish.
- Score Management: We implemented a replay button with win/loss counters. The app "remembers" these scores throughout the session.
- Persistence Control: In the Settings, we added two separate "Reset" buttons (one for each game) that allow the user to clear the stored scores and start fresh.
### **Interactive Features**
- Hint System: A dedicated button (which can be toggled on/off in Settings) that calculates the best move using the AI engine and highlights it with a pulsing gold effect.
- Dynamic Animations: * Hover Effect: Tokens appear at 75% size when the mouse is over a cell to preview a move.
- Physics: Realistic token drop animation specifically for Connect 4.
- Win Detection: A winning line is drawn across the sequence, and the footer displays the result using a color-coded overlay:
  - Green/Navy: User victory.
  - Red-ish: AI victory (User loss).

**Multimedia & Resource Engineering:**
- Physics-Based UI: The Connect 4 token drop uses a time-based acceleration model (y=21​gt2) rather than linear movement for a more natural feel.
- Event-Driven Audio: Low-latency .wav triggers are synchronized with specific game events (token impact, button clicks) using a centralized resource loader.
- State Safety: The Settings menu utilizes event suppression and restart-warning accents to prevent re-entrant bugs during real-time theme or language switching.

***Hint System Architecture:**
- Calls AIPlayer.chooseMove() at depth 8
- Returns best move calculated by same minimax engine
- Visual: highlights column/cell with pulsing yellow overlay (2s duration)
- Not a heuristic—uses full AI evaluation for accuracy


### Color Themes
We developed five distinct themes (4 light and 1 dark) to ensure the game remains visually engaging:
- Classic
- Nature (wood)
- Beachy
- Strawberry
- Dark

