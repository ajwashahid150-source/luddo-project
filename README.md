# 🎲 Console-Based 4-Player Ludo Game in C++

A fully functional **4-player Ludo game** developed in **C++** using **procedural programming** principles. The game runs entirely in the console and implements the core rules of Ludo with an interactive interface, dice animation, colored board rendering, and complete gameplay logic without using Object-Oriented Programming (OOP). :contentReference[oaicite:1]{index=1}

---

## 📌 Features

- 🎮 4 Human Players
- 🎲 Animated Dice Rolling
- 🎨 Colored Console Board (ANSI Escape Codes)
- 👥 Player Name Customization
- 🚩 Token Deployment on Rolling 6
- 🔄 Turn Management
- 🎁 Extra Turn on Rolling 6
- ❌ Three Consecutive Sixes Rule
- ⚔️ Token Cutting
- 🛡️ Safe Zones
- 🏠 Home Path
- 🏆 Winner & Ranking System
- 📊 Live Scoreboard
- 🔔 Console Beep Sound Effects
- 📝 Input Validation
- 🖥️ Console-Based User Interface

---

## 🛠 Technologies Used

- C++
- Procedural Programming
- Standard C++ Libraries
- ANSI Escape Sequences

---

## 📚 Standard Libraries

```cpp
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <string>
```

---

## 🚫 Project Restrictions

This project was intentionally developed without using:

- Object-Oriented Programming (OOP)
- Classes
- Objects
- Structures
- STL Containers
- External Libraries
- Graphics Libraries
- Windows API

The entire game is implemented using functions, arrays, loops, and conditional statements only.

---

## 🎮 Game Rules

- Each player starts with four tokens in the base.
- A token can enter the board only after rolling a **6**.
- Rolling a **6** grants an extra turn.
- Rolling **three consecutive 6s** cancels the turn.
- Landing on an opponent's token sends it back to the base.
- Tokens on **safe cells** cannot be captured.
- Players must reach the finish using the exact dice value.
- The first player to finish all four tokens wins.

---

## 📁 Project Structure

```
Ludo-Game/
│
├── luddo project.cpp
├── README.md
└── screenshots/
```

---

## ▶️ How to Run

1. Clone this repository

```bash
git clone https://github.com/your-username/Ludo-Game.git
```

2. Open the project in **Visual Studio** or any C++ IDE.

3. Compile the source file.

4. Run the executable.

5. Enter player names and enjoy the game.

---

## 🎯 Learning Outcomes

This project demonstrates practical implementation of:

- Procedural Programming
- Arrays
- Functions
- Loops
- Conditional Statements
- Random Number Generation
- Console Animation
- Game Logic Design
- Input Validation
- Algorithm Design

---

## 👩‍💻 Author

**Ajwa Shahid**

BS Computer Science  
FAST National University, Pakistan

---

## ⭐ Future Improvements

- Computer (AI) Player
- Save & Load Game
- Multiplayer over Network
- Graphical User Interface (GUI)
- Sound Effects using Multimedia Libraries
- Online Multiplayer Support

---

## 📄 License

This project is developed for educational and learning purposes.
