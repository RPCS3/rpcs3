float	ycbcrCoeffs[3] = { .299, .587, .114 };
/* default coding range is CCIR Rec 601-1 with no headroom/footroom */
unsigned long refBlackWhite[6] = { 0, 255, 128, 255, 128, 255 };

#define	LumaRed		ycbcrCoeffs[0]
#define	LumaGreen	ycbcrCoeffs[1]
#define	LumaBlue	ycbcrCoeffs[2]

long	eRtotal = 0;
long	eGtotal = 0;
long	eBtotal = 0;
long	preveRtotal = 0;
long	preveGtotal = 0;
long	preveBtotal = 0;
unsigned long AbseRtotal = 0;
unsigned long AbseGtotal = 0;
unsigned long AbseBtotal = 0;
unsigned long eCodes = 0;
unsigned long preveCodes = 0;
unsigned long eBits = 0;
unsigned long preveBits = 0;

static	void setupLumaTables();
static int abs(int v) { return (v < 0 ? -v : v); }
static double pct(int v,double range) { return (v*100. / range); }
static void check(int R, int G, int B);

float	D1, D2;
float	D3, D4;
float	D5, D6;

int
main(int argc, char** argv)
{
    int R, G, B;

    if (argc > 1) {
	refBlackWhite[0] = 16;
	refBlackWhite[1] = 235;
	refBlackWhite[2] = 128;
	refBlackWhite[3] = 240;
	refBlackWhite[4] = 128;
	refBlackWhite[5] = 240;
    }
    D3 = 2 - 2*LumaRed;
    D4 = 2 - 2*LumaBlue;
    D1 = 1. / D3;
    D2 = 1. / D4;
    D5 = D3*LumaRed / LumaGreen;
    D6 = D4*LumaBlue / LumaGreen;
    setupLumaTables();
    for (R = 0; R < 256; R++) {
	for (G = 0; G < 256; G++)
	    for (B = 0; B < 256; B++)
		check(R, G, B);
	printf("[%3u] c %u/%u b %u/%u (R %u/%d/%u G %u/%d/%u B %u/%d/%u)\n"
	    , R
	    , eCodes - preveCodes, eCodes
	    , eBits - preveBits, eBits
	    , abs(AbseRtotal - preveRtotal), eRtotal , AbseRtotal
	    , abs(AbseGtotal - preveGtotal), eGtotal , AbseGtotal
	    , abs(AbseBtotal - preveBtotal), eBtotal , AbseBtotal
	);
	preveRtotal = AbseRtotal;
	preveGtotal = AbseGtotal;
	preveBtotal = AbseBtotal;
	preveCodes = eCodes;
	preveBits = eBits;
    }
    printf("%u total codes\n", 256*256*256);
    printf("total error: %u codes %u bits (R %d/%u G %d/%u B %d/%u)\n"
	, eCodes
	, eBits
	, eRtotal , AbseRtotal
	, eGtotal , AbseGtotal
	, eBtotal , AbseBtotal
    );
    return (0);
}

float	*lumaRed;
float	*lumaGreen;
float	*lumaBlue;

static float*
setupLuma(float c)
{
    float *v = (float *)_TIFFmalloc(256 * sizeof (float));
    int i;
    for (i = 0; i < 256; i++)
	v[i] = c * i;
    return (v);
}

static void
setupLumaTables(void)
{
    lumaRed = setupLuma(LumaRed);
    lumaGreen = setupLuma(LumaGreen);
    lumaBlue = setupLuma(LumaBlue);
}

static unsigned
V2Code(float f, unsigned long RB, unsigned long RW, int CR)
{
    unsigned int c = (unsigned int)((((f)*(RW-RB)/CR)+RB)+.5);
    return (c > 255 ? 255 : c);
}

#define	Code2V(c, RB, RW, CR)	((((c)-(int)RB)*(float)CR)/(float)(RW-RB))

#define	CLAMP(f,min,max) \
    (int)((f)+.5 < (min) ? (min) : (f)+.5 > (max) ? (max) : (f)+.5)

static
void
check(int R, int G, int B)
{
    float Y, Cb, Cr;
    int iY, iCb, iCr;
    float rY, rCb, rCr;
    float rR, rG, rB;
    int eR, eG, eB;

    Y = lumaRed[R] + lumaGreen[G] + lumaBlue[B];
    Cb = (B - Y)*D2;
    Cr = (R - Y)*D1;
    iY = V2Code(Y, refBlackWhite[0], refBlackWhite[1], 255);
    iCb = V2Code(Cb, refBlackWhite[2], refBlackWhite[3], 127);
    iCr = V2Code(Cr, refBlackWhite[4], refBlackWhite[5], 127);
    rCb = Code2V(iCb, refBlackWhite[2], refBlackWhite[3], 127);
    rCr = Code2V(iCr, refBlackWhite[4], refBlackWhite[5], 127);
    rY = Code2V(iY, refBlackWhite[0], refBlackWhite[1], 255);
    rR = rY + rCr*D3;
    rB = rY + rCb*D4;
    rG = rY - rCb*D6 - rCr*D5;
    eR = R - CLAMP(rR,0,255);
    eG = G - CLAMP(rG,0,255);
    eB = B - CLAMP(rB,0,255);
    if (abs(eR) > 1 || abs(eG) > 1 || abs(eB) > 1) {
	printf("R %u G %u B %u", R, G, B);
	printf(" Y %g Cb %g Cr %g", Y, Cb, Cr);
	printf(" iY %u iCb %u iCr %u", iY, iCb, iCr);
	printf("\n -> Y %g Cb %g Cr %g", rY, rCb, rCr);
	printf(" R %g (%u) G %g (%u) B %g (%u) E=[%d %d %d])\n"
	    , rR, CLAMP(rR,0,255)
	    , rG, CLAMP(rG,0,255)
	    , rB, CLAMP(rB,0,255)
	    , eR, eG, eB
	);
    }
    eRtotal += eR;
    eGtotal += eG;
    eBtotal += eB;
    AbseRtotal += abs(eR);
    AbseGtotal += abs(eG);
    AbseBtotal += abs(eB);
    if (eR | eG | eB)
	eCodes++;
    eBits += abs(eR) + abs(eG) + abs(eB);
}
/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 8
 * fill-column: 78
 * End:
 */
