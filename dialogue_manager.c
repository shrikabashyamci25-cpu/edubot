#include "assistant.h"

/* ------------------------------------------------------------------ */
/* Command keyword tables                                              */
/* ------------------------------------------------------------------ */
static const char *EXIT_KW[]  = {"exit","quit","bye","goodbye",NULL};
static const char *MENU_KW[]  = {"menu","back","home","return","change subject",NULL};
static const char *QUIZ_KW[]  = {"quiz","test","practice","mcq",NULL};
static const char *YES_KW[]   = {"yes","y","yeah","yep","continue","sure",NULL};
static const char *NO_KW[]    = {"no","n","nope","stop","end","quit",NULL};

/* Topic keywords */
static const char *PHYS_KW[]  = {"physics","1",NULL};
static const char *CIVIL_KW[] = {"civil engineering","civil","2",NULL};
static const char *MATH_KW[]  = {"maths","math","mathematics","calculus","algebra","3",NULL};
static const char *AI_KW[]    = {"artificial intelligence","machine learning","ai","deep learning","4",NULL};
static const char *CPROG_KW[] = {"c programming","c language","cprogramming","pointers in c","5",NULL};
static const char *CHEM_KW[]  = {"chemistry","chemical","periodic table","6",NULL};
static const char *ELEC_KW[]  = {"electronics","electronic","circuit","diode","transistor","7",NULL};

/* ------------------------------------------------------------------ */
/* contains_keyword                                                    */
/* Returns 1 if any keyword from the NULL-terminated list appears as  */
/* a substring of str.                                                 */
/* ------------------------------------------------------------------ */
static int contains_keyword(const char *str, const char **kw) {
    int i;
    for (i = 0; kw[i]; i++)
        if (strstr(str, kw[i])) return 1;
    return 0;
}

/* ------------------------------------------------------------------ */
/* is_help_command                                                     */
/* BUG FIX: 'help' only triggers the command banner when the user     */
/* types it as a standalone word (not inside a real question like     */
/* "help me understand X"). We check that the normalised input is     */
/* EXACTLY "help" or one of a few explicit help phrases.              */
/* ------------------------------------------------------------------ */
static int is_help_command(const char *norm) {
    if (strcmp(norm, "help") == 0)          return 1;
    if (strcmp(norm, "commands") == 0)      return 1;
    if (strcmp(norm, "show help") == 0)     return 1;
    if (strcmp(norm, "show commands") == 0) return 1;
    if (strcmp(norm, "options") == 0)       return 1;
    return 0;
}

/* ------------------------------------------------------------------ */
/* topic_name  (internal helper) — must come before unit_menu        */
/* ------------------------------------------------------------------ */
static const char *topic_name(SubjectTopic t) {
    switch (t) {
        case TOPIC_PHYSICS:           return "Physics";
        case TOPIC_CIVIL_ENGINEERING: return "Civil Engineering";
        case TOPIC_MATHS:             return "Mathematics";
        case TOPIC_AI:                return "Artificial Intelligence";
        case TOPIC_C_PROGRAMMING:     return "C Programming";
        case TOPIC_CHEMISTRY:         return "Chemistry";
        case TOPIC_ELECTRONICS:       return "Electronics";
        default:                      return "Unknown";
    }
}

/* ------------------------------------------------------------------ */
/* unit_has_chapters — subjects that expose a unit/chapter menu       */
/* ------------------------------------------------------------------ */
static int unit_has_chapters(SubjectTopic t) {
    return (t == TOPIC_PHYSICS || t == TOPIC_MATHS || t == TOPIC_C_PROGRAMMING);
}

/* Unit names per subject (index 0 = unit 1) */
static const char *PHYS_UNITS[] = {
    "Quantum Mechanics",
    "Electrical Conductivity",
    "Superconductivity",
    "Lasers & Optical Fibers",
    "Semiconductor Physics"
};
static const char *MATH_UNITS[] = {
    "Sequences & Series",
    "Vector Differentiation",
    "Vector Integration",
    "Linear Ordinary Differential Equations",
    "Numerical Methods"
};
static const char *CPROG_UNITS[] = {
    "Logical Reasoning & Algorithms",
    "Decision Controls & Loops",
    "Arrays & Strings",
    "Functions, Structures & Pointers",
    "Dynamic Memory & Linked Lists"
};

static const char *get_unit_name(SubjectTopic t, int unit) {
    if (unit < 1 || unit > 5) return "Unknown";
    switch (t) {
        case TOPIC_PHYSICS:       return PHYS_UNITS[unit-1];
        case TOPIC_MATHS:         return MATH_UNITS[unit-1];
        case TOPIC_C_PROGRAMMING: return CPROG_UNITS[unit-1];
        default: return "All";
    }
}

static void unit_menu(SubjectTopic topic, char *out) {
    const char *n = topic_name(topic);
    int pos = snprintf(out, MAX_RESPONSE_LEN,
        "\n============================================================\n"
        "  %s -- SELECT A UNIT / CHAPTER\n"
        "============================================================\n"
        "  [0] All Units\n", n);
    int u;
    for (u = 1; u <= 5 && pos < MAX_RESPONSE_LEN - 80; u++)
        pos += snprintf(out + pos, MAX_RESPONSE_LEN - pos,
            "  [%d] Unit %d: %s\n", u, u, get_unit_name(topic, u));
    snprintf(out + pos, MAX_RESPONSE_LEN - pos,
        "------------------------------------------------------------\n"
        "  Type a number (0-5), or 'menu' to go back.");
}

/* ------------------------------------------------------------------ */
/* dm_init_session                                                     */
/* ------------------------------------------------------------------ */
void dm_init_session(SessionState *s) {
    if (!s) return;
    memset(s, 0, sizeof(SessionState));
    s->current_state = STATE_GREETING;
    s->current_topic = TOPIC_NONE;
    s->current_unit  = 0;
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(s->session_start, sizeof(s->session_start), "%Y-%m-%d %H:%M:%S", t);
}

/* ------------------------------------------------------------------ */
/* dm_get_greeting                                                     */
/* ------------------------------------------------------------------ */
void dm_get_greeting(char *out) {
    snprintf(out, MAX_RESPONSE_LEN,
        "============================================================\n"
        "       EduBot -- Text-Based Educational Assistant\n"
        "============================================================\n"
        "  Subjects available:\n"
        "    [1] Physics\n"
        "    [2] Civil Engineering\n"
        "    [3] Mathematics\n"
        "    [4] Artificial Intelligence\n"
        "    [5] C Programming\n"
        "    [6] Chemistry\n"
        "    [7] Electronics\n"
        "------------------------------------------------------------\n"
        "  Please enter your name to begin:");
}

/* ------------------------------------------------------------------ */
/* dm_get_topic_prompt                                                 */
/* ------------------------------------------------------------------ */
void dm_get_topic_prompt(char *out) {
    snprintf(out, MAX_RESPONSE_LEN,
        "\n============================================================\n"
        "  SELECT A SUBJECT\n"
        "============================================================\n"
        "  [1] Physics            [2] Civil Engineering\n"
        "  [3] Mathematics        [4] Artificial Intelligence\n"
        "  [5] C Programming      [6] Chemistry\n"
        "  [7] Electronics\n"
        "------------------------------------------------------------\n"
        "  Type a number or subject name. Type 'exit' to quit.");
}

/* ------------------------------------------------------------------ */
/* qa_banner  (internal)                                              */
/* ------------------------------------------------------------------ */
static void qa_banner(SubjectTopic topic, int unit, char *out) {
    const char *n = topic_name(topic);
    char unit_str[64];
    if (unit > 0)
        snprintf(unit_str, sizeof(unit_str), "Unit %d: %s", unit, get_unit_name(topic, unit));
    else
        snprintf(unit_str, sizeof(unit_str), "All Units");
    snprintf(out, MAX_RESPONSE_LEN,
        "\n============================================================\n"
        "  %s (%s) -- Q&A MODE\n"
        "============================================================\n"
        "  Ask any question about %s.\n"
        "  Commands:\n"
        "    quiz     - Start a multiple-choice quiz\n"
        "    units    - Change unit/chapter\n"
        "    menu     - Return to subject selection\n"
        "    help     - Show this help (type 'help' alone)\n"
        "    exit     - Quit EduBot\n"
        "------------------------------------------------------------",
        n, unit_str, n);
}

/* ------------------------------------------------------------------ */
/* format_quiz_question  (internal)                                   */
/* ------------------------------------------------------------------ */
static void format_quiz_q(const QuizQuestion *q, int num, int total, char *out) {
    char buf[MAX_RESPONSE_LEN];
    int  pos = 0, i;
    pos += snprintf(buf+pos, sizeof(buf)-pos,
        "\n============================================================\n"
        "  QUIZ -- Question %d of %d\n"
        "============================================================\n"
        "  %s\n\n", num, total, q->question);
    for (i = 0; i < NUM_QUIZ_OPTIONS && pos < (int)sizeof(buf)-4; i++)
        pos += snprintf(buf+pos, sizeof(buf)-pos, "  %s\n", q->options[i]);
    pos += snprintf(buf+pos, sizeof(buf)-pos,
        "------------------------------------------------------------\n"
        "  Enter your answer (A / B / C / D):");
    strncpy(out, buf, MAX_RESPONSE_LEN-1);
    out[MAX_RESPONSE_LEN-1] = '\0';
}

/* ------------------------------------------------------------------ */
/* dm_run_quiz_step                                                    */
/* ------------------------------------------------------------------ */
void dm_run_quiz_step(SessionState *session, const char *raw_input, char *response_out)
{
    if (!session || !raw_input || !response_out) return;
    int total = get_quiz_count(session->current_topic, session->current_unit);

    /* Phase C: awaiting yes/no at a checkpoint (every QUIZ_BATCH_SIZE questions) */
    if (session->quiz_awaiting_continue) {
        char cnorm[MAX_INPUT_LEN];
        strncpy(cnorm, raw_input, MAX_INPUT_LEN-1); cnorm[MAX_INPUT_LEN-1] = '\0';
        normalize_string(cnorm);

        if (contains_keyword(cnorm, YES_KW)) {
            session->quiz_awaiting_continue = 0;
            QuizQuestion qn; char nb[MAX_RESPONSE_LEN];
            if (get_quiz_question(session->current_topic, session->current_unit, session->quiz_question_index, &qn))
                format_quiz_q(&qn, session->quiz_question_index + 1, total, nb);
            else
                strncpy(nb, "Error loading next question.", MAX_RESPONSE_LEN-1);
            strncpy(response_out, nb, MAX_RESPONSE_LEN-1); response_out[MAX_RESPONSE_LEN-1]='\0';
            return;
        }
        if (contains_keyword(cnorm, NO_KW)) {
            int sc = session->quiz_score, done = session->quiz_question_index;
            float pct = done > 0 ? (100.0f * sc / done) : 0.0f;
            const char *grade =
                pct>=90?"A+": pct>=75?"A": pct>=60?"B": pct>=50?"C": "F";
            snprintf(response_out, MAX_RESPONSE_LEN,
                "\n============================================================\n"
                "  QUIZ ENDED\n"
                "============================================================\n"
                "  Score: %d / %d  (%.0f%%)   Grade: %s\n"
                "------------------------------------------------------------\n"
                "  Type 'quiz' to start again, 'menu' to change subject,\n"
                "  or ask any question to continue Q&A mode.\n"
                "============================================================",
                sc, done, pct, grade);
            session->quiz_awaiting_continue = 0;
            session->quiz_question_index    = 0;
            session->quiz_total             = 0;
            session->current_state          = STATE_QA_MODE;
            return;
        }
        snprintf(response_out, MAX_RESPONSE_LEN,
            "  [!] Please answer yes or no — continue the quiz?");
        return;
    }

    /* Phase A: first entry — show question 0 */
    if (session->quiz_total == 0) {
        session->quiz_score          = 0;
        session->quiz_total          = total;
        session->quiz_question_index = 0;
        QuizQuestion q;
        if (get_quiz_question(session->current_topic, session->current_unit, 0, &q))
            format_quiz_q(&q, 1, total, response_out);
        else {
            snprintf(response_out, MAX_RESPONSE_LEN, "No quiz questions found.");
            session->current_state = STATE_QA_MODE;
        }
        return;
    }

    /* Phase B: validate answer */
    char norm[MAX_INPUT_LEN];
    strncpy(norm, raw_input, MAX_INPUT_LEN-1); norm[MAX_INPUT_LEN-1] = '\0';
    normalize_string(norm);

    char ac = '\0';
    int  k;
    for (k = 0; norm[k]; k++) if (norm[k] != ' ') { ac = norm[k]; break; }

    if (ac < 'a' || ac > 'd') {
        snprintf(response_out, MAX_RESPONSE_LEN,
            "  [!] Invalid input. Please enter A, B, C, or D."); return;
    }

    int user_idx = ac - 'a';
    QuizQuestion qc;
    if (!get_quiz_question(session->current_topic, session->current_unit, session->quiz_question_index, &qc)) {
        snprintf(response_out, MAX_RESPONSE_LEN, "Error retrieving question.");
        session->current_state = STATE_QA_MODE; return;
    }

    char fb[MAX_RESPONSE_LEN]; int fpos = 0;
    if (user_idx == qc.correct_option) {
        session->quiz_score++;
        fpos += snprintf(fb+fpos, sizeof(fb)-fpos, "\n  [CORRECT!] Well done!\n");
    } else {
        fpos += snprintf(fb+fpos, sizeof(fb)-fpos,
            "\n  [INCORRECT] Correct answer: %c\n", 'A' + qc.correct_option);
    }
    fpos += snprintf(fb+fpos, sizeof(fb)-fpos,
        "  Explanation: %s\n", qc.explanation);

    session->quiz_question_index++;

    if (session->quiz_question_index >= total) {
        int sc = session->quiz_score, tot = session->quiz_total;
        float pct = tot > 0 ? (100.0f * sc / tot) : 0.0f;
        const char *grade =
            pct>=90?"A+": pct>=75?"A": pct>=60?"B": pct>=50?"C": "F";
        snprintf(response_out, MAX_RESPONSE_LEN,
            "%s\n============================================================\n"
            "  QUIZ COMPLETE!\n"
            "============================================================\n"
            "  Score: %d / %d  (%.0f%%)   Grade: %s\n"
            "------------------------------------------------------------\n"
            "  Type 'quiz' to retry, 'menu' to change subject,\n"
            "  or ask any question to continue Q&A mode.\n"
            "============================================================",
            fb, sc, tot, pct, grade);
        session->quiz_question_index    = 0;
        session->quiz_total             = 0;
        session->quiz_awaiting_continue = 0;
        session->current_state          = STATE_QA_MODE;
    } else if (session->quiz_question_index % QUIZ_BATCH_SIZE == 0) {
        /* Checkpoint: a batch of QUIZ_BATCH_SIZE questions is done, more remain */
        session->quiz_awaiting_continue = 1;
        snprintf(response_out, MAX_RESPONSE_LEN,
            "%s\n============================================================\n"
            "  Checkpoint: %d of %d questions done. Score so far: %d/%d\n"
            "------------------------------------------------------------\n"
            "  Continue with the quiz? (yes/no)\n"
            "============================================================",
            fb, session->quiz_question_index, total, session->quiz_score, session->quiz_question_index);
    } else {
        QuizQuestion qn; char nb[MAX_RESPONSE_LEN];
        if (get_quiz_question(session->current_topic, session->current_unit, session->quiz_question_index, &qn))
            format_quiz_q(&qn, session->quiz_question_index + 1, total, nb);
        else
            strncpy(nb, "Error loading next question.", MAX_RESPONSE_LEN-1);
        snprintf(response_out, MAX_RESPONSE_LEN, "%s%s", fb, nb);
    }
}

/* ------------------------------------------------------------------ */
/* dm_process_input — core state machine dispatcher                   */
/* ------------------------------------------------------------------ */
void dm_process_input(SessionState *session, const char *raw_input, char *response_out)
{
    if (!session || !raw_input || !response_out) return;

    char norm[MAX_INPUT_LEN];
    strncpy(norm, raw_input, MAX_INPUT_LEN-1); norm[MAX_INPUT_LEN-1] = '\0';
    normalize_string(norm);

    /* ==============================================================
     * STATE_GREETING: collect name
     * ============================================================== */
    if (session->current_state == STATE_GREETING) {
        if (strlen(norm) == 0) {
            snprintf(response_out, MAX_RESPONSE_LEN, "  Please enter your name:"); return;
        }
        strncpy(session->user_name, raw_input, 63); session->user_name[63] = '\0';
        int nl = (int)strlen(session->user_name) - 1;
        while (nl >= 0 && (session->user_name[nl]=='\n' ||
                           session->user_name[nl]=='\r' ||
                           session->user_name[nl]==' '))
            session->user_name[nl--] = '\0';

        session->current_state = STATE_TOPIC_SELECT;
        char tp[MAX_RESPONSE_LEN]; dm_get_topic_prompt(tp);
        snprintf(response_out, MAX_RESPONSE_LEN,
            "\n  Welcome, %s! Let's get started.\n%s", session->user_name, tp);
        return;
    }

    /* ==============================================================
     * Global: exit
     * ============================================================== */
    if (contains_keyword(norm, EXIT_KW)) {
        session->current_state = STATE_EXIT;
        snprintf(response_out, MAX_RESPONSE_LEN,
            "\n============================================================\n"
            "  Thank you for studying with EduBot, %s!\n"
            "  Session saved to '%s'. Keep learning!\n"
            "============================================================",
            session->user_name, LOG_FILENAME);
        return;
    }

    /* ==============================================================
     * STATE_TOPIC_SELECT
     * ============================================================== */
    if (session->current_state == STATE_TOPIC_SELECT) {
        SubjectTopic chosen = TOPIC_NONE;

        if      (contains_keyword(norm, PHYS_KW))  chosen = TOPIC_PHYSICS;
        else if (contains_keyword(norm, CIVIL_KW)) chosen = TOPIC_CIVIL_ENGINEERING;
        else if (contains_keyword(norm, MATH_KW))  chosen = TOPIC_MATHS;
        else if (contains_keyword(norm, AI_KW))    chosen = TOPIC_AI;
        else if (contains_keyword(norm, CPROG_KW)) chosen = TOPIC_C_PROGRAMMING;
        else if (contains_keyword(norm, CHEM_KW))  chosen = TOPIC_CHEMISTRY;
        else if (contains_keyword(norm, ELEC_KW))  chosen = TOPIC_ELECTRONICS;

        if (chosen != TOPIC_NONE) {
            session->current_topic = chosen;
            session->current_unit  = 0;
            if (unit_has_chapters(chosen)) {
                session->current_state = STATE_UNIT_SELECT;
                char um[MAX_RESPONSE_LEN];
                unit_menu(chosen, um);
                snprintf(response_out, MAX_RESPONSE_LEN,
                    "\n  %s selected.%s", topic_name(chosen), um);
            } else {
                session->current_state = STATE_QA_MODE;
                char banner[MAX_RESPONSE_LEN];
                qa_banner(chosen, 0, banner);
                snprintf(response_out, MAX_RESPONSE_LEN,
                    "\n  %s selected.%s", topic_name(chosen), banner);
            }
        } else {
            char tp[MAX_RESPONSE_LEN]; dm_get_topic_prompt(tp);
            snprintf(response_out, MAX_RESPONSE_LEN,
                "  [!] Subject not recognised.%s", tp);
        }
        return;
    }

    /* ==============================================================
     * STATE_UNIT_SELECT
     * ============================================================== */
    if (session->current_state == STATE_UNIT_SELECT) {
        if (contains_keyword(norm, MENU_KW)) {
            session->current_state = STATE_TOPIC_SELECT;
            session->current_topic = TOPIC_NONE;
            session->current_unit  = 0;
            char tp[MAX_RESPONSE_LEN]; dm_get_topic_prompt(tp);
            snprintf(response_out, MAX_RESPONSE_LEN,
                "\n  Returning to subject selection.\n%s", tp);
            return;
        }

        /* Parse "units" command to re-show the menu */
        if (strcmp(norm, "units") == 0 || strcmp(norm, "chapter") == 0) {
            char um[MAX_RESPONSE_LEN];
            unit_menu(session->current_topic, um);
            strncpy(response_out, um, MAX_RESPONSE_LEN-1);
            return;
        }

        /* Accept 0-5 */
        int chosen_unit = -1;
        if (norm[0] >= '0' && norm[0] <= '5' && (norm[1] == '\0' || norm[1] == ' '))
            chosen_unit = norm[0] - '0';
        /* Also accept "unit 3", "unit3" */
        if (chosen_unit < 0 && strstr(norm, "unit")) {
            const char *p = strstr(norm, "unit"); p += 4;
            while (*p == ' ') p++;
            if (*p >= '0' && *p <= '5') chosen_unit = *p - '0';
        }
        /* Also accept "all" */
        if (chosen_unit < 0 && strstr(norm, "all")) chosen_unit = 0;

        if (chosen_unit >= 0 && chosen_unit <= 5) {
            session->current_unit  = chosen_unit;
            session->current_state = STATE_QA_MODE;
            char banner[MAX_RESPONSE_LEN];
            qa_banner(session->current_topic, chosen_unit, banner);
            if (chosen_unit == 0)
                snprintf(response_out, MAX_RESPONSE_LEN,
                    "\n  Studying all units.%s", banner);
            else
                snprintf(response_out, MAX_RESPONSE_LEN,
                    "\n  Unit %d: %s selected.%s",
                    chosen_unit, get_unit_name(session->current_topic, chosen_unit), banner);
        } else {
            char um[MAX_RESPONSE_LEN];
            unit_menu(session->current_topic, um);
            snprintf(response_out, MAX_RESPONSE_LEN,
                "  [!] Please enter a number 0-5.\n%s", um);
        }
        return;
    }

    /* ==============================================================
     * STATE_QUIZ_MODE
     * ============================================================== */
    if (session->current_state == STATE_QUIZ_MODE) {
        if (contains_keyword(norm, MENU_KW)) {
            session->quiz_question_index = 0; session->quiz_total = 0;
            session->quiz_awaiting_continue = 0;
            session->current_state = STATE_TOPIC_SELECT;
            session->current_topic = TOPIC_NONE;
            session->current_unit  = 0;
            char tp[MAX_RESPONSE_LEN]; dm_get_topic_prompt(tp);
            snprintf(response_out, MAX_RESPONSE_LEN,
                "\n  Quiz aborted. Returning to subject selection.\n%s", tp);
            return;
        }
        dm_run_quiz_step(session, raw_input, response_out);
        return;
    }

    /* ==============================================================
     * STATE_QA_MODE
     * ============================================================== */
    if (session->current_state == STATE_QA_MODE) {

        /* menu */
        if (contains_keyword(norm, MENU_KW)) {
            session->current_state = STATE_TOPIC_SELECT;
            session->current_topic = TOPIC_NONE;
            session->current_unit  = 0;
            char tp[MAX_RESPONSE_LEN]; dm_get_topic_prompt(tp);
            snprintf(response_out, MAX_RESPONSE_LEN,
                "\n  Returning to subject selection.\n%s", tp);
            return;
        }

        /* units/chapters — re-open unit selection (exact commands only) */
        if (strcmp(norm, "units") == 0 || strcmp(norm, "chapters") == 0 ||
            strcmp(norm, "change unit") == 0 || strcmp(norm, "unit") == 0) {
            if (unit_has_chapters(session->current_topic)) {
                session->current_state = STATE_UNIT_SELECT;
                char um[MAX_RESPONSE_LEN];
                unit_menu(session->current_topic, um);
                strncpy(response_out, um, MAX_RESPONSE_LEN-1);
            } else {
                snprintf(response_out, MAX_RESPONSE_LEN,
                    "  This subject does not have separate units. Ask any question!");
            }
            return;
        }

        /* help — shows all topics available in current unit */
        if (is_help_command(norm)) {
            list_topics(session->current_topic, session->current_unit, response_out);
            return;
        }

        /* quiz */
        if (contains_keyword(norm, QUIZ_KW)) {
            session->current_state          = STATE_QUIZ_MODE;
            session->quiz_question_index    = 0;
            session->quiz_total             = 0;
            session->quiz_score             = 0;
            session->quiz_awaiting_continue = 0;
            dm_run_quiz_step(session, raw_input, response_out);
            return;
        }

        /* knowledge base query */
        char ans[MAX_RESPONSE_LEN];
        find_best_match(norm, session->current_topic, session->current_unit, ans);
        snprintf(response_out, MAX_RESPONSE_LEN,
            "\n%s\n\n"
            "  [Ask another question | type 'quiz' | 'units' | 'menu' | 'help']", ans);
        return;
    }

    snprintf(response_out, MAX_RESPONSE_LEN,
        "  [!] Unknown state. Type 'exit' to quit.");
}
