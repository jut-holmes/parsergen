
#if 0

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include "regex.h"

static unsigned char* regex_expand(unsigned char* in, long insize, unsigned int* output_size)
{
	unsigned char left[256], right[256], stack[30];
	short int c, count, i, size;
	unsigned char* output;
	long input_offset = 9; /* skip output size header and signature */
	long output_offset = 0;
	int* decomp_size = (int*)(in+5); /* skip signature */

	/* safety checking */
	if (insize < 1) return 0;
	if (*decomp_size < 0 || *decomp_size < insize || *decomp_size > (1024*1024*2)) return 0;
	if (*(unsigned int*)in != '1cgp') return 0;

	/* set output size */
	if (output_size) *output_size = *decomp_size;
	output = (unsigned char*)malloc(*decomp_size);

	/* Unpack each block until end of file */
	while (input_offset < insize) {
		count = in[input_offset++];

		/* Set left to itself as literal flag */
		for (i = 0; i < 256; i++) left[i] = (unsigned char)i;

		/* Read pair table */
		for (c = 0;;) {

			/* Skip range of literal bytes */
			if (count > 127) {
				c += count - 127;
				count = 0;
			}
			if (c == 256) break;

			/* Read pairs, skip right if literal */
			for (i = 0; i <= count; i++, c++) {
				left[c] = in[input_offset++];
				if (c != left[c])
					right[c] = in[input_offset++];
			}
			if (c == 256) break;
			count = in[input_offset++];
		}

		/* Calculate packed data block size */
		size = 256 * in[input_offset] + in[input_offset+1];
		input_offset += 2;

		/* Unpack data block */
		for (i = 0;;) {

			/* Pop byte from stack or read byte */
			if (i)
				c = stack[--i];     
			else {
				if (!size--) break;
				c = in[input_offset++];
			}

			/* Output byte or push pair on stack */
			if (c == left[c])      
				output[output_offset++] = (unsigned char)c;
			else {
				stack[i++] = right[c];
				stack[i++] = left[c];
			}
		}
	}
	return output;
}

#define TESTREAD(_val, _size) 	{if (size<_size) {if (ptr && compressed) free(ptr); return -1;} memcpy(_val, ptr, _size); ptr+= _size;}
#define TESTALLOC(cond) if (!(cond)) {if (ptr && compressed) free(ptr); return -1;}

int regex_load(regex* r, const char* state, const char* filename)
{
	int nres;
	size_t size;
	char* buf;
	FILE* fin;
	struct stat st;

	fin = fopen(filename, "rb");
	if (!fin) return -1;
	if (fstat(fin->_file, &st)) {
		fclose(fin);
		return -1;
	}
	buf = (char*)malloc(st.st_size);
	size = fread(buf, 1, st.st_size, fin);
	fclose(fin);
	nres = regex_create(r, state, buf, (int)size);
	free(buf);
	return nres;
}

static unsigned int regex_find_state(const char* state, unsigned char* ptr, unsigned int* size)
{
	unsigned int	i;
	unsigned short	sz;
	unsigned int	nstates, noffset;

	memcpy(&nstates, ptr, 4); ptr += 4;
	for (i=0; i<nstates; i++)
	{
		memcpy(&sz, ptr, 2); ptr += 2;
		if (!strncmp(ptr, state, sz)) {
			memcpy(&noffset, ptr+sz, 4);
			if (i == nstates-1) {
				*size = 0;
				return noffset;
			}
			/* read next state's offset */
			ptr += sz+4;
			memcpy(&sz, ptr, 2); ptr += sz+2;
			memcpy(size, ptr, 4);
			size -= noffset;
			return noffset;
		}
		ptr += sz+4;
	}
	return (unsigned int)-1;
}

/* create regex */
int regex_create(regex* r, const char* state, void* bytes, int len)
{
	unsigned short	ndfa;
	int				compressed = 0;
	int				i, size = 0;
	unsigned short	*sptr, *dptr;
	unsigned int	ndfa_offset;
	unsigned int	nstate_list_offset, nstate_offset, nstate_size;
	unsigned char*	ptr = 0, *base_ptr;

	if (*(unsigned int*)bytes == '1ngp')  {
		/* uncompressed */
		ptr = (unsigned char*)bytes+5;
		size = len;
	} else
	if (*(unsigned int*)bytes == '1cgp') {
		/* compressed */
		ptr = regex_expand((unsigned char*)bytes, len, &size);
		compressed = 1;
	} else
		/* error */
		return -1;

	/* save base offset */
	base_ptr = ptr;

	/* get dfa and state list offsets */
	TESTREAD(&ndfa_offset, 4);
	ptr += 8;
	TESTREAD(&nstate_list_offset, 4);

	/* find state */
	nstate_offset = regex_find_state(state, base_ptr+nstate_list_offset, &nstate_size);
	
	/* read dfa size */
	TESTREAD(&ndfa, 2);

	if (nstate_size == 0)
		nstate_size = ndfa - nstate_offset;

	/* read dfa for this state */
	TESTALLOC(r->dfa = (dfa*)malloc(nstate_size*512));
	dptr = (unsigned short*)r->dfa;
	sptr = (unsigned short*)(base_ptr+ndfa_offset+2+(512*nstate_offset));
	for (i=0; i<(nstate_size*256); i++, dptr++, sptr++)
		*dptr = (*sptr) ? *sptr - nstate_offset : 0;

	/* read accept table */
	TESTALLOC(r->accept = (unsigned short*)malloc(nstate_size*2));
	memcpy(r->accept, (base_ptr+ndfa_offset+2+(512*ndfa)+(2*nstate_offset)), (nstate_size * 2));

	/* free the buffer */
	if (compressed)
		free(base_ptr);

	return 0;
}

/* delete regex */
void regex_close(regex* r)
{
	free(r->dfa);
	free(r->accept);
	r->dfa = 0;
	r->accept = 0;
}

#define MF_SHORTEST 1

typedef struct rmatch
{
	char*	orig_ptr;

	char*	match_ptr;
	int		match_ofs;
	int		match_len;

	regex*	rgx;
	struct rmatch* next;
} rmatch;

rmatch* regex_match(regex* r, char* str, unsigned int flags)
{
	int		nstate, naccept;
	char	*startptr = str, *tmpptr;
	rmatch	*cur, *head=0, *tail=0;
	while(*str) {
		nstate = 0;

		/* skip non-matches */
		while (*str && !r->dfa->table[nstate][*str]) str++;
		if (!*str)
			break;

		/* save current offset */
		tmpptr = str;

		/* get match */
		if (flags & MF_SHORTEST) {
			while((*str && r->dfa->table[nstate][*str]) && (r->accept[nstate] == 0xffff)) {
				nstate = r->dfa->table[nstate][*str];
				str++;
			}
			if (r->accept[nstate] == -1) {
				/* no match, continue on */
				str = tmpptr+1;
				continue;
			}
		} else {
			naccept=-1;
			while(1) {
				if (r->accept[nstate] != 0xffff)
					naccept=str-startptr;
				if (!r->dfa->table[nstate][*str])
					break;
				nstate = r->dfa->table[nstate][*str];
				if (!*str)
					break;
				str++;
			}
			if (naccept == -1) {
				/* no match, continue on */
				str = tmpptr+1;
				continue;
			}

			/* rewind str if necessary */
			str = startptr+naccept;
		}

		/* create match record */
		cur = (rmatch*)malloc(sizeof(rmatch));
		cur->match_len = (str-tmpptr);
		cur->match_ofs = tmpptr-startptr;
		cur->match_ptr = tmpptr;
		cur->next = 0;
		cur->orig_ptr = startptr;
		cur->rgx = r;

		/* add to match list */
		if (!head) head=tail=cur;
		else {tail->next=cur;tail=cur;}
	}
	return head;
}

void test_regex()
{
	rmatch* m;
	regex r;
	int nresult;

	nresult = regex_load(&r, "tag", "rtg.dat");
	if (nresult < 0) {
		printf("error loading regex file\n");
		return;
	}

	m = regex_match(&r, "<asdfg>  <dsdfgr>", 0);

	if (m) {
		printf("match found: offset %d length %d\n", m->match_ofs, m->match_len);
	} else {
		printf("no match found\n");
	}
}

#endif