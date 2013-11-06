/*
 * colorings of characters
 * This file is #included by regcomp.c.
 *
 * Copyright (c) 1998, 1999 Henry Spencer.  All rights reserved.
 * 
 * Development of this software was funded, in part, by Cray Research Inc.,
 * UUNET Communications Services Inc., Sun Microsystems Inc., and Scriptics
 * Corporation, none of whom are responsible for the results.  The author
 * thanks all of them. 
 * 
 * Redistribution and use in source and binary forms -- with or without
 * modification -- are permitted for any purpose, provided that
 * redistributions in source form retain this entire copyright notice and
 * indicate the origin and nature of any modifications.
 * 
 * I'd appreciate being given credit for this package in the documentation
 * of software which uses it, but that is not a requirement.
 * 
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL
 * HENRY SPENCER BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *
 *
 * Note that there are some incestuous relationships between this code and
 * NFA arc maintenance, which perhaps ought to be cleaned up sometime.
 */



#define	CISERR()	VISERR(cm->v)
#define	CERR(e)		(void)VERR(cm->v, (e))



/*
 - initcm - set up new colormap
 ^ static VOID initcm(struct vars *, struct colormap *);
 */
static VOID
initcm(v, cm)
struct vars *v;
struct colormap *cm;
{
	int i;
	int j;
	union tree *t;
	union tree *nextt;
	struct colordesc *cd;

	cm->magic = CMMAGIC;
	cm->v = v;

	cm->ncds = NINLINECDS;
	cm->cd = cm->cdspace;
	cm->max = 0;
	cm->free = 0;

	cd = cm->cd;			/* cm->cd[WHITE] */
	cd->sub = NOSUB;
	cd->arcs = NULL;
	cd->flags = 0;
	cd->nchrs = CHR_MAX - CHR_MIN + 1;

	/* upper levels of tree */
	for (t = &cm->tree[0], j = NBYTS-1; j > 0; t = nextt, j--) {
		nextt = t + 1;
		for (i = BYTTAB-1; i >= 0; i--)
			t->tptr[i] = nextt;
	}
	/* bottom level is solid white */
	t = &cm->tree[NBYTS-1];
	for (i = BYTTAB-1; i >= 0; i--)
		t->tcolor[i] = WHITE;
	cd->block = t;
}

/*
 - freecm - free dynamically-allocated things in a colormap
 ^ static VOID freecm(struct colormap *);
 */
static VOID
freecm(cm)
struct colormap *cm;
{
	size_t i;
	union tree *cb;

	cm->magic = 0;
	if (NBYTS > 1)
		cmtreefree(cm, cm->tree, 0);
	for (i = 1; i <= cm->max; i++)		/* skip WHITE */
		if (!UNUSEDCOLOR(&cm->cd[i])) {
			cb = cm->cd[i].block;
			if (cb != NULL)
				FREE(cb);
		}
	if (cm->cd != cm->cdspace)
		FREE(cm->cd);
}

/*
 - cmtreefree - free a non-terminal part of a colormap tree
 ^ static VOID cmtreefree(struct colormap *, union tree *, int);
 */
static VOID
cmtreefree(cm, tree, level)
struct colormap *cm;
union tree *tree;
int level;			/* level number (top == 0) of this block */
{
	int i;
	union tree *t;
	union tree *fillt = &cm->tree[level+1];
	union tree *cb;

	assert(level < NBYTS-1);	/* this level has pointers */
	for (i = BYTTAB-1; i >= 0; i--) {
		t = tree->tptr[i];
		assert(t != NULL);
		if (t != fillt) {
			if (level < NBYTS-2) {	/* more pointer blocks below */
				cmtreefree(cm, t, level+1);
				FREE(t);
			} else {		/* color block below */
				cb = cm->cd[t->tcolor[0]].block;
				if (t != cb)	/* not a solid block */
					FREE(t);
			}
		}
	}
}

/*
 - setcolor - set the color of a character in a colormap
 ^ static color setcolor(struct colormap *, pchr, pcolor);
 */
static color			/* previous color */
setcolor(cm, c, co)
struct colormap *cm;
pchr c;
pcolor co;
{
	uchr uc = c;
	int shift;
	int level;
	int b;
	int bottom;
	union tree *t;
	union tree *newt;
	union tree *fillt;
	union tree *lastt;
	union tree *cb;
	color prev;

	assert(cm->magic == CMMAGIC);
	if (CISERR() || co == COLORLESS)
		return COLORLESS;

	t = cm->tree;
	for (level = 0, shift = BYTBITS * (NBYTS - 1); shift > 0;
						level++, shift -= BYTBITS) {
		b = (uc >> shift) & BYTMASK;
		lastt = t;
		t = lastt->tptr[b];
		assert(t != NULL);
		fillt = &cm->tree[level+1];
		bottom = (shift <= BYTBITS) ? 1 : 0;
		cb = (bottom) ? cm->cd[t->tcolor[0]].block : fillt;
		if (t == fillt || t == cb) {	/* must allocate a new block */
			newt = (union tree *)MALLOC((bottom) ?
				sizeof(struct colors) : sizeof(struct ptrs));
			if (newt == NULL) {
				CERR(REG_ESPACE);
				return COLORLESS;
			}
			if (bottom)
				memcpy(VS(newt->tcolor), VS(t->tcolor),
							BYTTAB*sizeof(color));
			else
				memcpy(VS(newt->tptr), VS(t->tptr),
						BYTTAB*sizeof(union tree *));
			t = newt;
			lastt->tptr[b] = t;
		}
	}

	b = uc & BYTMASK;
	prev = t->tcolor[b];
	t->tcolor[b] = (color)co;
	return prev;
}

/*
 - maxcolor - report largest color number in use
 ^ static color maxcolor(struct colormap *);
 */
static color
maxcolor(cm)
struct colormap *cm;
{
	if (CISERR())
		return COLORLESS;

	return (color)cm->max;
}

/*
 - newcolor - find a new color (must be subject of setcolor at once)
 * Beware:  may relocate the colordescs.
 ^ static color newcolor(struct colormap *);
 */
static color			/* COLORLESS for error */
newcolor(cm)
struct colormap *cm;
{
	struct colordesc *cd;
	struct colordesc *new;
	size_t n;

	if (CISERR())
		return COLORLESS;

	if (cm->free != 0) {
		assert(cm->free > 0);
		assert((size_t)cm->free < cm->ncds);
		cd = &cm->cd[cm->free];
		assert(UNUSEDCOLOR(cd));
		assert(cd->arcs == NULL);
		cm->free = cd->sub;
	} else if (cm->max < cm->ncds - 1) {
		cm->max++;
		cd = &cm->cd[cm->max];
	} else {
		/* oops, must allocate more */
		n = cm->ncds * 2;
		if (cm->cd == cm->cdspace) {
			new = (struct colordesc *)MALLOC(n *
						sizeof(struct colordesc));
			if (new != NULL)
				memcpy(VS(new), VS(cm->cdspace), cm->ncds *
						sizeof(struct colordesc));
		} else
			new = (struct colordesc *)REALLOC(cm->cd,
						n * sizeof(struct colordesc));
		if (new == NULL) {
			CERR(REG_ESPACE);
			return COLORLESS;
		}
		cm->cd = new;
		cm->ncds = n;
		assert(cm->max < cm->ncds - 1);
		cm->max++;
		cd = &cm->cd[cm->max];
	}

	cd->nchrs = 0;
	cd->sub = NOSUB;
	cd->arcs = NULL;
	cd->flags = 0;
	cd->block = NULL;

	return (color)(cd - cm->cd);
}

/*
 - freecolor - free a color (must have no arcs or subcolor)
 ^ static VOID freecolor(struct colormap *, pcolor);
 */
static VOID
freecolor(cm, co)
struct colormap *cm;
pcolor co;
{
	struct colordesc *cd = &cm->cd[co];
	color pco, nco;			/* for freelist scan */

	assert(co >= 0);
	if (co == WHITE)
		return;

	assert(cd->arcs == NULL);
	assert(cd->sub == NOSUB);
	assert(cd->nchrs == 0);
	cd->flags = FREECOL;
	if (cd->block != NULL) {
		FREE(cd->block);
		cd->block = NULL;	/* just paranoia */
	}

	if ((size_t)co == cm->max) {
		while (cm->max > WHITE && UNUSEDCOLOR(&cm->cd[cm->max]))
			cm->max--;
		assert(cm->free >= 0);
		while ((size_t)cm->free > cm->max)
			cm->free = cm->cd[cm->free].sub;
		if (cm->free > 0) {
			assert(cm->free < cm->max);
			pco = cm->free;
			nco = cm->cd[pco].sub;
			while (nco > 0)
				if ((size_t)nco > cm->max) {
					/* take this one out of freelist */
					nco = cm->cd[nco].sub;
					cm->cd[pco].sub = nco;
				} else {
					assert(nco < cm->max);
					pco = nco;
					nco = cm->cd[pco].sub;
				}
		}
	} else {
		cd->sub = cm->free;
		cm->free = (color)(cd - cm->cd);
	}
}

/*
 - pseudocolor - allocate a false color, to be managed by other means
 ^ static color pseudocolor(struct colormap *);
 */
static color
pseudocolor(cm)
struct colormap *cm;
{
	color co;

	co = newcolor(cm);
	if (CISERR())
		return COLORLESS;
	cm->cd[co].nchrs = 1;
	cm->cd[co].flags = PSEUDO;
	return co;
}

/*
 - subcolor - allocate a new subcolor (if necessary) to this chr
 ^ static color subcolor(struct colormap *, pchr c);
 */
static color
subcolor(cm, c)
struct colormap *cm;
pchr c;
{
	color co;			/* current color of c */
	color sco;			/* new subcolor */

	co = GETCOLOR(cm, c);
	sco = newsub(cm, co);
	if (CISERR())
		return COLORLESS;
	assert(sco != COLORLESS);

	if (co == sco)		/* already in an open subcolor */
		return co;	/* rest is redundant */
	cm->cd[co].nchrs--;
	cm->cd[sco].nchrs++;
	setcolor(cm, c, sco);
	return sco;
}

/*
 - newsub - allocate a new subcolor (if necessary) for a color
 ^ static color newsub(struct colormap *, pcolor);
 */
static color
newsub(cm, co)
struct colormap *cm;
pcolor co;
{
	color sco;			/* new subcolor */

	sco = cm->cd[co].sub;
	if (sco == NOSUB) {		/* color has no open subcolor */
		if (cm->cd[co].nchrs == 1)	/* optimization */
			return co;
		sco = newcolor(cm);	/* must create subcolor */
		if (sco == COLORLESS) {
			assert(CISERR());
			return COLORLESS;
		}
		cm->cd[co].sub = sco;
		cm->cd[sco].sub = sco;	/* open subcolor points to self */
	}
	assert(sco != NOSUB);

	return sco;
}

/*
 - subrange - allocate new subcolors to this range of chrs, fill in arcs
 ^ static VOID subrange(struct vars *, pchr, pchr, struct state *,
 ^ 	struct state *);
 */
static VOID
subrange(v, from, to, lp, rp)
struct vars *v;
pchr from;
pchr to;
struct state *lp;
struct state *rp;
{
	uchr uf;
	int i;

	assert(from <= to);

	/* first, align "from" on a tree-block boundary */
	uf = (uchr)from;
	i = (int)( ((uf + BYTTAB-1) & (uchr)~BYTMASK) - uf );
	for (; from <= to && i > 0; i--, from++)
		newarc(v->nfa, PLAIN, subcolor(v->cm, from), lp, rp);
	if (from > to)			/* didn't reach a boundary */
		return;

	/* deal with whole blocks */
	for (; to - from >= BYTTAB; from += BYTTAB)
		subblock(v, from, lp, rp);

	/* clean up any remaining partial table */
	for (; from <= to; from++)
		newarc(v->nfa, PLAIN, subcolor(v->cm, from), lp, rp);
}

/*
 - subblock - allocate new subcolors for one tree block of chrs, fill in arcs
 ^ static VOID subblock(struct vars *, pchr, struct state *, struct state *);
 */
static VOID
subblock(v, start, lp, rp)
struct vars *v;
pchr start;			/* first of BYTTAB chrs */
struct state *lp;
struct state *rp;
{
	uchr uc = start;
	struct colormap *cm = v->cm;
	int shift;
	int level;
	int i;
	int b = 0;
	union tree *t;
	union tree *cb;
	union tree *fillt;
	union tree *lastt = NULL;
	int previ;
	int ndone;
	color co;
	color sco;

	assert((uc % BYTTAB) == 0);

	/* find its color block, making new pointer blocks as needed */
	t = cm->tree;
	fillt = NULL;
	for (level = 0, shift = BYTBITS * (NBYTS - 1); shift > 0;
						level++, shift -= BYTBITS) {
		b = (uc >> shift) & BYTMASK;
		lastt = t;
		t = lastt->tptr[b];
		assert(t != NULL);
		fillt = &cm->tree[level+1];
		if (t == fillt && shift > BYTBITS) {	/* need new ptr block */
			t = (union tree *)MALLOC(sizeof(struct ptrs));
			if (t == NULL) {
				CERR(REG_ESPACE);
				return;
			}
			memcpy(VS(t->tptr), VS(fillt->tptr),
						BYTTAB*sizeof(union tree *));
			lastt->tptr[b] = t;
		}
	}

	/* special cases:  fill block or solid block */
	co = t->tcolor[0];
	cb = cm->cd[co].block;
	if (t == fillt || t == cb) {
		/* either way, we want a subcolor solid block */
		sco = newsub(cm, co);
		t = cm->cd[sco].block;
		if (t == NULL) {	/* must set it up */
			t = (union tree *)MALLOC(sizeof(struct colors));
			if (t == NULL) {
				CERR(REG_ESPACE);
				return;
			}
			for (i = 0; i < BYTTAB; i++)
				t->tcolor[i] = sco;
			cm->cd[sco].block = t;
		}
		/* find loop must have run at least once */
		lastt->tptr[b] = t;
		newarc(v->nfa, PLAIN, sco, lp, rp);
		cm->cd[co].nchrs -= BYTTAB;
		cm->cd[sco].nchrs += BYTTAB;
		return;
	}

	/* general case, a mixed block to be altered */
	i = 0;
	while (i < BYTTAB) {
		co = t->tcolor[i];
		sco = newsub(cm, co);
		newarc(v->nfa, PLAIN, sco, lp, rp);
		previ = i;
		do {
			t->tcolor[i++] = sco;
		} while (i < BYTTAB && t->tcolor[i] == co);
		ndone = i - previ;
		cm->cd[co].nchrs -= ndone;
		cm->cd[sco].nchrs += ndone;
	}
}

/*
 - okcolors - promote subcolors to full colors
 ^ static VOID okcolors(struct nfa *, struct colormap *);
 */
static VOID
okcolors(nfa, cm)
struct nfa *nfa;
struct colormap *cm;
{
	struct colordesc *cd;
	struct colordesc *end = CDEND(cm);
	struct colordesc *scd;
	struct arc *a;
	color co;
	color sco;

	for (cd = cm->cd, co = 0; cd < end; cd++, co++) {
		sco = cd->sub;
		if (UNUSEDCOLOR(cd) || sco == NOSUB) {
			/* has no subcolor, no further action */
		} else if (sco == co) {
			/* is subcolor, let parent deal with it */
		} else if (cd->nchrs == 0) {
			/* parent empty, its arcs change color to subcolor */
			cd->sub = NOSUB;
			scd = &cm->cd[sco];
			assert(scd->nchrs > 0);
			assert(scd->sub == sco);
			scd->sub = NOSUB;
			while ((a = cd->arcs) != NULL) {
				assert(a->co == co);
				/* uncolorchain(cm, a); */
				cd->arcs = a->colorchain;
				a->co = sco;
				/* colorchain(cm, a); */
				a->colorchain = scd->arcs;
				scd->arcs = a;
			}
			freecolor(cm, co);
		} else {
			/* parent's arcs must gain parallel subcolor arcs */
			cd->sub = NOSUB;
			scd = &cm->cd[sco];
			assert(scd->nchrs > 0);
			assert(scd->sub == sco);
			scd->sub = NOSUB;
			for (a = cd->arcs; a != NULL; a = a->colorchain) {
				assert(a->co == co);
				newarc(nfa, a->type, sco, a->from, a->to);
			}
		}
	}
}

/*
 - colorchain - add this arc to the color chain of its color
 ^ static VOID colorchain(struct colormap *, struct arc *);
 */
static VOID
colorchain(cm, a)
struct colormap *cm;
struct arc *a;
{
	struct colordesc *cd = &cm->cd[a->co];

	a->colorchain = cd->arcs;
	cd->arcs = a;
}

/*
 - uncolorchain - delete this arc from the color chain of its color
 ^ static VOID uncolorchain(struct colormap *, struct arc *);
 */
static VOID
uncolorchain(cm, a)
struct colormap *cm;
struct arc *a;
{
	struct colordesc *cd = &cm->cd[a->co];
	struct arc *aa;

	aa = cd->arcs;
	if (aa == a)		/* easy case */
		cd->arcs = a->colorchain;
	else {
		for (; aa != NULL && aa->colorchain != a; aa = aa->colorchain)
			continue;
		assert(aa != NULL);
		aa->colorchain = a->colorchain;
	}
	a->colorchain = NULL;	/* paranoia */
}

/*
 - singleton - is this character in its own color?
 ^ static int singleton(struct colormap *, pchr c);
 */
static int			/* predicate */
singleton(cm, c)
struct colormap *cm;
pchr c;
{
	color co;			/* color of c */

	co = GETCOLOR(cm, c);
	if (cm->cd[co].nchrs == 1 && cm->cd[co].sub == NOSUB)
		return 1;
	return 0;
}

/*
 - rainbow - add arcs of all full colors (but one) between specified states
 ^ static VOID rainbow(struct nfa *, struct colormap *, int, pcolor,
 ^ 	struct state *, struct state *);
 */
static VOID
rainbow(nfa, cm, type, but, from, to)
struct nfa *nfa;
struct colormap *cm;
int type;
pcolor but;			/* COLORLESS if no exceptions */
struct state *from;
struct state *to;
{
	struct colordesc *cd;
	struct colordesc *end = CDEND(cm);
	color co;

	for (cd = cm->cd, co = 0; cd < end && !CISERR(); cd++, co++)
		if (!UNUSEDCOLOR(cd) && cd->sub != co && co != but &&
							!(cd->flags&PSEUDO))
			newarc(nfa, type, co, from, to);
}

/*
 - colorcomplement - add arcs of complementary colors
 * The calling sequence ought to be reconciled with cloneouts().
 ^ static VOID colorcomplement(struct nfa *, struct colormap *, int,
 ^ 	struct state *, struct state *, struct state *);
 */
static VOID
colorcomplement(nfa, cm, type, of, from, to)
struct nfa *nfa;
struct colormap *cm;
int type;
struct state *of;		/* complements of this guy's PLAIN outarcs */
struct state *from;
struct state *to;
{
	struct colordesc *cd;
	struct colordesc *end = CDEND(cm);
	color co;

	assert(of != from);
	for (cd = cm->cd, co = 0; cd < end && !CISERR(); cd++, co++)
		if (!UNUSEDCOLOR(cd) && !(cd->flags&PSEUDO))
			if (findarc(of, PLAIN, co) == NULL)
				newarc(nfa, type, co, from, to);
}



#ifdef REG_DEBUG
/*
 ^ #ifdef REG_DEBUG
 */

/*
 - dumpcolors - debugging output
 ^ static VOID dumpcolors(struct colormap *, FILE *);
 */
static VOID
dumpcolors(cm, f)
struct colormap *cm;
FILE *f;
{
	struct colordesc *cd;
	struct colordesc *end;
	color co;
	chr c;
	char *has;

	fprintf(f, "max %ld\n", (long)cm->max);
	if (NBYTS > 1)
		fillcheck(cm, cm->tree, 0, f);
	end = CDEND(cm);
	for (cd = cm->cd + 1, co = 1; cd < end; cd++, co++)	/* skip 0 */
		if (!UNUSEDCOLOR(cd)) {
			assert(cd->nchrs > 0);
			has = (cd->block != NULL) ? "#" : "";
			if (cd->flags&PSEUDO)
				fprintf(f, "#%2ld%s(ps): ", (long)co, has);
			else
				fprintf(f, "#%2ld%s(%2d): ", (long)co,
							has, cd->nchrs);
			/* it's hard to do this more efficiently */
			for (c = CHR_MIN; c < CHR_MAX; c++)
				if (GETCOLOR(cm, c) == co)
					dumpchr(c, f);
			assert(c == CHR_MAX);
			if (GETCOLOR(cm, c) == co)
				dumpchr(c, f);
			fprintf(f, "\n");
		}
}

/*
 - fillcheck - check proper filling of a tree
 ^ static VOID fillcheck(struct colormap *, union tree *, int, FILE *);
 */
static VOID
fillcheck(cm, tree, level, f)
struct colormap *cm;
union tree *tree;
int level;			/* level number (top == 0) of this block */
FILE *f;
{
	int i;
	union tree *t;
	union tree *fillt = &cm->tree[level+1];

	assert(level < NBYTS-1);	/* this level has pointers */
	for (i = BYTTAB-1; i >= 0; i--) {
		t = tree->tptr[i];
		if (t == NULL)
			fprintf(f, "NULL found in filled tree!\n");
		else if (t == fillt)
			{}
		else if (level < NBYTS-2)	/* more pointer blocks below */
			fillcheck(cm, t, level+1, f);
	}
}

/*
 - dumpchr - print a chr
 * Kind of char-centric but works well enough for debug use.
 ^ static VOID dumpchr(pchr, FILE *);
 */
static VOID
dumpchr(c, f)
pchr c;
FILE *f;
{
	if (c == '\\')
		fprintf(f, "\\\\");
	else if (c > ' ' && c <= '~')
		putc((char)c, f);
	else
		fprintf(f, "\\u%04lx", (long)c);
}

/*
 ^ #endif
 */
#endif				/* ifdef REG_DEBUG */
