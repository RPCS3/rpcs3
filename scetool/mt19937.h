//Mersenne-Twister 19937 pseudorandom number generator.
//Reference implementation at:
//http://www.math.sci.hiroshima-u.ac.jp/~m-mat/MT/MT2002/CODES/mt19937ar.c

#ifndef _MT19937_H_
#define _MT19937_H_

/*! State size. */
#define MT_N 624
#define MT_M 397
#define MT_MATRIX_A 0x9908b0df
#define MT_UPPER_MASK 0x80000000
#define MT_LOWER_MASK 0x7fffffff

/*! Mersenne-Twister 19937 context. */
typedef struct _mt19937_ctxt
{
	/*! State. */
	unsigned int state[MT_N];
	/*! Index. */
	unsigned int idx;
} mt19937_ctxt_t;

/*!
* \brief Initialize Mersenne-Twister 19937 context.
*
* \param ctxt Mersenne-Twister 19937 context.
* \param seed Random seed.
*/
void mt19937_init(mt19937_ctxt_t *ctxt, unsigned int seed);

/*!
* \brief Update Mersenne-Twister 19937 state.
*
* \param ctxt Mersenne-Twister 19937 context.
*
* \return Generated pseudorandom number.
*/
unsigned int mt19937_update(mt19937_ctxt_t *ctxt);

#endif
