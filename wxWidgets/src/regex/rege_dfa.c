/*
 * DFA routines
 * This file is #included by regexec.c.
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
 */

/*
 - longest - longest-preferred matching engine
 ^ static chr *longest(struct vars *, struct dfa *, chr *, chr *, int *);
 */
static chr *			/* endpoint, or NULL */
longest(v, d, start, stop, hitstopp)
struct vars *v;			/* used only for debug and exec flags */
struct dfa *d;
chr *start;			/* where the match should start */
chr *stop;			/* match must end at or before here */
int *hitstopp;			/* record whether hit v->stop, if non-NULL */
{
	chr *cp;
	chr *realstop = (stop == v->stop) ? stop : stop + 1;
	color co;
	struct sset *css;
	struct sset *ss;
	chr *post;
	int i;
	struct colormap *cm = d->cm;

	/* initialize */
	css = initialize(v, d, start);
	cp = start;
	if (hitstopp != NULL)
		*hitstopp = 0;

	/* startup */
	FDEBUG(("+++ startup +++\n"));
	if (cp == v->start) {
		co = d->cnfa->bos[(v->eflags&REG_NOTBOL) ? 0 : 1];
		FDEBUG(("color %ld\n", (long)co));
	} else {
		co = GETCOLOR(cm, *(cp - 1));
		FDEBUG(("char %c, color %ld\n", (char)*(cp-1), (long)co));
	}
	css = miss(v, d, css, co, cp, start);
	if (css == NULL)
		return NULL;
	css->lastseen = cp;

	/* main loop */
	if (v->eflags&REG_FTRACE)
		while (cp < realstop) {
			FDEBUG(("+++ at c%d +++\n", css - d->ssets));
			co = GETCOLOR(cm, *cp);
			FDEBUG(("char %c, color %ld\n", (char)*cp, (long)co));
			ss = css->outs[co];
			if (ss == NULL) {
				ss = miss(v, d, css, co, cp+1, start);
				if (ss == NULL)
					break;	/* NOTE BREAK OUT */
			}
			cp++;
			ss->lastseen = cp;
			css = ss;
		}
	else
		while (cp < realstop) {
			co = GETCOLOR(cm, *cp);
			ss = css->outs[co];
			if (ss == NULL) {
				ss = miss(v, d, css, co, cp+1, start);
				if (ss == NULL)
					break;	/* NOTE BREAK OUT */
			}
			cp++;
			ss->lastseen = cp;
			css = ss;
		}

	/* shutdown */
	FDEBUG(("+++ shutdown at c%d +++\n", css - d->ssets));
	if (cp == v->stop && stop == v->stop) {
		if (hitstopp != NULL)
			*hitstopp = 1;
		co = d->cnfa->eos[(v->eflags&REG_NOTEOL) ? 0 : 1];
		FDEBUG(("color %ld\n", (long)co));
		ss = miss(v, d, css, co, cp, start);
		/* special case:  match ended at eol? */
		if (ss != NULL && (ss->flags&POSTSTATE))
			return cp;
		else if (ss != NULL)
			ss->lastseen = cp;	/* to be tidy */
	}

	/* find last match, if any */
	post = d->lastpost;
	for (ss = d->ssets, i = d->nssused; i > 0; ss++, i--)
		if ((ss->flags&POSTSTATE) && post != ss->lastseen &&
					(post == NULL || post < ss->lastseen))
			post = ss->lastseen;
	if (post != NULL)		/* found one */
		return post - 1;

	return NULL;
}

/*
 - shortest - shortest-preferred matching engine
 ^ static chr *shortest(struct vars *, struct dfa *, chr *, chr *, chr *,
 ^ 	chr **, int *);
 */
static chr *			/* endpoint, or NULL */
shortest(v, d, start, min, max, coldp, hitstopp)
struct vars *v;
struct dfa *d;
chr *start;			/* where the match should start */
chr *min;			/* match must end at or after here */
chr *max;			/* match must end at or before here */
chr **coldp;			/* store coldstart pointer here, if nonNULL */
int *hitstopp;			/* record whether hit v->stop, if non-NULL */
{
	chr *cp;
	chr *realmin = (min == v->stop) ? min : min + 1;
	chr *realmax = (max == v->stop) ? max : max + 1;
	color co;
	struct sset *css;
	struct sset *ss;
	struct colormap *cm = d->cm;

	/* initialize */
	css = initialize(v, d, start);
	cp = start;
	if (hitstopp != NULL)
		*hitstopp = 0;

	/* startup */
	FDEBUG(("--- startup ---\n"));
	if (cp == v->start) {
		co = d->cnfa->bos[(v->eflags&REG_NOTBOL) ? 0 : 1];
		FDEBUG(("color %ld\n", (long)co));
	} else {
		co = GETCOLOR(cm, *(cp - 1));
		FDEBUG(("char %c, color %ld\n", (char)*(cp-1), (long)co));
	}
	css = miss(v, d, css, co, cp, start);
	if (css == NULL)
		return NULL;
	css->lastseen = cp;
	ss = css;

	/* main loop */
	if (v->eflags&REG_FTRACE)
		while (cp < realmax) {
			FDEBUG(("--- at c%d ---\n", css - d->ssets));
			co = GETCOLOR(cm, *cp);
			FDEBUG(("char %c, color %ld\n", (char)*cp, (long)co));
			ss = css->outs[co];
			if (ss == NULL) {
				ss = miss(v, d, css, co, cp+1, start);
				if (ss == NULL)
					break;	/* NOTE BREAK OUT */
			}
			cp++;
			ss->lastseen = cp;
			css = ss;
			if ((ss->flags&POSTSTATE) && cp >= realmin)
				break;		/* NOTE BREAK OUT */
		}
	else
		while (cp < realmax) {
			co = GETCOLOR(cm, *cp);
			ss = css->outs[co];
			if (ss == NULL) {
				ss = miss(v, d, css, co, cp+1, start);
				if (ss == NULL)
					break;	/* NOTE BREAK OUT */
			}
			cp++;
			ss->lastseen = cp;
			css = ss;
			if ((ss->flags&POSTSTATE) && cp >= realmin)
				break;		/* NOTE BREAK OUT */
		}

	if (ss == NULL)
		return NULL;

	if (coldp != NULL)	/* report last no-progress state set, if any */
		*coldp = lastcold(v, d);

	if ((ss->flags&POSTSTATE) && cp > min) {
		assert(cp >= realmin);
		cp--;
	} else if (cp == v->stop && max == v->stop) {
		co = d->cnfa->eos[(v->eflags&REG_NOTEOL) ? 0 : 1];
		FDEBUG(("color %ld\n", (long)co));
		ss = miss(v, d, css, co, cp, start);
		/* match might have ended at eol */
		if ((ss == NULL || !(ss->flags&POSTSTATE)) && hitstopp != NULL)
			*hitstopp = 1;
	}

	if (ss == NULL || !(ss->flags&POSTSTATE))
		return NULL;

	return cp;
}

/*
 - lastcold - determine last point at which no progress had been made
 ^ static chr *lastcold(struct vars *, struct dfa *);
 */
static chr *			/* endpoint, or NULL */
lastcold(v, d)
struct vars *v;
struct dfa *d;
{
	struct sset *ss;
	chr *nopr;
	int i;

	nopr = d->lastnopr;
	if (nopr == NULL)
		nopr = v->start;
	for (ss = d->ssets, i = d->nssused; i > 0; ss++, i--)
		if ((ss->flags&NOPROGRESS) && nopr < ss->lastseen)
			nopr = ss->lastseen;
	return nopr;
}

/*
 - newdfa - set up a fresh DFA
 ^ static struct dfa *newdfa(struct vars *, struct cnfa *,
 ^ 	struct colormap *, struct smalldfa *);
 */
 
/* FIXME Required for CW 8 on Mac since it's not in limits.h */
#ifndef __CHAR_BIT__
#define __CHAR_BIT__ 8
#endif 
 
static struct dfa *
newdfa(v, cnfa, cm, small)
struct vars *v;
struct cnfa *cnfa;
struct colormap *cm;
struct smalldfa *small;		/* preallocated space, may be NULL */
{
	struct dfa *d;
	size_t nss = cnfa->nstates * 2;
	int wordsper = (cnfa->nstates + UBITS - 1) / UBITS;
	struct smalldfa *smallwas = small;

	assert(cnfa != NULL && cnfa->nstates != 0);

	if (nss <= FEWSTATES && cnfa->ncolors <= FEWCOLORS) {
		assert(wordsper == 1);
		if (small == NULL) {
			small = (struct smalldfa *)MALLOC(
						sizeof(struct smalldfa));
			if (small == NULL) {
				ERR(REG_ESPACE);
				return NULL;
			}
		}
		d = &small->dfa;
		d->ssets = small->ssets;
		d->statesarea = small->statesarea;
		d->work = &d->statesarea[nss];
		d->outsarea = small->outsarea;
		d->incarea = small->incarea;
		d->cptsmalloced = 0;
		d->mallocarea = (smallwas == NULL) ? (char *)small : NULL;
	} else {
		d = (struct dfa *)MALLOC(sizeof(struct dfa));
		if (d == NULL) {
			ERR(REG_ESPACE);
			return NULL;
		}
		d->ssets = (struct sset *)MALLOC(nss * sizeof(struct sset));
		d->statesarea = (unsigned *)MALLOC((nss+WORK) * wordsper *
							sizeof(unsigned));
		d->work = &d->statesarea[nss * wordsper];
		d->outsarea = (struct sset **)MALLOC(nss * cnfa->ncolors *
							sizeof(struct sset *));
		d->incarea = (struct arcp *)MALLOC(nss * cnfa->ncolors *
							sizeof(struct arcp));
		d->cptsmalloced = 1;
		d->mallocarea = (char *)d;
		if (d->ssets == NULL || d->statesarea == NULL ||
				d->outsarea == NULL || d->incarea == NULL) {
			freedfa(d);
			ERR(REG_ESPACE);
			return NULL;
		}
	}

	d->nssets = (v->eflags&REG_SMALL) ? 7 : nss;
	d->nssused = 0;
	d->nstates = cnfa->nstates;
	d->ncolors = cnfa->ncolors;
	d->wordsper = wordsper;
	d->cnfa = cnfa;
	d->cm = cm;
	d->lastpost = NULL;
	d->lastnopr = NULL;
	d->search = d->ssets;

	/* initialization of sset fields is done as needed */

	return d;
}

/*
 - freedfa - free a DFA
 ^ static VOID freedfa(struct dfa *);
 */
static VOID
freedfa(d)
struct dfa *d;
{
	if (d->cptsmalloced) {
		if (d->ssets != NULL)
			FREE(d->ssets);
		if (d->statesarea != NULL)
			FREE(d->statesarea);
		if (d->outsarea != NULL)
			FREE(d->outsarea);
		if (d->incarea != NULL)
			FREE(d->incarea);
	}

	if (d->mallocarea != NULL)
		FREE(d->mallocarea);
}

/*
 - hash - construct a hash code for a bitvector
 * There are probably better ways, but they're more expensive.
 ^ static unsigned hash(unsigned *, int);
 */
static unsigned
hash(uv, n)
unsigned *uv;
int n;
{
	int i;
	unsigned h;

	h = 0;
	for (i = 0; i < n; i++)
		h ^= uv[i];
	return h;
}

/*
 - initialize - hand-craft a cache entry for startup, otherwise get ready
 ^ static struct sset *initialize(struct vars *, struct dfa *, chr *);
 */
static struct sset *
initialize(v, d, start)
struct vars *v;			/* used only for debug flags */
struct dfa *d;
chr *start;
{
	struct sset *ss;
	int i;

	/* is previous one still there? */
	if (d->nssused > 0 && (d->ssets[0].flags&STARTER))
		ss = &d->ssets[0];
	else {				/* no, must (re)build it */
		ss = getvacant(v, d, start, start);
		for (i = 0; i < d->wordsper; i++)
			ss->states[i] = 0;
		BSET(ss->states, d->cnfa->pre);
		ss->hash = HASH(ss->states, d->wordsper);
		assert(d->cnfa->pre != d->cnfa->post);
		ss->flags = STARTER|LOCKED|NOPROGRESS;
		/* lastseen dealt with below */
	}

	for (i = 0; i < d->nssused; i++)
		d->ssets[i].lastseen = NULL;
	ss->lastseen = start;		/* maybe untrue, but harmless */
	d->lastpost = NULL;
	d->lastnopr = NULL;
	return ss;
}

/*
 - miss - handle a cache miss
 ^ static struct sset *miss(struct vars *, struct dfa *, struct sset *,
 ^ 	pcolor, chr *, chr *);
 */
static struct sset *		/* NULL if goes to empty set */
miss(v, d, css, co, cp, start)
struct vars *v;			/* used only for debug flags */
struct dfa *d;
struct sset *css;
pcolor co;
chr *cp;			/* next chr */
chr *start;			/* where the attempt got started */
{
	struct cnfa *cnfa = d->cnfa;
	int i;
	unsigned h;
	struct carc *ca;
	struct sset *p;
	int ispost;
	int noprogress;
	int gotstate;
	int dolacons;
	int sawlacons;

	/* for convenience, we can be called even if it might not be a miss */
	if (css->outs[co] != NULL) {
		FDEBUG(("hit\n"));
		return css->outs[co];
	}
	FDEBUG(("miss\n"));

	/* first, what set of states would we end up in? */
	for (i = 0; i < d->wordsper; i++)
		d->work[i] = 0;
	ispost = 0;
	noprogress = 1;
	gotstate = 0;
	for (i = 0; i < d->nstates; i++)
		if (ISBSET(css->states, i))
			for (ca = cnfa->states[i]+1; ca->co != COLORLESS; ca++)
				if (ca->co == co) {
					BSET(d->work, ca->to);
					gotstate = 1;
					if (ca->to == cnfa->post)
						ispost = 1;
					if (!cnfa->states[ca->to]->co)
						noprogress = 0;
					FDEBUG(("%d -> %d\n", i, ca->to));
				}
	dolacons = (gotstate) ? (cnfa->flags&HASLACONS) : 0;
	sawlacons = 0;
	while (dolacons) {		/* transitive closure */
		dolacons = 0;
		for (i = 0; i < d->nstates; i++)
			if (ISBSET(d->work, i))
				for (ca = cnfa->states[i]+1; ca->co != COLORLESS;
									ca++) {
					if (ca->co <= cnfa->ncolors)
						continue; /* NOTE CONTINUE */
					sawlacons = 1;
					if (ISBSET(d->work, ca->to))
						continue; /* NOTE CONTINUE */
					if (!lacon(v, cnfa, cp, ca->co))
						continue; /* NOTE CONTINUE */
					BSET(d->work, ca->to);
					dolacons = 1;
					if (ca->to == cnfa->post)
						ispost = 1;
					if (!cnfa->states[ca->to]->co)
						noprogress = 0;
					FDEBUG(("%d :> %d\n", i, ca->to));
				}
	}
	if (!gotstate)
		return NULL;
	h = HASH(d->work, d->wordsper);

	/* next, is that in the cache? */
	for (p = d->ssets, i = d->nssused; i > 0; p++, i--)
		if (HIT(h, d->work, p, d->wordsper)) {
			FDEBUG(("cached c%d\n", p - d->ssets));
			break;			/* NOTE BREAK OUT */
		}
	if (i == 0) {		/* nope, need a new cache entry */
		p = getvacant(v, d, cp, start);
		assert(p != css);
		for (i = 0; i < d->wordsper; i++)
			p->states[i] = d->work[i];
		p->hash = h;
		p->flags = (ispost) ? POSTSTATE : 0;
		if (noprogress)
			p->flags |= NOPROGRESS;
		/* lastseen to be dealt with by caller */
	}

	if (!sawlacons) {		/* lookahead conds. always cache miss */
		FDEBUG(("c%d[%d]->c%d\n", css - d->ssets, co, p - d->ssets));
		css->outs[co] = p;
		css->inchain[co] = p->ins;
		p->ins.ss = css;
		p->ins.co = (color)co;
	}
	return p;
}

/*
 - lacon - lookahead-constraint checker for miss()
 ^ static int lacon(struct vars *, struct cnfa *, chr *, pcolor);
 */
static int			/* predicate:  constraint satisfied? */
lacon(v, pcnfa, cp, co)
struct vars *v;
struct cnfa *pcnfa;		/* parent cnfa */
chr *cp;
pcolor co;			/* "color" of the lookahead constraint */
{
	int n;
	struct subre *sub;
	struct dfa *d;
	struct smalldfa sd;
	chr *end;

	n = co - pcnfa->ncolors;
	assert(n < v->g->nlacons && v->g->lacons != NULL);
	FDEBUG(("=== testing lacon %d\n", n));
	sub = &v->g->lacons[n];
	d = newdfa(v, &sub->cnfa, &v->g->cmap, &sd);
	if (d == NULL) {
		ERR(REG_ESPACE);
		return 0;
	}
	end = longest(v, d, cp, v->stop, (int *)NULL);
	freedfa(d);
	FDEBUG(("=== lacon %d match %d\n", n, (end != NULL)));
	return (sub->subno) ? (end != NULL) : (end == NULL);
}

/*
 - getvacant - get a vacant state set
 * This routine clears out the inarcs and outarcs, but does not otherwise
 * clear the innards of the state set -- that's up to the caller.
 ^ static struct sset *getvacant(struct vars *, struct dfa *, chr *, chr *);
 */
static struct sset *
getvacant(v, d, cp, start)
struct vars *v;			/* used only for debug flags */
struct dfa *d;
chr *cp;
chr *start;
{
	int i;
	struct sset *ss;
	struct sset *p;
	struct arcp ap;
	struct arcp lastap;
	color co;
    lastap.ss = NULL; lastap.co = 0; // WX: suppress dummy gcc warnings
	ss = pickss(v, d, cp, start);
	assert(!(ss->flags&LOCKED));

	/* clear out its inarcs, including self-referential ones */
	ap = ss->ins;
	while ((p = ap.ss) != NULL) {
		co = ap.co;
		FDEBUG(("zapping c%d's %ld outarc\n", p - d->ssets, (long)co));
		p->outs[co] = NULL;
		ap = p->inchain[co];
		p->inchain[co].ss = NULL;	/* paranoia */
	}
	ss->ins.ss = NULL;

	/* take it off the inarc chains of the ssets reached by its outarcs */
	for (i = 0; i < d->ncolors; i++) {
		p = ss->outs[i];
		assert(p != ss);		/* not self-referential */
		if (p == NULL)
			continue;		/* NOTE CONTINUE */
		FDEBUG(("del outarc %d from c%d's in chn\n", i, p - d->ssets));
		if (p->ins.ss == ss && p->ins.co == i)
			p->ins = ss->inchain[i];
		else {
			assert(p->ins.ss != NULL);
			for (ap = p->ins; ap.ss != NULL &&
						!(ap.ss == ss && ap.co == i);
						ap = ap.ss->inchain[ap.co])
				lastap = ap;
			assert(ap.ss != NULL);
			lastap.ss->inchain[lastap.co] = ss->inchain[i];
		}
		ss->outs[i] = NULL;
		ss->inchain[i].ss = NULL;
	}

	/* if ss was a success state, may need to remember location */
	if ((ss->flags&POSTSTATE) && ss->lastseen != d->lastpost &&
			(d->lastpost == NULL || d->lastpost < ss->lastseen))
		d->lastpost = ss->lastseen;

	/* likewise for a no-progress state */
	if ((ss->flags&NOPROGRESS) && ss->lastseen != d->lastnopr &&
			(d->lastnopr == NULL || d->lastnopr < ss->lastseen))
		d->lastnopr = ss->lastseen;

	return ss;
}

/*
 - pickss - pick the next stateset to be used
 ^ static struct sset *pickss(struct vars *, struct dfa *, chr *, chr *);
 */
static struct sset *
pickss(v, d, cp, start)
struct vars *v;			/* used only for debug flags */
struct dfa *d;
chr *cp;
chr *start;
{
	int i;
	struct sset *ss;
	struct sset *end;
	chr *ancient;

	/* shortcut for cases where cache isn't full */
	if (d->nssused < d->nssets) {
		i = d->nssused;
		d->nssused++;
		ss = &d->ssets[i];
		FDEBUG(("new c%d\n", i));
		/* set up innards */
		ss->states = &d->statesarea[i * d->wordsper];
		ss->flags = 0;
		ss->ins.ss = NULL;
		ss->ins.co = WHITE;		/* give it some value */
		ss->outs = &d->outsarea[i * d->ncolors];
		ss->inchain = &d->incarea[i * d->ncolors];
		for (i = 0; i < d->ncolors; i++) {
			ss->outs[i] = NULL;
			ss->inchain[i].ss = NULL;
		}
		return ss;
	}

	/* look for oldest, or old enough anyway */
	if (cp - start > d->nssets*2/3)		/* oldest 33% are expendable */
		ancient = cp - d->nssets*2/3;
	else
		ancient = start;
	for (ss = d->search, end = &d->ssets[d->nssets]; ss < end; ss++)
		if ((ss->lastseen == NULL || ss->lastseen < ancient) &&
							!(ss->flags&LOCKED)) {
			d->search = ss + 1;
			FDEBUG(("replacing c%d\n", ss - d->ssets));
			return ss;
		}
	for (ss = d->ssets, end = d->search; ss < end; ss++)
		if ((ss->lastseen == NULL || ss->lastseen < ancient) &&
							!(ss->flags&LOCKED)) {
			d->search = ss + 1;
			FDEBUG(("replacing c%d\n", ss - d->ssets));
			return ss;
		}

	/* nobody's old enough?!? -- something's really wrong */
	FDEBUG(("can't find victim to replace!\n"));
	assert(NOTREACHED);
	ERR(REG_ASSERT);
	return d->ssets;
}
