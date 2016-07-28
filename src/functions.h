#ifndef FUNCTIONS_H_INCLUDED
#define FUNCTIONS_H_INCLUDED

float floor_f(float, int);
double floor_d(double, int);
int stricmp(const char*, const char*);
char* strchr_replace(char*, char, char);

int file_exist(const char*);
int file_close(FILE**);
int file_size(FILE* f);
int fname_correct(char* fname);

int random(int, int);
int random(int);

double rand_norm();

double Noise2D(int, int);
double NoiseInt(int);
double AWGN_generator();

char getchl();
char getchYN();
char getchYN(bool);

void pause();
void pause(bool);
void pause(bool, bool);


#endif // FUNCTIONS_H_INCLUDED
