/*
 * simulation.c
 *
 * Automated evaluation harness for EduBot.
 * Runs 20 predefined queries through the pointer-based knowledge base
 * and prints a structured performance report.
 *
 * Architecture note:
 *   Each SimQuery holds a raw question string and metadata. The runner
 *   passes every query through normalize_string() + find_best_match()
 *   — the same pointer-traversal path the live chatbot uses — then
 *   records per-query metrics in a SimResult array.  A final pass over
 *   that array produces the aggregate report.
 */

#include "assistant.h"

/* ------------------------------------------------------------------ */
/*  Data types                                                          */
/* ------------------------------------------------------------------ */

typedef struct {
    const char  *query;       /* raw question text                    */
    SubjectTopic topic;       /* expected subject domain              */
    int          unit;        /* expected unit (0 = any)              */
    int          is_known;    /* 1 = should map to a KB node, 0 = OOS */
} SimQuery;

typedef struct {
    const SimQuery *q;
    int   matched;            /* 1 = KB pointer resolved, 0 = fallback */
    float confidence;         /* 0.0 – 1.0                            */
    int   gap_exposed;        /* 1 = known query but no match found   */
} SimResult;

/* ------------------------------------------------------------------ */
/*  Test suite — 20 queries                                            */
/*  Mix: 14 known (across all 7 subjects) + 6 out-of-scope            */
/* ------------------------------------------------------------------ */

static const SimQuery TEST_SUITE[] = {
    /* --- C Programming --- */
    { "what is an array",                   TOPIC_C_PROGRAMMING, 3, 1 },
    { "for loop syntax",                    TOPIC_C_PROGRAMMING, 2, 1 },
    { "malloc calloc difference",           TOPIC_C_PROGRAMMING, 5, 1 },
    { "pointer definition address memory",  TOPIC_C_PROGRAMMING, 4, 1 },
    { "recursion factorial fibonacci",      TOPIC_C_PROGRAMMING, 4, 1 },

    /* --- Physics --- */
    { "laser stimulated emission",          TOPIC_PHYSICS,        4, 1 },
    { "superconductivity zero resistance",  TOPIC_PHYSICS,        3, 1 },
    { "de Broglie matter waves",            TOPIC_PHYSICS,        1, 1 },

    /* --- Mathematics --- */
    { "gradient del operator scalar field", TOPIC_MATHS,          2, 1 },
    { "stokes theorem curl surface",        TOPIC_MATHS,          3, 1 },

    /* --- Civil Engineering --- */
    { "types of beams simply supported",    TOPIC_CIVIL_ENGINEERING, 0, 1 },

    /* --- AI --- */
    { "transformer attention mechanism",    TOPIC_AI,             0, 1 },

    /* --- Chemistry --- */
    { "hybridization sp sp2 sp3",           TOPIC_CHEMISTRY,      0, 1 },

    /* --- Electronics --- */
    { "PWM pulse width modulation",         TOPIC_ELECTRONICS,    0, 1 },

    /* --- Out-of-scope / unknown queries --- */
    { "what is the color of the sky",       TOPIC_C_PROGRAMMING,  0, 0 },
    { "why is physics so interesting",      TOPIC_PHYSICS,         0, 0 },
    { "recommend me a good restaurant",     TOPIC_MATHS,           0, 0 },
    { "who won the cricket match yesterday",TOPIC_AI,              0, 0 },
    { "translate this to French bonjour",   TOPIC_CHEMISTRY,       0, 0 },
    { "what is the meaning of life",        TOPIC_ELECTRONICS,     0, 0 },
};

#define SIM_COUNT ((int)(sizeof(TEST_SUITE) / sizeof(TEST_SUITE[0])))

/* ------------------------------------------------------------------ */
/*  Helpers                                                             */
/* ------------------------------------------------------------------ */

/* Returns a display-friendly subject name */
static const char *topic_label(SubjectTopic t)
{
    switch (t) {
        case TOPIC_PHYSICS:           return "Physics";
        case TOPIC_CIVIL_ENGINEERING: return "Civil Engineering";
        case TOPIC_MATHS:             return "Mathematics";
        case TOPIC_AI:                return "AI";
        case TOPIC_C_PROGRAMMING:     return "C Programming";
        case TOPIC_CHEMISTRY:         return "Chemistry";
        case TOPIC_ELECTRONICS:       return "Electronics";
        default:                      return "Unknown";
    }
}

/*
 * Compute a confidence score for a single query result.
 *
 * Scoring model (mirrors the pointer-path depth concept):
 *   - Known query + direct KB hit  → 0.90 – 1.00 (unit-exact = 1.00, unit=0 = 0.90)
 *   - Known query + fallback       → 0.00  (gap exposed)
 *   - Unknown query + fallback     → 0.00  (expected: correct rejection)
 *   - Unknown query + hit          → 0.30  (partial / coincidental match)
 */
static float compute_confidence(const SimQuery *q, int matched)
{
    if (q->is_known && matched)
        return (q->unit > 0) ? 1.00f : 0.90f;   /* exact-unit vs any-unit hit */
    if (q->is_known && !matched)
        return 0.00f;                             /* gap: expected answer missing */
    if (!q->is_known && !matched)
        return 0.00f;                             /* correct: OOS rejected cleanly */
    /* !is_known && matched — unexpected partial hit */
    return 0.30f;
}

/* ------------------------------------------------------------------ */
/*  Main simulation entry point                                         */
/* ------------------------------------------------------------------ */

void run_simulation(void)
{
    SimResult results[SIM_COUNT];
    char      response[MAX_RESPONSE_LEN];
    char      norm[MAX_INPUT_LEN];
    int       i;

    /* --- counters --- */
    int   total_matched   = 0;
    int   total_fallback  = 0;
    int   total_gaps      = 0;
    float total_conf      = 0.0f;

    printf("\n");
    printf("============================================================\n");
    printf("  EDUBOT — AUTOMATED SIMULATION EVALUATION\n");
    printf("  Pointer-Based Knowledge Retrieval Benchmark\n");
    printf("  Total queries: %d\n", SIM_COUNT);
    printf("============================================================\n\n");

    /* ---- per-query run ---- */
    for (i = 0; i < SIM_COUNT; i++) {
        const SimQuery *q = &TEST_SUITE[i];
        SimResult      *r = &results[i];
        r->q = q;

        /* Normalise exactly as the live chatbot does */
        strncpy(norm, q->query, MAX_INPUT_LEN - 1);
        norm[MAX_INPUT_LEN - 1] = '\0';
        normalize_string(norm);

        /* Traverse the KB pointer network */
        response[0] = '\0';
        r->matched = find_best_match(norm, q->topic, q->unit, response);

        r->confidence  = compute_confidence(q, r->matched);
        r->gap_exposed = (q->is_known && !r->matched) ? 1 : 0;

        if (r->matched)   total_matched++;
        else              total_fallback++;
        if (r->gap_exposed) total_gaps++;
        total_conf += r->confidence;

        /* Per-query output */
        printf("  [Q%02d] %-42s | %-16s U%d\n",
               i + 1, q->query, topic_label(q->topic), q->unit);
        printf("        Status     : %s\n",
               r->matched ? "MATCHED  (pointer resolved)" : "FALLBACK (no KB node found)");
        printf("        Query type : %s\n",
               q->is_known ? "Known / in-scope" : "Out-of-scope");
        printf("        Confidence : %.0f%%\n", r->confidence * 100.0f);
        if (r->gap_exposed)
            printf("        *** KB GAP DETECTED — expected answer missing ***\n");
        printf("\n");
    }

    /* ---- aggregate report ---- */
    float avg_conf = total_conf / (float)SIM_COUNT;

    printf("============================================================\n");
    printf("  FINAL EVALUATION REPORT\n");
    printf("============================================================\n");
    printf("  Total Queries Processed   : %d\n",  SIM_COUNT);
    printf("  Successful Matches        : %d  (%.1f%%)\n",
           total_matched,  (total_matched  * 100.0f) / SIM_COUNT);
    printf("  Fallbacks Triggered       : %d  (%.1f%%)\n",
           total_fallback, (total_fallback * 100.0f) / SIM_COUNT);
    printf("  KB Gaps Exposed           : %d\n",  total_gaps);
    printf("  Avg Confidence Score      : %.1f%%\n", avg_conf * 100.0f);
    printf("------------------------------------------------------------\n");

    /* Gap detail */
    if (total_gaps > 0) {
        printf("  GAP DETAIL — queries that found no KB pointer:\n");
        for (i = 0; i < SIM_COUNT; i++) {
            if (results[i].gap_exposed)
                printf("    - [Q%02d] \"%s\" (%s U%d)\n",
                       i + 1,
                       results[i].q->query,
                       topic_label(results[i].q->topic),
                       results[i].q->unit);
        }
        printf("\n");
    } else {
        printf("  No KB gaps detected — all known queries resolved.\n\n");
    }

    /* Performance band */
    printf("  Performance Band : ");
    if      (avg_conf >= 0.85f) printf("EXCELLENT  (>= 85%%)\n");
    else if (avg_conf >= 0.70f) printf("GOOD       (70 – 84%%)\n");
    else if (avg_conf >= 0.50f) printf("AVERAGE    (50 – 69%%)\n");
    else                        printf("NEEDS WORK (< 50%%)\n");

    printf("============================================================\n\n");
}
