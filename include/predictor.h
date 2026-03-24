#ifndef PREDICTOR_H
#define PREDICTOR_H

typedef struct {
    char intent[32];
    float confidence;
} Prediction;

Prediction predict_intent(const char* input);

#endif