#include "assistant.h"

static const char *STOPWORDS[] = {
    "a","an","the","is","it","in","on","at","to","for","of","and","or","but",
    "not","with","as","by","from","that","this","are","was","be","has","have",
    "had","do","does","did","will","would","can","could","should","may","might",
    "shall","what","which","who","how","when","where","why","i","me","my","we",
    "our","you","your","he","she","his","her","they","their","its","about",
    "tell","explain","define","describe","give","please","mean","means",
    "difference","between","s", NULL
};

void normalize_string(char *str)
{
    if (!str) return;
    int len = (int)strlen(str);
    while (len > 0 && (str[len-1]=='\n'||str[len-1]=='\r'||str[len-1]==' '))
        str[--len] = '\0';

    int i, j = 0;
    for (i = 0; i < len; i++) {
        unsigned char uc = (unsigned char)str[i];
        char c = (char)tolower(uc);
        if (isalnum(uc) || c == ' ') { str[j++] = c; }
        else { if (j > 0 && str[j-1] != ' ') str[j++] = ' '; }
    }
    str[j] = '\0';

    char buf[MAX_INPUT_LEN];
    int out = 0, in_space = 0;
    for (i = 0; str[i] != '\0' && out < MAX_INPUT_LEN-1; i++) {
        if (str[i]==' ') { if (!in_space && out>0) { buf[out++]=' '; in_space=1; } }
        else { buf[out++]=str[i]; in_space=0; }
    }
    if (out > 0 && buf[out-1]==' ') out--;
    buf[out] = '\0';
    strncpy(str, buf, MAX_INPUT_LEN-1);
    str[MAX_INPUT_LEN-1] = '\0';
}

void tokenize(const char *str, TokenList *tl)
{
    if (!str || !tl) return;
    tl->count = 0;
    char buf[MAX_INPUT_LEN];
    strncpy(buf, str, MAX_INPUT_LEN-1); buf[MAX_INPUT_LEN-1]='\0';
    char *tok = strtok(buf, " ");
    while (tok && tl->count < MAX_TOKENS) {
        strncpy(tl->tokens[tl->count], tok, MAX_TOKEN_LEN-1);
        tl->tokens[tl->count][MAX_TOKEN_LEN-1]='\0';
        tl->count++;
        tok = strtok(NULL, " ");
    }
}

static int is_stopword(const char *w) {
    int i; for (i=0; STOPWORDS[i]; i++) if (strcmp(w,STOPWORDS[i])==0) return 1; return 0;
}

void remove_stopwords(TokenList *tl)
{
    if (!tl) return;
    char f[MAX_TOKENS][MAX_TOKEN_LEN]; int nc=0, i;
    for (i=0; i<tl->count; i++)
        if (!is_stopword(tl->tokens[i])) { strncpy(f[nc],tl->tokens[i],MAX_TOKEN_LEN-1); f[nc][MAX_TOKEN_LEN-1]='\0'; nc++; }
    tl->count = nc;
    for (i=0; i<nc; i++) { strncpy(tl->tokens[i],f[i],MAX_TOKEN_LEN-1); tl->tokens[i][MAX_TOKEN_LEN-1]='\0'; }
}

float compute_similarity(const char *input, const char *target_keywords)
{
    if (!input || !target_keywords) return 0.0f;
    char ni[MAX_INPUT_LEN], nt[MAX_INPUT_LEN];
    strncpy(ni, input, MAX_INPUT_LEN-1);           ni[MAX_INPUT_LEN-1]='\0';
    strncpy(nt, target_keywords, MAX_INPUT_LEN-1); nt[MAX_INPUT_LEN-1]='\0';
    normalize_string(ni); normalize_string(nt);

    TokenList tli, tlt;
    tokenize(ni,&tli); tokenize(nt,&tlt);
    remove_stopwords(&tli); remove_stopwords(&tlt);
    if (tli.count==0 || tlt.count==0) return 0.0f;

    float mw = 0.0f; int i, j;
    for (i=0; i<tli.count; i++) {
        const char *iw = tli.tokens[i]; int il = (int)strlen(iw);
        for (j=0; j<tlt.count; j++) {
            const char *tw = tlt.tokens[j]; int tl2 = (int)strlen(tw);
            if (strcmp(iw,tw)==0) { mw+=1.0f; break; }
            int ml = il<tl2?il:tl2;
            if (ml>=3 && strncmp(iw,tw,(size_t)ml)==0) { mw+=0.6f; break; }
        }
    }
    int denom = tli.count>tlt.count?tli.count:tlt.count;
    float s = mw/(float)denom;
    return s>1.0f?1.0f:s;
}
