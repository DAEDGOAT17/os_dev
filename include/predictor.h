#ifndef PREDICTOR_H
#define PREDICTOR_H

void predictor_learn(const char* cmd);
const char* predictor_suggest(const char* input);
int predictor_get_matches(const char* input,const char** results,int max_results);
int predictor_longest_common_prefix(const char* input,char* output);

#endif