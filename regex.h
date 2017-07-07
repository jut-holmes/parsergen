
#ifndef __PGEN_REGEX_H_
#define __PGEN_REGEX_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
	unsigned short table[1][256];
} dfa;

typedef struct regex
{
	dfa*			dfa;
	unsigned short*	accept;
} regex;

/* create/delete regex */
int regex_load(regex* r, const char* state, const char* filename);
int regex_create(regex* r, const char* state, void* bytes, int len);
void regex_close(regex* r);

#ifdef __cplusplus
}
#endif

#endif