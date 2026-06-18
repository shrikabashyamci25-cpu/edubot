#include "assistant.h"

/* ================================================================== */
/*  RUNTIME KNOWLEDGE BASE  — loaded from edubot_data.txt             */
/* ================================================================== */
static KnowledgeItem KB_STORE[MAX_KB_ENTRIES];
static int           KB_COUNT = 0;

/* Map topic string in dataset file to SubjectTopic enum */
static SubjectTopic parse_topic_str(const char *s) {
    if (strstr(s,"physics"))     return TOPIC_PHYSICS;
    if (strstr(s,"civil"))       return TOPIC_CIVIL_ENGINEERING;
    if (strstr(s,"maths")||strstr(s,"math")) return TOPIC_MATHS;
    if (strstr(s,"ai")||strstr(s,"artificial")) return TOPIC_AI;
    if (strstr(s,"cprog")||strstr(s,"c_prog")) return TOPIC_C_PROGRAMMING;
    if (strstr(s,"chemistry"))   return TOPIC_CHEMISTRY;
    if (strstr(s,"electronics")) return TOPIC_ELECTRONICS;
    return TOPIC_NONE;
}

/*
 * load_knowledge_base
 *
 * Parses edubot_data.txt into KB_STORE[].
 *
 * File format per entry:
 *   #TOPIC=<topic_name>
 *   #KEYS=<space separated keywords>
 *   <answer line 1>
 *   <answer line 2>
 *   ...
 *   ##END##
 *
 * Lines starting with # that are NOT #TOPIC or #KEYS are ignored.
 * Blank lines between ##END## and next #TOPIC are skipped.
 */
void load_knowledge_base(const char *filename)
{
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        fprintf(stderr,
            "[WARNING] Dataset file '%s' not found.\n"
            "  Place edubot_data.txt in the same folder as edubot.exe\n",
            filename);
        return;
    }

    char         line[MAX_RESPONSE_LEN];
    KnowledgeItem cur;
    int           in_entry = 0;
    memset(&cur, 0, sizeof(cur));

    while (fgets(line, sizeof(line), fp)) {
        /* Strip trailing newline / carriage-return */
        int len = (int)strlen(line);
        while (len > 0 && (line[len-1]=='\n' || line[len-1]=='\r'))
            line[--len] = '\0';

        if (strncmp(line, "#TOPIC=", 7) == 0) {
            /* Start a new entry */
            memset(&cur, 0, sizeof(cur));
            cur.topic = parse_topic_str(line + 7);
            in_entry  = 1;

        } else if (strncmp(line, "#UNIT=", 6) == 0) {
            cur.unit = atoi(line + 6);

        } else if (strncmp(line, "#KEYS=", 6) == 0) {
            strncpy(cur.keywords, line + 6, MAX_INPUT_LEN - 1);
            cur.keywords[MAX_INPUT_LEN - 1] = '\0';

        } else if (strcmp(line, "##END##") == 0) {
            /* Save completed entry */
            if (in_entry && cur.topic != TOPIC_NONE &&
                KB_COUNT < MAX_KB_ENTRIES) {
                KB_STORE[KB_COUNT++] = cur;
            }
            in_entry = 0;
            memset(&cur, 0, sizeof(cur));

        } else if (in_entry && line[0] != '\0') {
            /* Append line to answer */
            int cur_len = (int)strlen(cur.answer);
            if (cur_len > 0 && cur_len < MAX_RESPONSE_LEN - 2) {
                cur.answer[cur_len]   = '\n';
                cur.answer[cur_len+1] = '\0';
                cur_len++;
            }
            strncat(cur.answer, line, MAX_RESPONSE_LEN - cur_len - 1);
        }
    }

    fclose(fp);
    printf("  [EduBot] Loaded %d knowledge entries from '%s'\n\n",
           KB_COUNT, filename);
}

/*
 * find_best_match
 *
 * Scores every KB entry for the given topic against the processed
 * input using compute_similarity() on both keyword and answer fields,
 * and returns the highest-scoring answer above SIMILARITY_THRESHOLD.
 */
int find_best_match(const char *processed_input,
                    SubjectTopic topic,
                    int unit,
                    char *response_out)
{
    if (!processed_input || !response_out) return 0;

    float best      = 0.0f;
    int   best_idx  = -1;
    int   i;

    for (i = 0; i < KB_COUNT; i++) {
        if (KB_STORE[i].topic != topic) continue;
        /* unit filter: skip if entry has a unit set and it doesn't match */
        if (unit > 0 && KB_STORE[i].unit > 0 && KB_STORE[i].unit != unit) continue;

        float s1 = compute_similarity(processed_input, KB_STORE[i].keywords);
        /* Also score against first 256 chars of answer for richer matching */
        char ans_head[256];
        strncpy(ans_head, KB_STORE[i].answer, 255);
        ans_head[255] = '\0';
        float s2 = compute_similarity(processed_input, ans_head);
        float s  = s1 > s2 ? s1 : s2;
        /* boost exact unit match so unit-specific entries beat unit=0 generics */
        if (unit > 0 && KB_STORE[i].unit == unit) s *= 1.5f;
        if (s > 1.0f) s = 1.0f;

        if (s > best) { best = s; best_idx = i; }
    }

    if (best_idx >= 0 && best >= SIMILARITY_THRESHOLD) {
        strncpy(response_out, KB_STORE[best_idx].answer, MAX_RESPONSE_LEN - 1);
        response_out[MAX_RESPONSE_LEN - 1] = '\0';
        return 1;
    }

    strncpy(response_out,
        "I could not find a relevant answer for that query.\n"
        "  Tips:\n"
        "    - Use key words only (e.g. 'array definition' not 'what is an array')\n"
        "    - Try a different unit with 'units' command\n"
        "    - Type 'help' to see all commands",
        MAX_RESPONSE_LEN - 1);
    return 0;
}

/*
 * list_topics
 *
 * Builds a formatted list of every KB entry title that matches the
 * given topic+unit filter.  The "title" is the first line of the
 * answer text (everything before the first '\n').
 * Used by the help command to show students what they can ask about.
 */
void list_topics(SubjectTopic topic, int unit, char *response_out)
{
    if (!response_out) return;

    int pos = 0, count = 0, i;
    pos += snprintf(response_out + pos, MAX_RESPONSE_LEN - pos,
        "\n  Topics you can ask about");

    if (unit > 0)
        pos += snprintf(response_out + pos, MAX_RESPONSE_LEN - pos,
            " (Unit %d):\n", unit);
    else
        pos += snprintf(response_out + pos, MAX_RESPONSE_LEN - pos,
            ":\n");

    pos += snprintf(response_out + pos, MAX_RESPONSE_LEN - pos,
        "  ------------------------------------------------------------\n");

    for (i = 0; i < KB_COUNT && pos < MAX_RESPONSE_LEN - 80; i++) {
        if (KB_STORE[i].topic != topic) continue;
        if (unit > 0 && KB_STORE[i].unit > 0 && KB_STORE[i].unit != unit) continue;

        /* Extract first line of answer as the topic title */
        char title[128];
        const char *nl = strchr(KB_STORE[i].answer, '\n');
        int tlen = nl ? (int)(nl - KB_STORE[i].answer) : (int)strlen(KB_STORE[i].answer);
        if (tlen > 127) tlen = 127;
        strncpy(title, KB_STORE[i].answer, tlen);
        title[tlen] = '\0';
        /* Strip trailing colon if present */
        int tl = (int)strlen(title);
        if (tl > 0 && title[tl-1] == ':') title[--tl] = '\0';

        pos += snprintf(response_out + pos, MAX_RESPONSE_LEN - pos,
            "  %2d. %s\n", ++count, title);
    }

    if (count == 0) {
        pos += snprintf(response_out + pos, MAX_RESPONSE_LEN - pos,
            "  (No topics loaded for this unit yet)\n");
    }

    snprintf(response_out + pos, MAX_RESPONSE_LEN - pos,
        "  ------------------------------------------------------------\n"
        "  Commands: quiz | units | menu | exit\n"
        "  Tip: type keywords from a topic title to ask about it.\n");
}

/* ================================================================== */
/*  QUIZ DATA — unit-tagged questions for each subject                */
/*  unit=0 means question applies to all units                        */
/* ================================================================== */

/* Helper macro to shorten struct init: topic, unit */
#define QQ(t,u) t,u

static const QuizQuestion PHYSICS_QUIZ[] = {
    /* Unit 1: Quantum Mechanics */
    {"de Broglie wavelength of a particle is:",
     {"A) h/mv","B) mv/h","C) h*mv","D) m/hv"},
     0,"lambda = h/p = h/(mv). Lighter/slower particle => larger wavelength.",QQ(TOPIC_PHYSICS,1)},
    {"Heisenberg uncertainty: delta_x * delta_p >=",
     {"A) h","B) h/2pi","C) h/(4pi)","D) 2h"},
     2,"Uncertainty principle: delta_x * delta_p >= h/(4*pi).",QQ(TOPIC_PHYSICS,1)},
    {"Energy of particle in a box: E_n is proportional to:",
     {"A) n","B) n^2","C) 1/n","D) sqrt(n)"},
     1,"E_n = n^2*h^2/(8ma^2). Energy scales as n^2.",QQ(TOPIC_PHYSICS,1)},
    {"At E = EF (Fermi level), probability of occupancy f(E) =",
     {"A) 0","B) 1","C) 0.5","D) depends on T"},
     2,"Fermi factor f(EF) = 1/[1+exp(0)] = 1/2 = 0.5 at ALL temperatures.",QQ(TOPIC_PHYSICS,1)},
    /* Unit 2: Electrical Conductivity */
    {"Classical free electron theory FAILS to explain:",
     {"A) Ohm's law","B) Specific heat of metals","C) Existence of current","D) Wire resistance"},
     1,"CFET predicts Cv=(3/2)R but experiment gives ~10^-4 RT. Quantum theory needed.",QQ(TOPIC_PHYSICS,2)},
    {"Hall coefficient RH is positive for:",
     {"A) N-type materials","B) P-type materials","C) Insulators","D) Superconductors"},
     1,"Positive RH means majority carriers are holes (p-type / some metals like Zn).",QQ(TOPIC_PHYSICS,2)},
    {"Forbidden energy gap of silicon is approximately:",
     {"A) 0.07 eV","B) 0.67 eV","C) 1.1 eV","D) 5.4 eV"},
     2,"Silicon Eg = 1.1 eV. Ge = 0.67 eV. Diamond (insulator) = 5.4 eV.",QQ(TOPIC_PHYSICS,2)},
    /* Unit 3: Superconductivity */
    {"The Meissner effect refers to:",
     {"A) Zero resistance","B) Expulsion of magnetic flux","C) Phonon-mediated pairing","D) Energy gap at Fermi level"},
     1,"Meissner effect: superconductor expels all magnetic flux (B=0 inside).",QQ(TOPIC_PHYSICS,3)},
    {"Cooper pairs in BCS theory are formed by:",
     {"A) Direct Coulomb attraction","B) Phonon-mediated interaction","C) Magnetic force","D) Gravity"},
     1,"BCS: electron-phonon-electron interaction creates effective attractive force.",QQ(TOPIC_PHYSICS,3)},
    {"Type II superconductors differ from Type I because they have:",
     {"A) Higher resistance","B) No Meissner effect","C) Two critical fields Hc1 and Hc2","D) Only one critical temperature"},
     2,"Type II: partial flux penetration (vortex state) between Hc1 and Hc2.",QQ(TOPIC_PHYSICS,3)},
    /* Unit 4: Lasers & Optical Fibers */
    {"Laser action requires:",
     {"A) Population inversion","B) Thermal equilibrium","C) Ground state dominance","D) High pressure"},
     0,"Population inversion (N2>N1) is essential for stimulated emission to dominate.",QQ(TOPIC_PHYSICS,4)},
    {"Ruby laser output wavelength is approximately:",
     {"A) 532 nm (green)","B) 694 nm (red)","C) 1064 nm (infrared)","D) 488 nm (blue)"},
     1,"Ruby laser emits at 694.3 nm (deep red). Pulsed output, three-level system.",QQ(TOPIC_PHYSICS,4)},
    {"Numerical Aperture (NA) of an optical fiber equals:",
     {"A) n1/n2","B) n1-n2","C) sqrt(n1^2 - n2^2)","D) n1*n2"},
     2,"NA = sqrt(n1^2 - n2^2). Determines acceptance angle of the fiber.",QQ(TOPIC_PHYSICS,4)}
};

static const QuizQuestion CIVIL_QUIZ[] = {
    {"How many main types of staircases are there?",
     {"A) 4","B) 6","C) 8","D) 10"},
     2,"8 types: straight, dog-legged, open-well, spiral, bifurcated, quarter-turn, three-quarter turn, circular.",QQ(TOPIC_CIVIL_ENGINEERING,0)},
    {"Minimum concrete grade for RCC per IS 456:",
     {"A) M10","B) M15","C) M20","D) M25"},
     2,"IS 456 mandates M20 minimum for RCC.",QQ(TOPIC_CIVIL_ENGINEERING,0)},
    {"Dog-legged staircase has how many flights?",
     {"A) 1","B) 2","C) 3","D) 4"},
     1,"Dog-legged staircase has two flights turning 180 degrees at a common landing.",QQ(TOPIC_CIVIL_ENGINEERING,0)},
    {"Slump cone height per IS 1199:",
     {"A) 200mm","B) 250mm","C) 300mm","D) 350mm"},
     2,"Slump cone: bottom=200mm, top=100mm, height=300mm.",QQ(TOPIC_CIVIL_ENGINEERING,0)},
    {"Which dam type resists water pressure by its own weight?",
     {"A) Arch dam","B) Gravity dam","C) Earthen dam","D) Buttress dam"},
     1,"Gravity dam relies on its own massive weight to resist water pressure.",QQ(TOPIC_CIVIL_ENGINEERING,0)}
};

static const QuizQuestion MATHS_QUIZ[] = {
    /* Unit 2: Vector Differentiation */
    {"If |A| is constant, then A . (dA/dt) =",
     {"A) |A|","B) 1","C) 0","D) |A|^2"},
     2,"If magnitude is constant, A is perpendicular to its derivative, so dot product = 0.",QQ(TOPIC_MATHS,2)},
    {"div(F) = 0 means F is:",
     {"A) Irrotational","B) Solenoidal","C) Conservative","D) Scalar"},
     1,"div(F)=0 means solenoidal (no sources/sinks). curl(F)=0 means irrotational.",QQ(TOPIC_MATHS,2)},
    {"curl(grad(phi)) is always:",
     {"A) grad(phi)","B) div(phi)","C) zero vector","D) phi"},
     2,"curl of any gradient is identically zero. This is a vector identity.",QQ(TOPIC_MATHS,2)},
    /* Unit 3: Vector Integration */
    {"Green's theorem converts a line integral to a:",
     {"A) Volume integral","B) Surface integral","C) Double integral","D) Triple integral"},
     2,"Green's theorem: closed line integral = double integral over the enclosed region.",QQ(TOPIC_MATHS,3)},
    {"Gauss's divergence theorem relates surface integral to:",
     {"A) Line integral","B) Volume integral","C) Double integral","D) Path integral"},
     1,"Gauss: flux through closed surface = volume integral of divergence inside.",QQ(TOPIC_MATHS,3)},
    {"Stokes' theorem: oint_C F.dr = iint_S (curl F).dS. C is the ___ of S.",
     {"A) Interior","B) Normal","C) Boundary","D) Gradient"},
     2,"Stokes: C is the boundary curve of surface S.",QQ(TOPIC_MATHS,3)},
    /* Unit 4: Linear ODEs */
    {"For auxiliary equation roots m = 2+3i, 2-3i, the complementary function is:",
     {"A) e^2x(C1cos3x+C2sin3x)","B) (C1+C2x)e^2x","C) C1e^2x+C2e^3x","D) e^3x(C1cos2x+C2sin2x)"},
     0,"Complex roots a+/-bi -> yc = e^(ax)[C1*cos(bx)+C2*sin(bx)].",QQ(TOPIC_MATHS,4)},
    {"1/F(D) * e^(ax) when F(a)=0 equals:",
     {"A) e^(ax)/F(a)","B) x*e^(ax)/F'(a)","C) e^(ax)*F(a)","D) 0"},
     1,"If F(a)=0, multiply by x: yp = x*e^(ax)/F'(a).",QQ(TOPIC_MATHS,4)},
    {"d/dx(x^5) =",
     {"A) 5x^4","B) x^4","C) 5x^6","D) x^6/6"},
     0,"Power rule: d/dx(x^n)=n*x^(n-1). So d/dx(x^5)=5x^4.",QQ(TOPIC_MATHS,0)}
};

static const QuizQuestion AI_QUIZ[] = {
    {"Which ML type uses labelled data?",
     {"A) Unsupervised","B) Reinforcement","C) Supervised","D) Generative"},
     2,"Supervised learning trains on labelled input-output pairs.",QQ(TOPIC_AI,0)},
    {"ReLU activation function is:",
     {"A) 1/(1+e^-x)","B) max(0,x)","C) tanh(x)","D) x^2"},
     1,"ReLU = max(0,x). Outputs 0 for negative inputs, x for positive.",QQ(TOPIC_AI,0)},
    {"A* search uses f(n) =",
     {"A) h(n)","B) g(n)","C) g(n)+h(n)","D) g(n)*h(n)"},
     2,"A*: f(n)=g(n)+h(n). g=cost so far, h=heuristic estimate to goal.",QQ(TOPIC_AI,0)},
    {"Overfitting causes:",
     {"A) High train + high test error","B) Low train + high test error","C) Low train + low test error","D) High train + low test error"},
     1,"Overfitting: memorises training data => low train error, high test error.",QQ(TOPIC_AI,0)},
    {"PCA reduces dimensionality by finding directions of:",
     {"A) Minimum variance","B) Maximum variance","C) Zero variance","D) Median variance"},
     1,"PCA finds principal components = directions of maximum variance in data.",QQ(TOPIC_AI,0)}
};

static const QuizQuestion CPROG_QUIZ[] = {
    /* Unit 1: Introduction */
    {"C language was developed by:",
     {"A) Bjarne Stroustrup","B) Dennis Ritchie","C) James Gosling","D) Guido van Rossum"},
     1,"Dennis Ritchie developed C at Bell Labs (1972). Used to write UNIX.",QQ(TOPIC_C_PROGRAMMING,1)},
    {"Which header file is needed for printf and scanf?",
     {"A) stdlib.h","B) string.h","C) stdio.h","D) math.h"},
     2,"stdio.h (standard input/output) contains printf, scanf, fgets, puts, etc.",QQ(TOPIC_C_PROGRAMMING,1)},
    {"The entry point of every C program is:",
     {"A) start()","B) begin()","C) init()","D) main()"},
     3,"Every C program must have a main() function; execution begins there.",QQ(TOPIC_C_PROGRAMMING,1)},
    /* Unit 2: Strings & Functions */
    {"strlen(\"Hello\") returns:",
     {"A) 6","B) 5","C) 4","D) 0"},
     1,"strlen counts characters up to but NOT including '\\0'. \"Hello\" has 5 chars.",QQ(TOPIC_C_PROGRAMMING,2)},
    {"static local variable:",
     {"A) Makes it const","B) Stores in register","C) Persists between function calls","D) Makes it global"},
     2,"static local: initialised once; retains value between function calls.",QQ(TOPIC_C_PROGRAMMING,2)},
    {"Recursion MUST have a:",
     {"A) Return type void","B) Base case","C) Global variable","D) Loop inside"},
     1,"Without a base case, recursion never stops and causes stack overflow.",QQ(TOPIC_C_PROGRAMMING,2)},
    /* Unit 3: 2D Arrays */
    {"int a[3][4] has how many elements?",
     {"A) 7","B) 3","C) 4","D) 12"},
     3,"Total elements = rows * cols = 3 * 4 = 12.",QQ(TOPIC_C_PROGRAMMING,3)},
    {"C stores 2D arrays in:",
     {"A) Column-major order","B) Row-major order","C) Random order","D) Diagonal order"},
     1,"C uses row-major order: all elements of row 0, then row 1, etc.",QQ(TOPIC_C_PROGRAMMING,3)},
    /* Unit 4: Structures & Pointers */
    {"Struct member access via pointer uses:",
     {"A) .","B) ->","C) *","D) &"},
     1,"Arrow operator (->) accesses struct member through pointer: ptr->member.",QQ(TOPIC_C_PROGRAMMING,4)},
    /* Unit 5: Dynamic Memory */
    {"calloc() vs malloc(): calloc also:",
     {"A) Is faster","B) Zero-initialises memory","C) Allocates on stack","D) Takes one argument"},
     1,"calloc(n,size) allocates n*size bytes AND zero-initialises all bytes.",QQ(TOPIC_C_PROGRAMMING,5)},
    {"free() in C is used to:",
     {"A) Release stack memory","B) Release heap memory","C) Clear a variable","D) Reset a pointer to NULL"},
     1,"free() releases memory previously allocated by malloc/calloc/realloc on the heap.",QQ(TOPIC_C_PROGRAMMING,5)}
};

static const QuizQuestion CHEMISTRY_QUIZ[] = {
    {"Avogadro's number:",
     {"A) 6.022e23","B) 6.022e20","C) 3.14e23","D) 9.8e23"},
     0,"1 mole = 6.022 x 10^23 particles.",QQ(TOPIC_CHEMISTRY,0)},
    {"pH=3 means solution is:",
     {"A) Basic","B) Neutral","C) Acidic","D) Amphoteric"},
     2,"pH < 7 = acidic. pH=3 = strongly acidic ([H+]=10^-3 mol/L).",QQ(TOPIC_CHEMISTRY,0)},
    {"Oxidation means:",
     {"A) Gain of electrons","B) Loss of electrons","C) Gain of protons","D) Loss of protons"},
     1,"OIL RIG: Oxidation Is Loss (of electrons). Reduction Is Gain.",QQ(TOPIC_CHEMISTRY,0)},
    {"Ideal Gas Law:",
     {"A) PV=nT","B) P/V=nR","C) PV=nRT","D) PT=nRV"},
     2,"PV=nRT. P=pressure, V=volume, n=moles, R=8.314 J/mol/K, T=Kelvin.",QQ(TOPIC_CHEMISTRY,0)},
    {"Ionic bond involves:",
     {"A) Equal electron sharing","B) Unequal sharing","C) Electron transfer metal->non-metal","D) Electron transfer non-metal->metal"},
     2,"Ionic bond: metal TRANSFERS electrons to non-metal. Example: Na->Cl in NaCl.",QQ(TOPIC_CHEMISTRY,0)}
};

static const QuizQuestion ELECTRONICS_QUIZ[] = {
    {"Forward voltage drop of silicon diode:",
     {"A) 0V","B) 0.3V","C) 0.7V","D) 1.4V"},
     2,"Silicon diode forward voltage ~0.7V. Germanium ~0.3V.",QQ(TOPIC_ELECTRONICS,0)},
    {"In BJT, IC = beta * IB. Beta is:",
     {"A) Base voltage","B) Current gain","C) Collector resistance","D) Emitter current"},
     1,"Beta (hFE) = DC current gain of transistor: IC=beta*IB.",QQ(TOPIC_ELECTRONICS,0)},
    {"Universal logic gate:",
     {"A) AND","B) OR","C) XOR","D) NAND"},
     3,"NAND (and NOR) are universal gates — any logic function can be built from them.",QQ(TOPIC_ELECTRONICS,0)},
    {"PWM duty cycle 25% with 12V supply gives average output:",
     {"A) 12V","B) 6V","C) 3V","D) 9V"},
     2,"V_avg = duty_cycle * Vsupply = 0.25 * 12 = 3V.",QQ(TOPIC_ELECTRONICS,0)},
    {"555 timer in astable mode acts as:",
     {"A) One-shot pulse generator","B) SR flip-flop","C) Free-running oscillator","D) Voltage regulator"},
     2,"Astable mode: no stable state; oscillates continuously as square wave generator.",QQ(TOPIC_ELECTRONICS,0)}
};

/* ================================================================== */
/*  get_quiz_question and get_quiz_count — with unit filtering        */
/* ================================================================== */

/*
 * get_all_quiz — returns the base array and size for a topic.
 * Internal helper used by the two public functions.
 */
static const QuizQuestion *get_topic_quiz(SubjectTopic topic, int *total_out)
{
    switch (topic) {
        case TOPIC_PHYSICS:
            *total_out = (int)(sizeof(PHYSICS_QUIZ)/sizeof(PHYSICS_QUIZ[0]));
            return PHYSICS_QUIZ;
        case TOPIC_CIVIL_ENGINEERING:
            *total_out = (int)(sizeof(CIVIL_QUIZ)/sizeof(CIVIL_QUIZ[0]));
            return CIVIL_QUIZ;
        case TOPIC_MATHS:
            *total_out = (int)(sizeof(MATHS_QUIZ)/sizeof(MATHS_QUIZ[0]));
            return MATHS_QUIZ;
        case TOPIC_AI:
            *total_out = (int)(sizeof(AI_QUIZ)/sizeof(AI_QUIZ[0]));
            return AI_QUIZ;
        case TOPIC_C_PROGRAMMING:
            *total_out = (int)(sizeof(CPROG_QUIZ)/sizeof(CPROG_QUIZ[0]));
            return CPROG_QUIZ;
        case TOPIC_CHEMISTRY:
            *total_out = (int)(sizeof(CHEMISTRY_QUIZ)/sizeof(CHEMISTRY_QUIZ[0]));
            return CHEMISTRY_QUIZ;
        case TOPIC_ELECTRONICS:
            *total_out = (int)(sizeof(ELECTRONICS_QUIZ)/sizeof(ELECTRONICS_QUIZ[0]));
            return ELECTRONICS_QUIZ;
        default: *total_out = 0; return NULL;
    }
}

int get_quiz_question(SubjectTopic topic, int unit, int index, QuizQuestion *q_out)
{
    int total = 0, nth = 0, i;
    const QuizQuestion *arr = get_topic_quiz(topic, &total);
    if (!arr || !q_out) return 0;
    for (i = 0; i < total; i++) {
        /* include if unit==0 (all) OR question's unit==0 OR question's unit matches */
        if (unit == 0 || arr[i].unit == 0 || arr[i].unit == unit) {
            if (nth == index) { *q_out = arr[i]; return 1; }
            nth++;
        }
    }
    return 0;
}

int get_quiz_count(SubjectTopic topic, int unit)
{
    int total = 0, count = 0, i;
    const QuizQuestion *arr = get_topic_quiz(topic, &total);
    if (!arr) return 0;
    for (i = 0; i < total; i++) {
        if (unit == 0 || arr[i].unit == 0 || arr[i].unit == unit)
            count++;
    }
    return count > 0 ? count : total; /* fallback: show all if none match */
}
