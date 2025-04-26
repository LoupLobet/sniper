#include <stdlib.h>
#include <string.h>

#include "dat.h"
#include "utf/utf.h"
#include "util.h"

void
bufprint(Buffer *buf)
{
	char *c;
	int gap;

	gap = 0;
	c = buf->bob;
	for (c = buf->bob; c <= buf->eob; c++) {
		if (c == buf->bog)
			gap = 1;
		if (gap)
			putchar('_');
		else
			putchar(*c);
		if (c == buf->eog)
			gap = 0;
	}
}

Buffer *
bufcreate(unsigned int size)
{
	Buffer *buf;
	
	if ((buf = malloc(sizeof(Buffer))) == NULL)
		return NULL;
	if ((buf->bob = malloc(size)) == NULL) {
		free(buf);
		return NULL;
	}
	buf->len = 0;
	buf->size = size;
	buf->gapsize = buf->size;
	buf->eob = buf->bob + buf->size - 1;
	buf->bog = buf->bob;
	buf->eog = buf->eob;
	return buf;
}

int
bufmovebackwards(Buffer *buf, int n)
{
	int moved;

	moved = 0;
	while (moved != n) {
		if (buf->bog == buf->bob)
			return moved;
		*buf->eog = *(buf->bog - 1);
		buf->bog--;
		buf->eog--;
		moved++;
	}
	return moved;
}

int
bufmovecolumn(Buffer *buf, int n)
{
	int current;

	current = buf->bog - buf->bob;
	printf("moving cursor from column %d to %d\n", current, n);
	printf("buffer len %lld\n", buf->len);
	if (buf->len < n || n < 0)
		return current;
	if (current > n)
		for (int i = 0; i < current - n; i++) {
			printf("moving forwards\n");
			bufmoverunebackwards(buf, 1);
		}
	else if (current <= n)
		for (int i = 0; i < n - current; i++) {
			printf("moving backward\n");
			bufmoveruneforward(buf, 1);
		}
	return n;
}

int
bufmoveforward(Buffer *buf, int n)
{
	int moved;

	moved = 0;
	while (moved != n) {
		if (buf->eog == buf->eob)
			return moved;
		*buf->bog = *(buf->eog + 1);
		buf->bog++;
		buf->eog++;
		moved++;
	}
	return moved;
}

int
bufmoverunebackwards(Buffer *buf, int n)
{
	Rune r;
	unsigned int moved;
	int i, runelen;

	moved = 0;
	while (moved != n) {
		if (buf->bog == buf->bob)
			return moved;
		/* find the last rune before the gap */
		for (i = 0; i < 4; i++) {
			if (buf->bog - i == buf->bob)
				return moved;
			runelen = chartorune(&r, buf->bog - i - 1);
			if (r == Runeerror)
				continue;
			memcpy(buf->eog - runelen + 1, buf->bog - runelen, runelen);
			buf->bog -= runelen;
			buf->eog -= runelen;
			moved++;
			break;
		}
		if (r == Runeerror) /* invalid utf8 codepoint */
			return moved;
	}
	return moved;
}

int
bufmoveruneforward(Buffer *buf, int n)
{
	Rune r;
	unsigned int moved;
	int runelen;

	moved = 0;
	while (moved != n) {
		if (buf->eog == buf->eob)
			return moved;
		runelen = chartorune(&r, buf->eog + 1);
		if (r == Runeerror)
			return moved;
		memcpy(buf->bog, buf->eog + 1, runelen);
		buf->bog += runelen;
		buf->eog += runelen;
		moved++;
	}
	return moved;
}

int
bufinsert(Buffer *buf, char *s, int n)
{
	int i;

	for (i = 0; i < n; i++) {
		*buf->bog = s[i];
		buf->bog++;
		buf->gapsize--;
		buf->len++;
		if (buf->gapsize <= 1) {
			if (!bufresize(buf))
				return i;
		}
	}
	return i;
}

int
bufdeleteafter(Buffer *buf, int n)
{
	int i;
	for (i = 0; i < n; i++) {
		if (buf->eog == buf->eob)
			return i;
		buf->eog++;
		buf->gapsize++;
		buf->len--;
	}
	return i;
}

int
bufdeleteruneafter(Buffer *buf, int n)
{
	int i, runelen;
	Rune r;

	for (i = 0; i < n; i++) {
		if (buf->eog == buf->eob)
			return i;
		runelen = chartorune(&r, buf->eog + 1);
		if (r == Runeerror)
			return i;
		buf->eog += runelen;
		buf->gapsize += runelen;
		buf->len -= runelen;
	}
	return i;
}

int
bufdeletebefore(Buffer *buf, int n)
{
	int i;
	for (i = 0; i < n; i++) {
		if (buf->bog == buf->bob)
			return i;
		buf->bog--;
		buf->gapsize++;
		buf->len--;
	}
	return i;
}

int
bufdeleterunebefore(Buffer *buf, int n)
{
	int i, j, runelen;
	Rune r;

	for (i = 0; i < n; i++) {
		if (buf->bog == buf->bob)
			return i;
		/* find the last rune before the gap */
		for (j = 0; j < 4; j++) {
			if (buf->bog - j == buf->bob)
				return i;
			runelen = chartorune(&r, buf->bog - 1 - j);
			if (r == Runeerror)
				continue;
			buf->bog -= runelen;
			buf->gapsize += runelen;
			buf->len -= runelen;
			break;
		}
		if (r == Runeerror) /* invalid utf8 codepoint */
			return i;
	}
	return i;
}

void
buffree(Buffer *buf)
{
	free(buf->bob);
	free(buf);
}

int
bufresize(Buffer *buf)
{
	char *new;
	unsigned int leftsize, newsize, rightsize;
	unsigned int gapsize;

	newsize = (buf->size < 1 ? 2 : buf->size * 2);
	leftsize = buf->bog - buf->bob;
	rightsize = buf->eob - buf->eog;
	if ((new = realloc(buf->bob, newsize)) == NULL)
		return 0;
	gapsize = newsize - (buf->size - buf->gapsize);
	if (buf->eog != buf->eob)
		memcpy(new + leftsize + gapsize, new + leftsize + buf->gapsize, rightsize);
	buf->bob = new;
	buf->bog = buf->bob + leftsize;
	buf->eog = buf->bog + gapsize - 1;
	buf->eob = new + newsize - 1;
	buf->size = newsize;
	buf->gapsize = gapsize;
	/* buf->len  = buf->len */
	return newsize;
}

char *
buftostr(Buffer *buf)
{
	char *p, *q, *s;

	if ((s = calloc(buf->len + 1, 1)) == NULL)
		return NULL;
	q = s;
	for (p = buf->bob; p <= buf->eob; p++) {
		if (p == buf->bog) {
			if (buf->eog == buf->eob)
				break;
			p = buf->eog + 1;
		}
		*q = *p;
		q++;
	}
	return s;
}
