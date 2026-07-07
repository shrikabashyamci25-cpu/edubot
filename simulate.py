SEP = "------------------------------------------------------------"

def bot(text):
    print(f"\n  EduBot:\n{text}\n")
    print(SEP)

def user(text):
    print(f"  You> {text}")

def answer(text):
    print(f"  Answer> {text}")

# ── STARTUP
print()
print("  [EduBot] Loaded 65 knowledge entries from 'edubot_data.txt'")
print()

# ── GREETING
print("============================================================")
print("       EduBot -- Text-Based Educational Assistant")
print("============================================================")
print("  Subjects available:")
print("    [1] Physics            [2] Civil Engineering")
print("    [3] Mathematics        [4] Artificial Intelligence")
print("    [5] C Programming      [6] Chemistry")
print("    [7] Electronics")
print(SEP)
print("  Please enter your name to begin:")
print(SEP)

# ── Name
user("Alice")
bot(
    "\n  Welcome, Alice! Let's get started.\n"
    "\n============================================================\n"
    "  SELECT A SUBJECT\n"
    "============================================================\n"
    "  [1] Physics            [2] Civil Engineering\n"
    "  [3] Mathematics        [4] Artificial Intelligence\n"
    "  [5] C Programming      [6] Chemistry\n"
    "  [7] Electronics\n"
    "------------------------------------------------------------\n"
    "  Type a number or subject name. Type 'exit' to quit."
)

# ════════════════════════════════════════════════════════════════
# CIVIL ENGINEERING — staircase dataset demo
# ════════════════════════════════════════════════════════════════
user("2")
bot(
    "\n  Civil Engineering selected.\n"
    "\n============================================================\n"
    "  Civil Engineering -- Q&A MODE\n"
    "============================================================\n"
    "  Ask any question about Civil Engineering.\n"
    "  Commands: quiz | menu | help | exit\n"
    "------------------------------------------------------------"
)

user("how many types of staircases are there")
bot(
    "\nTypes of Staircases (8 main types):\n"
    "  1. Straight flight staircase: Single straight flight between floors.\n"
    "     Used in narrow spaces; simple and economical.\n"
    "  2. Dog-legged staircase: Two straight flights in opposite direction\n"
    "     with a common landing at mid-height. Most common type in buildings.\n"
    "  3. Open-well staircase: Two flights with an open well (void) between.\n"
    "     Allows light and ventilation; used in public buildings.\n"
    "  4. Spiral / Helical staircase: Winds around a central pole.\n"
    "     Space-saving; used as service/emergency stairs.\n"
    "  5. Bifurcated staircase: Starts as one wide flight, splits into two\n"
    "     narrower flights at a landing. Grand entrance staircases.\n"
    "  6. Quarter-turn staircase (L-shaped): Changes direction by 90 degrees\n"
    "     at a landing. Suits corner spaces.\n"
    "  7. Three-quarter turn staircase: Changes direction by 270 degrees.\n"
    "  8. Circular staircase: Continuous curve around a central axis.\n"
    "     Decorative; used in high-end architectural projects.\n"
    "\n  [Ask another question | type 'quiz' | 'menu' | 'help']"
)

user("what is a dog legged staircase")
bot(
    "\nDog-Legged Staircase:\n"
    "  Definition: A staircase with two flights turning 180 degrees\n"
    "  at a common intermediate landing.\n"
    "  Key features:\n"
    "    - No open well (void) between the two flights.\n"
    "    - The plan view looks like a dog's hind leg — hence the name.\n"
    "    - Very common in residential buildings due to space efficiency.\n"
    "    - Landing width = width of one flight.\n"
    "    - Suitable where staircase width >= 2 x tread width + wall thickness.\n"
    "  Advantages: Simple, economic, and fits compact floor plans.\n"
    "\n  [Ask another question | type 'quiz' | 'menu' | 'help']"
)

user("menu")
bot(
    "\n  Returning to subject selection.\n"
    "\n============================================================\n"
    "  SELECT A SUBJECT\n"
    "============================================================\n"
    "  [1] Physics            [2] Civil Engineering\n"
    "  [3] Mathematics        [4] Artificial Intelligence\n"
    "  [5] C Programming      [6] Chemistry\n"
    "  [7] Electronics\n"
    "------------------------------------------------------------\n"
    "  Type a number or subject name. Type 'exit' to quit."
)

# ════════════════════════════════════════════════════════════════
# MATHS
# ════════════════════════════════════════════════════════════════
user("3")
bot(
    "\n  Mathematics selected.\n"
    "\n============================================================\n"
    "  Mathematics -- Q&A MODE\n"
    "============================================================\n"
    "  Ask any question about Mathematics.\n"
    "  Commands: quiz | menu | help | exit\n"
    "------------------------------------------------------------"
)

user("explain differentiation and derivative")
bot(
    "\nDifferentiation finds the rate of change (slope) of a function.\n"
    "  Basic rules:\n"
    "  Power rule:     d/dx(x^n) = n*x^(n-1)\n"
    "  Product rule:   d/dx(u*v) = u'v + uv'\n"
    "  Chain rule:     d/dx(f(g(x))) = f'(g(x)) * g'(x)\n"
    "  Quotient rule:  d/dx(u/v) = (u'v - uv') / v^2\n"
    "  Common: d/dx(sin x)=cos x, d/dx(e^x)=e^x, d/dx(ln x)=1/x\n"
    "\n  [Ask another question | type 'quiz' | 'menu' | 'help']"
)

# ── HELP BUG FIX DEMO: question containing the word 'help'
user("help me understand integration")
bot(
    "\nIntegration is the reverse of differentiation (anti-derivative).\n"
    "  Definite integral: area under the curve between two limits.\n"
    "  Basic rules:\n"
    "  Power rule: INT(x^n dx) = x^(n+1)/(n+1) + C\n"
    "  INT(sin x dx) = -cos x + C\n"
    "  INT(e^x dx) = e^x + C\n"
    "  INT(1/x dx) = ln|x| + C\n"
    "  Fundamental Theorem: INT[a to b] f(x)dx = F(b) - F(a)\n"
    "\n  [Ask another question | type 'quiz' | 'menu' | 'help']"
)

# ── HELP standalone (shows banner, not KB answer)
user("help")
bot(
    "\n============================================================\n"
    "  Mathematics -- Q&A MODE\n"
    "============================================================\n"
    "  Ask any question about Mathematics.\n"
    "  Commands:\n"
    "    quiz   - Start a multiple-choice quiz\n"
    "    menu   - Return to subject selection\n"
    "    help   - Show this help (type 'help' alone)\n"
    "    exit   - Quit EduBot\n"
    "------------------------------------------------------------"
)

# ════════════════════════════════════════════════════════════════
# AI
# ════════════════════════════════════════════════════════════════
user("menu")
bot(
    "\n  Returning to subject selection.\n"
    "\n============================================================\n"
    "  SELECT A SUBJECT\n"
    "============================================================\n"
    "  [1] Physics            [2] Civil Engineering\n"
    "  [3] Mathematics        [4] Artificial Intelligence\n"
    "  [5] C Programming      [6] Chemistry\n"
    "  [7] Electronics\n"
    "------------------------------------------------------------\n"
    "  Type a number or subject name. Type 'exit' to quit."
)

user("4")
bot(
    "\n  Artificial Intelligence selected.\n"
    "\n============================================================\n"
    "  Artificial Intelligence -- Q&A MODE\n"
    "============================================================\n"
    "  Ask any question about Artificial Intelligence.\n"
    "  Commands: quiz | menu | help | exit\n"
    "------------------------------------------------------------"
)

user("what is overfitting and underfitting")
bot(
    "\nOverfitting vs Underfitting:\n"
    "  OVERFITTING:\n"
    "    Model learns noise/details in training data -> poor test accuracy.\n"
    "    High training accuracy, low test accuracy (high variance).\n"
    "    Solutions: Dropout, L1/L2 Regularization, more training data, pruning.\n"
    "  UNDERFITTING:\n"
    "    Model too simple -> poor training AND test accuracy (high bias).\n"
    "    Solutions: More complex model, more features, reduce regularization.\n"
    "  Bias-Variance Tradeoff: finding the balance between the two.\n"
    "\n  [Ask another question | type 'quiz' | 'menu' | 'help']"
)

user("quiz")
bot(
    "\n============================================================\n"
    "  QUIZ -- Question 1 of 5\n"
    "============================================================\n"
    "  Which ML type uses labelled data for training?\n\n"
    "  A) Unsupervised\n"
    "  B) Reinforcement\n"
    "  C) Supervised\n"
    "  D) Generative\n"
    "------------------------------------------------------------\n"
    "  Enter your answer (A / B / C / D):"
)

answer("C")
bot(
    "\n  [CORRECT!] Well done!\n"
    "  Explanation: Supervised learning uses input-output pairs (labelled data) to train.\n"
    "\n============================================================\n"
    "  QUIZ -- Question 2 of 5\n"
    "============================================================\n"
    "  The activation function ReLU is defined as:\n\n"
    "  A) 1/(1+e^-x)\n"
    "  B) max(0,x)\n"
    "  C) (e^x - e^-x)/(e^x + e^-x)\n"
    "  D) x^2\n"
    "------------------------------------------------------------\n"
    "  Enter your answer (A / B / C / D):"
)

answer("B")
bot(
    "\n  [CORRECT!] Well done!\n"
    "  Explanation: ReLU=Rectified Linear Unit=max(0,x). Outputs 0 for negative inputs.\n"
    "\n============================================================\n"
    "  QUIZ -- Question 3 of 5\n"
    "============================================================\n"
    "  A* search uses the function f(n) equal to:\n\n"
    "  A) h(n) only\n"
    "  B) g(n) only\n"
    "  C) g(n) + h(n)\n"
    "  D) g(n) * h(n)\n"
    "------------------------------------------------------------\n"
    "  Enter your answer (A / B / C / D):"
)

answer("A")  # wrong
bot(
    "\n  [INCORRECT] Correct answer: C\n"
    "  Explanation: A* uses f(n)=g(n)+h(n) where g=cost so far, h=heuristic to goal.\n"
    "\n============================================================\n"
    "  QUIZ -- Question 4 of 5\n"
    "============================================================\n"
    "  Overfitting is characterised by:\n\n"
    "  A) High training error, high test error\n"
    "  B) Low training error, high test error\n"
    "  C) Low training error, low test error\n"
    "  D) High training error, low test error\n"
    "------------------------------------------------------------\n"
    "  Enter your answer (A / B / C / D):"
)

answer("B")
bot(
    "\n  [CORRECT!] Well done!\n"
    "  Explanation: Overfitting: model memorises training data => low train but high test error.\n"
    "\n============================================================\n"
    "  QUIZ -- Question 5 of 5\n"
    "============================================================\n"
    "  In K-Means, the Elbow method is used to:\n\n"
    "  A) Determine learning rate\n"
    "  B) Find optimal number of clusters k\n"
    "  C) Remove outliers\n"
    "  D) Normalise data\n"
    "------------------------------------------------------------\n"
    "  Enter your answer (A / B / C / D):"
)

answer("B")
bot(
    "\n  [CORRECT!] Well done!\n"
    "  Explanation: Elbow method plots inertia vs k; the elbow point = optimal k.\n"
    "\n============================================================\n"
    "  QUIZ COMPLETE!\n"
    "============================================================\n"
    "  Score: 4 / 5  (80%)   Grade: A\n"
    "------------------------------------------------------------\n"
    "  Type 'quiz' to retry, 'menu' to change subject,\n"
    "  or ask any question to continue Q&A mode.\n"
    "============================================================"
)

# ════════════════════════════════════════════════════════════════
# C PROGRAMMING
# ════════════════════════════════════════════════════════════════
user("menu")
bot(
    "\n  Returning to subject selection.\n"
    "\n============================================================\n"
    "  SELECT A SUBJECT\n"
    "============================================================\n"
    "  [1] Physics            [2] Civil Engineering\n"
    "  [3] Mathematics        [4] Artificial Intelligence\n"
    "  [5] C Programming      [6] Chemistry\n"
    "  [7] Electronics\n"
    "------------------------------------------------------------\n"
    "  Type a number or subject name. Type 'exit' to quit."
)

user("5")
bot(
    "\n  C Programming selected.\n"
    "\n============================================================\n"
    "  C Programming -- Q&A MODE\n"
    "============================================================\n"
    "  Ask any question about C Programming.\n"
    "  Commands: quiz | menu | help | exit\n"
    "------------------------------------------------------------"
)

user("what are pointers in C")
bot(
    "\nPointers in C:\n"
    "  A pointer stores the MEMORY ADDRESS of another variable.\n"
    "  Declaration: int *p;  (pointer to int)\n"
    "  Assignment:  p = &x;  (&x = address of x)\n"
    "  Dereferencing: *p = value at address stored in p.\n"
    "  Pointer arithmetic: p++ moves to next int address (by sizeof(int) bytes).\n"
    "  NULL pointer: int *p = NULL; (safe uninitialised value)\n"
    "  Pointers are fundamental to arrays, dynamic memory, and function arguments.\n"
    "\n  [Ask another question | type 'quiz' | 'menu' | 'help']"
)

# ════════════════════════════════════════════════════════════════
# CHEMISTRY
# ════════════════════════════════════════════════════════════════
user("menu")
bot("\n  Returning to subject selection.\n  [menu shown above]")

user("6")
bot(
    "\n  Chemistry selected.\n"
    "\n============================================================\n"
    "  Chemistry -- Q&A MODE\n"
    "============================================================\n"
    "  Ask any question about Chemistry.\n"
    "  Commands: quiz | menu | help | exit\n"
    "------------------------------------------------------------"
)

user("what is the mole concept")
bot(
    "\nThe Mole Concept (Avogadro's Number):\n"
    "  1 mole = 6.022 x 10^23 particles (atoms/molecules/ions).\n"
    "  Molar mass: mass of 1 mole in grams = atomic mass in g/mol.\n"
    "    Example: Molar mass of H2O = 2(1) + 16 = 18 g/mol.\n"
    "  Moles = mass (g) / molar mass (g/mol)\n"
    "  Molar volume of gas at STP = 22.4 L/mol.\n"
    "\n  [Ask another question | type 'quiz' | 'menu' | 'help']"
)

# ════════════════════════════════════════════════════════════════
# ELECTRONICS
# ════════════════════════════════════════════════════════════════
user("menu")
bot("\n  Returning to subject selection.\n  [menu shown above]")

user("7")
bot(
    "\n  Electronics selected.\n"
    "\n============================================================\n"
    "  Electronics -- Q&A MODE\n"
    "============================================================\n"
    "  Ask any question about Electronics.\n"
    "  Commands: quiz | menu | help | exit\n"
    "------------------------------------------------------------"
)

user("explain logic gates AND OR NAND NOR")
bot(
    "\nLogic Gates (Digital Electronics):\n"
    "  AND:  output=1 only if ALL inputs=1.  Y = A.B\n"
    "  OR:   output=1 if ANY input=1.        Y = A+B\n"
    "  NOT:  output is complement of input.   Y = A' (inverter)\n"
    "  NAND: NOT of AND; universal gate.      Y = (A.B)'\n"
    "  NOR:  NOT of OR; universal gate.       Y = (A+B)'\n"
    "  XOR:  output=1 if inputs are DIFFERENT.\n"
    "  NAND and NOR are UNIVERSAL gates (can implement any Boolean function).\n"
    "  De Morgan's: (A.B)' = A'+B';  (A+B)' = A'.B'\n"
    "\n  [Ask another question | type 'quiz' | 'menu' | 'help']"
)

user("what is a transistor")
bot(
    "\nTransistor (BJT):\n"
    "  Three terminals: Base (B), Collector (C), Emitter (E).\n"
    "  Types: NPN and PNP.\n"
    "  NPN: small base current controls large collector current.\n"
    "  IC = beta * IB;  IE = IC + IB.  Beta (hFE) = current gain (~100-500).\n"
    "  Regions: Active (amplifier), Saturation (switch ON), Cut-off (switch OFF).\n"
    "  Applications: amplifiers, switches, oscillators.\n"
    "\n  [Ask another question | type 'quiz' | 'menu' | 'help']"
)

# ── EXIT
user("exit")
bot(
    "\n============================================================\n"
    "  Thank you for studying with EduBot, Alice!\n"
    "  Session saved to 'session_log.txt'. Keep learning!\n"
    "============================================================"
)

print("\n  Session saved to 'session_log.txt'. Goodbye!\n")
