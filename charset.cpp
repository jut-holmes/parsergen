
#include "pgen.h"
#include <ctype.h>
#include <stdlib.h>

#define SetBit(set, bit)	(set[(bit >> 3)] |= (1 << (bit % 8)))
#define UnsetBit(set, bit)	(set[(bit >> 3)] &= ~(1 << (bit % 8)))

#ifndef WIN32
char* strlwr(const char* s)
{
	char* sp = strdup(s);
	for (char* c=sp;*c;c++) {
		if (isupper(*c))
			*c=tolower(*c);
	}
	return sp;
}
int stricmp(const char* s1, const char* s2)
{
	int result;
	char* sp1 = strlwr(s1);
	char* sp2 = strlwr(s2);
	result = strcmp(sp1,sp2);
	free(sp1); free(sp2);
	return result;
}
#endif

charset* charset_add(parser* p, const char* name)
{
	charset* plast;
	for (charset* pcs=p->charsets;pcs;pcs=pcs->next)
	{
		if (!_stricmp(pcs->name,name))
			return pcs;
		plast=pcs;
	}
	charset* c = new charset;
	memset(c,0,sizeof(charset));
	c->name = _strdup(name);
	c->mode = charset::CS_ADD;
	if (!p->charsets)
		p->charsets = c;
	else
		plast->next=c;
	return c;
}

charset* charset_find(parser* p, const char* name)
{
	for (charset* pcs=p->charsets;pcs;pcs=pcs->next)
	{
		if (!_stricmp(pcs->name,name))
			return pcs;
	}
	return 0;
}

charset* charset_combine(charset* dst, charset* src, char badd)
{
	if (badd)
	{
		for (int i=0;i<8;i++)
			((unsigned int*)dst->set)[i] |= ((unsigned int*)src->set)[i];
	} else
	{
		for (int i=0;i<8;i++)
			((unsigned int*)dst->set)[i] &= ~((unsigned int*)src->set)[i];
	}
	return dst;
}

charset* charset_set(charset* dst, int ch, char bset)
{
	if (bset)
		SetBit(dst->set, ch);
	else
		UnsetBit(dst->set, ch);
	return dst;
}

charset* charset_set_range(charset* dst, int start, int end, char bset)
{
	int ns = min(start, end);
	int ne = max(start, end);
	ns = max(ns,0);
	ne = min(ne,255);
	if (bset)
	{
		for (int i=ns;i<=ne;i++)
			SetBit(dst->set, i);
	} else
	{
		for (int i=ns;i<=ne;i++)
			UnsetBit(dst->set, i);
	}
	return dst;
}

void charset_build_default(parser* p)
{
	// single character sets
	SetBit(charset_add(p, "{HT}")->set, '\x09');
	SetBit(charset_add(p, "{LF}")->set, '\x0a');
	SetBit(charset_add(p, "{VT}")->set, '\x0b');
	SetBit(charset_add(p, "{FF}")->set, '\x0c');
	SetBit(charset_add(p, "{CR}")->set, '\x0d');
	SetBit(charset_add(p, "{NL}")->set, '\x0a');
	SetBit(charset_add(p, "{Space}")->set, '\x20');
	SetBit(charset_add(p, "{NBSP}")->set, '\xa0');

	// not newline
	charset* notnl = charset_set_range(charset_add(p, "{Notnl}"), 0, 255, TRUE);
	charset_set(notnl, '\n', FALSE);
	charset_set(notnl, '\r', FALSE);

	// digit
	charset* digit = charset_set_range(charset_add(p, "{Digit}"), '0', '9', TRUE);

	// alpha
	charset* alpha = charset_set_range(charset_add(p, "{Alpha}"), 'a', 'z', TRUE);
	charset_set_range(alpha, 'A', 'Z', TRUE);

	// upper
	charset* upper = charset_set_range(charset_add(p, "{Upper}"), 'A', 'Z', TRUE);

	// lower
	charset* lower = charset_set_range(charset_add(p, "{Lower}"), 'a', 'z', TRUE);

	// alphanum
	charset* alphanum = charset_combine(charset_combine(charset_add(p, "{AlphaNum}"), alpha, TRUE), digit, TRUE);

	// printable
	charset* printable = charset_set(charset_set_range(charset_add(p, "{Printable}"), 33, 126, TRUE), '\xa0', TRUE);

	// word
	charset* word = charset_set(charset_add(p, "{Word}"), '_', TRUE);
	charset_combine(word, alpha, TRUE);
	charset_combine(word, digit, TRUE);

	// hexdigit
	charset* hexdigit = charset_set_range(charset_add(p,"{Hexdigit}"),'0','9',TRUE);
	charset_set_range(hexdigit, 'a', 'f', TRUE);
	charset_set_range(hexdigit, 'A', 'F', TRUE);

	// whitespace not including newline
	charset* whitespace = charset_set_range(charset_add(p, "{Whitespace}"), 0x09, 0x0c, TRUE);
	charset_set(whitespace, '\x20', TRUE);
	charset_set(whitespace, '\xa0', TRUE);
	charset_set(whitespace, '\n', FALSE);
	charset_set(whitespace, '\r', FALSE);

	// whitespace including newline
	charset* whitespacenl = charset_set_range(charset_add(p, "{Whitespacenl}"), 0x09, 0x0a, TRUE);
	charset_set(whitespacenl, '\x20', TRUE);
	charset_set(whitespacenl, '\xa0', TRUE);

	// ^Word
	charset* notword = charset_set_range(charset_add(p, "{NotWord}"), 0x00, 0xff, TRUE); 
	charset_combine(notword, word, FALSE);

	// ^Digit
	charset* notdigit = charset_set_range(charset_add(p, "{NotDigit}"), 0x00, 0xff, TRUE); 
	charset_combine(notdigit, digit, FALSE);

	// ^WhitespaceNL
	charset* notwhitespacenl = charset_set_range(charset_add(p, "{NotWhitespacenl}"), 0x00, 0xff, TRUE); 
	charset_combine(notwhitespacenl, whitespacenl, FALSE);

}
