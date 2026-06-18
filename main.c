#include "assistant.h"

char *si_safe_input(char *buf, int size, FILE *stream)
{
    if (!buf||size<=0||!stream) return NULL;
    buf[0]='\0';
    if (fgets(buf,size,stream)==NULL) return NULL;
    int len=(int)strlen(buf);
    while (len>0&&(buf[len-1]=='\n'||buf[len-1]=='\r')) buf[--len]='\0';
    if (len==size-1) { int c; while ((c=fgetc(stream))!='\n'&&c!=EOF); }
    return buf;
}

void si_log_entry(const char *speaker, const char *text, FILE *log_fp) {
    if (!speaker||!text||!log_fp) return;
    fprintf(log_fp,"[%s]: %s\n",speaker,text); fflush(log_fp);
}

void si_flush_log(SessionState *session, FILE *log_fp)
{
    if (!session||!log_fp) return;
    time_t now=time(NULL); struct tm *t=localtime(&now); char et[32];
    strftime(et,sizeof(et),"%Y-%m-%d %H:%M:%S",t);
    const char *topic=
        session->current_topic==TOPIC_PHYSICS          ?"Physics":
        session->current_topic==TOPIC_CIVIL_ENGINEERING?"Civil Engineering":
        session->current_topic==TOPIC_MATHS            ?"Mathematics":
        session->current_topic==TOPIC_AI               ?"Artificial Intelligence":
        session->current_topic==TOPIC_C_PROGRAMMING    ?"C Programming":
        session->current_topic==TOPIC_CHEMISTRY        ?"Chemistry":
        session->current_topic==TOPIC_ELECTRONICS      ?"Electronics":"None";
    fprintf(log_fp,
        "\n============================================================\n"
        "  SESSION SUMMARY\n"
        "============================================================\n"
        "  User   : %s\n  Start  : %s\n  End    : %s\n"
        "  Subject: %s\n  Status : %s\n"
        "============================================================\n\n",
        session->user_name[0]?session->user_name:"Unknown",
        session->session_start,et,topic,
        session->current_state==STATE_EXIT?"Clean exit":"Interrupted");
    fflush(log_fp);
}

int main(void)
{
    SessionState session;
    dm_init_session(&session);

    /* Load runtime knowledge base from dataset file */
    load_knowledge_base(DATASET_FILE);

    FILE *log_fp=fopen(LOG_FILENAME,"a");
    if (!log_fp) fprintf(stderr,"[WARNING] Cannot open log file. Continuing without logging.\n\n");
    else { fprintf(log_fp,"\n=== NEW SESSION ===\n"); fflush(log_fp); }

    char response[MAX_RESPONSE_LEN];
    char user_input[MAX_INPUT_LEN];

    dm_get_greeting(response);
    printf("\n%s\n",response);
    printf("------------------------------------------------------------\n");
    if (log_fp) si_log_entry("EduBot",response,log_fp);

    while (session.current_state!=STATE_EXIT) {
        printf(session.current_state==STATE_QUIZ_MODE?"  Answer> ":"  You> ");
        fflush(stdout);

        if (si_safe_input(user_input,MAX_INPUT_LEN,stdin)==NULL) {
            printf("\n  [EOF] Ending session.\n"); break;
        }

        char chk[MAX_INPUT_LEN];
        strncpy(chk,user_input,MAX_INPUT_LEN-1); chk[MAX_INPUT_LEN-1]='\0';
        normalize_string(chk);
        if (strlen(chk)==0) continue;

        if (log_fp) si_log_entry(session.user_name[0]?session.user_name:"User",user_input,log_fp);

        response[0]='\0';
        dm_process_input(&session,user_input,response);

        printf("\n  EduBot:\n%s\n\n",response);
        if (log_fp) si_log_entry("EduBot",response,log_fp);
        printf("------------------------------------------------------------\n");
    }

    if (log_fp) { si_flush_log(&session,log_fp); fclose(log_fp); }
    printf("\n  Session saved to '%s'. Goodbye!\n\n",LOG_FILENAME);
    return 0;
}
