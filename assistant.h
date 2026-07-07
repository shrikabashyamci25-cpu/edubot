#ifndef ASSISTANT_H
#define ASSISTANT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <time.h>

#define MAX_INPUT_LEN        512
#define MAX_RESPONSE_LEN     2048
#define MAX_TOKEN_LEN        64
#define MAX_TOKENS           128
#define LOG_FILENAME         "session_log.txt"
#define MAX_KB_ENTRIES       800
#define DATASET_FILE         "edubot_data.txt"
#define NUM_QUIZ_OPTIONS     4
#define SIMILARITY_THRESHOLD 0.10f

typedef enum {
    STATE_GREETING = 0,
    STATE_TOPIC_SELECT,
    STATE_UNIT_SELECT,
    STATE_QA_MODE,
    STATE_QUIZ_MODE,
    STATE_EXIT
} DialogueState;

typedef enum {
    TOPIC_NONE = 0,
    TOPIC_PHYSICS,
    TOPIC_CIVIL_ENGINEERING,
    TOPIC_MATHS,
    TOPIC_AI,
    TOPIC_C_PROGRAMMING,
    TOPIC_CHEMISTRY,
    TOPIC_ELECTRONICS
} SubjectTopic;

typedef struct {
    char tokens[MAX_TOKENS][MAX_TOKEN_LEN];
    int  count;
} TokenList;

typedef struct {
    char         question[MAX_INPUT_LEN];
    char         answer[MAX_RESPONSE_LEN];
    char         keywords[MAX_INPUT_LEN];
    SubjectTopic topic;
    int          unit;   /* 0 = unspecified / all, 1-5 = specific unit */
} KnowledgeItem;

typedef struct {
    char         question[MAX_INPUT_LEN];
    char         options[NUM_QUIZ_OPTIONS][MAX_RESPONSE_LEN];
    int          correct_option;
    char         explanation[MAX_RESPONSE_LEN];
    SubjectTopic topic;
    int          unit;   /* 0 = all units, 1-5 = specific unit */
} QuizQuestion;

typedef struct {
    char          user_name[64];
    DialogueState current_state;
    SubjectTopic  current_topic;
    int           current_unit;  /* 0 = all units, 1-5 = specific unit */
    int           quiz_question_index;
    int           quiz_score;
    int           quiz_total;
    char          session_start[32];
} SessionState;

/* nlp_engine.c */
void  normalize_string(char *str);
void  tokenize(const char *str, TokenList *tl);
void  remove_stopwords(TokenList *tl);
float compute_similarity(const char *input, const char *target_keywords);

/* knowledge_base.c */
void load_knowledge_base(const char *filename);
int  find_best_match(const char *processed_input, SubjectTopic topic, int unit, char *response_out);
void list_topics(SubjectTopic topic, int unit, char *response_out);
int  get_quiz_question(SubjectTopic topic, int unit, int index, QuizQuestion *q_out);
int  get_quiz_count(SubjectTopic topic, int unit);

/* dialogue_manager.c */
void dm_init_session(SessionState *session);
void dm_get_greeting(char *response_out);
void dm_get_topic_prompt(char *response_out);
void dm_process_input(SessionState *session, const char *raw_input, char *response_out);
void dm_run_quiz_step(SessionState *session, const char *raw_input, char *response_out);

/* simulation.c */
void run_simulation(void);

/* main.c */
char *si_safe_input(char *buf, int size, FILE *stream);
void  si_log_entry(const char *speaker, const char *text, FILE *log_fp);
void  si_flush_log(SessionState *session, FILE *log_fp);

#endif /* ASSISTANT_H */
