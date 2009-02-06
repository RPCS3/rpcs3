#ifndef ZEROGS_MATH_H
#define ZEROGS_MATH_H

#ifndef _WIN32
#include <alloca.h>
#endif

#include <string.h>
#include <math.h>

#ifndef PI
#define PI ((dReal)3.141592654)
#endif

#define rswap(x, y) *(int*)&(x) ^= *(int*)&(y) ^= *(int*)&(x) ^= *(int*)&(y);

template <class T> inline T RAD_2_DEG(T radians)  { return (radians * (T)57.29577951); }

class Transform;
class TransformMatrix;

typedef float dReal;
typedef dReal dMatrix3[3*4];

inline dReal* normalize3(dReal* pfout, const dReal* pf);
inline dReal* normalize4(dReal* pfout, const dReal* pf);
inline dReal* cross3(dReal* pfout, const dReal* pf1, const dReal* pf2);

// multiplies 3x3 matrices
inline dReal* mult3(dReal* pfres, const dReal* pf1, const dReal* pf2);
inline double* mult3(double* pfres, const double* pf1, const double* pf2);

inline dReal* inv3(const dReal* pf, dReal* pfres, int stride);
inline dReal* inv4(const dReal* pf, dReal* pfres);

// class used for 3 and 4 dim vectors and quaternions
// It is better to use this for a 3 dim vector because it is 16byte aligned and SIMD instructions can be used
class Vector
{
public:
	dReal x, y, z, w;

	Vector() : x(0), y(0), z(0), w(0) {}
	Vector(dReal x, dReal y, dReal z) : x(x), y(y), z(z), w(0) {}
	Vector(dReal x, dReal y, dReal z, dReal w) : x(x), y(y), z(z), w(w) {}
	Vector(const Vector &vec) : x(vec.x), y(vec.y), z(vec.z), w(vec.w) {}
	Vector(const dReal* pf) { assert(pf != NULL); x = pf[0]; y = pf[1]; z = pf[2]; w = 0; }

	dReal  operator[](int i) const	   { return (&x)[i]; }
	dReal& operator[](int i)			 { return (&x)[i]; }

	// casting operators
	operator dReal* () { return &x; }
	operator const dReal* () const { return (const dReal*)&x; }

	// SCALAR FUNCTIONS
	inline dReal dot(const Vector &v) const { return x*v.x + y*v.y + z*v.z + w*v.w; }
	inline void normalize() { normalize4(&x, &x); }

	inline void Set3(const float* pvals) { x = pvals[0]; y = pvals[1]; z = pvals[2]; }
	inline void Set4(const float* pvals) { x = pvals[0]; y = pvals[1]; z = pvals[2]; w = pvals[3]; }

	// 3 dim cross product, w is not touched
	/// this = this x v
	inline void Cross(const Vector &v) { cross3(&x, &x, v); }

	/// this = u x v
	inline void Cross(const Vector &u, const Vector &v) { cross3(&x, u, v); }

	inline Vector operator-() const { Vector v; v.x = -x; v.y = -y; v.z = -z; v.w = -w; return v; }
	inline Vector operator+(const Vector &r) const { Vector v; v.x = x+r.x; v.y = y+r.y; v.z = z+r.z; v.w = w+r.w; return v; }
	inline Vector operator-(const Vector &r) const { Vector v; v.x = x-r.x; v.y = y-r.y; v.z = z-r.z; v.w = w-r.w; return v; }
	inline Vector operator*(const Vector &r) const { Vector v; v.x = r.x*x; v.y = r.y*y; v.z = r.z*z; v.w = r.w*w; return v; }
	inline Vector operator*(dReal k) const { Vector v; v.x = k*x; v.y = k*y; v.z = k*z; v.w = k*w; return v; }

	inline Vector& operator += (const Vector& r) { x += r.x; y += r.y; z += r.z; w += r.w; return *this; }
	inline Vector& operator -= (const Vector& r) { x -= r.x; y -= r.y; z -= r.z; w -= r.w; return *this; }
	inline Vector& operator *= (const Vector& r) { x *= r.x; y *= r.y; z *= r.z; w *= r.w; return *this; }
	
	inline Vector& operator *= (const dReal k) { x *= k; y *= k; z *= k; w *= k; return *this; }
	inline Vector& operator /= (const dReal _k) { dReal k=1/_k; x *= k; y *= k; z *= k; w *= k; return *this; }

	friend Vector operator* (float f, const Vector& v);
	//friend ostream& operator<<(ostream& O, const Vector& v);
	//friend istream& operator>>(istream& I, Vector& v);
};

inline Vector operator* (float f, const Vector& left)
{
	Vector v;
	v.x = f * left.x;
	v.y = f * left.y;
	v.z = f * left.z;
	return v;
}

struct AABB
{
	Vector pos, extents;
};

struct OBB
{
	Vector right, up, dir, pos, extents;
};

struct TRIANGLE
{
	TRIANGLE() {}
	TRIANGLE(const Vector& v1, const Vector& v2, const Vector& v3) : v1(v1), v2(v2), v3(v3) {}
	~TRIANGLE() {}

	Vector v1, v2, v3;	  //!< the vertices of the triangle

	const Vector& operator[](int i) const { return (&v1)[i]; }
	Vector&	   operator[](int i)	   { return (&v1)[i]; }

	/// assumes CCW ordering of vertices 
	inline Vector ComputeNormal() {
		Vector normal;
		cross3(normal, v2-v1, v3-v1);
		return normal;
	}
};

// Routines made for 3D graphics that deal with 3 or 4 dim algebra structures
// Functions with postfix 3 are for 3x3 operations, etc

// all fns return pfout on success or NULL on failure
// results and arguments can share pointers


// multiplies 4x4 matrices
inline dReal* mult4(dReal* pfres, const dReal* pf1, const dReal* pf2);
inline double* mult4(double* pfres, const double* pf1, const double* pf2);

// pf1^T * pf2
inline dReal* multtrans3(dReal* pfres, const dReal* pf1, const dReal* pf2);
inline double* multtrans3(double* pfres, const double* pf1, const double* pf2);
inline dReal* multtrans4(dReal* pfres, const dReal* pf1, const dReal* pf2);
inline double* multtrans4(double* pfres, const double* pf1, const double* pf2);

inline dReal* transpose3(const dReal* pf, dReal* pfres);
inline double* transpose3(const double* pf, double* pfres);
inline dReal* transpose4(const dReal* pf, dReal* pfres);
inline double* transpose4(const double* pf, double* pfres);

inline dReal dot2(const dReal* pf1, const dReal* pf2);
inline dReal dot3(const dReal* pf1, const dReal* pf2);
inline dReal dot4(const dReal* pf1, const dReal* pf2);

inline dReal lengthsqr2(const dReal* pf);
inline dReal lengthsqr3(const dReal* pf);
inline dReal lengthsqr4(const dReal* pf);

inline dReal* normalize2(dReal* pfout, const dReal* pf);
inline dReal* normalize3(dReal* pfout, const dReal* pf);
inline dReal* normalize4(dReal* pfout, const dReal* pf);

////
// More complex ops that deal with arbitrary matrices //
////

// extract eigen values and vectors from a 2x2 matrix and returns true if all values are real
// returned eigen vectors are normalized
inline bool eig2(const dReal* pfmat, dReal* peigs, dReal& fv1x, dReal& fv1y, dReal& fv2x, dReal& fv2y);

// Simple routines for linear algebra algorithms //
int CubicRoots (double c0, double c1, double c2, double *r0, double *r1, double *r2);
bool QLAlgorithm3 (dReal* m_aafEntry, dReal* afDiag, dReal* afSubDiag);

void EigenSymmetric3(dReal* fCovariance, dReal* eval, dReal* fAxes);

void GetCovarBasisVectors(dReal fCovariance[3][3], Vector* vRight, Vector* vUp, Vector* vDir);

// first root returned is always >= second, roots are defined if the quadratic doesn't have real solutions
void QuadraticSolver(dReal* pfQuadratic, dReal* pfRoots);

int insideQuadrilateral(const Vector* p0,const Vector* p1, const Vector* p2,const Vector* p3);
int insideTriangle(const Vector* p0, const Vector* p1, const Vector* p2);

// multiplies a matrix by a scalar
template <class T> inline void mult(T* pf, T fa, int r);

// multiplies a r1xc1 by c1xc2 matrix into pfres, if badd is true adds the result to pfres
// does not handle cases where pfres is equal to pf1 or pf2, use multtox for those cases
template <class T, class S, class R>
inline T* mult(T* pf1, R* pf2, int r1, int c1, int c2, S* pfres, bool badd = false);

// pf1 is transposed before mult
// rows of pf2 must equal rows of pf1
// pfres will be c1xc2 matrix
template <class T, class S, class R>
inline T* multtrans(T* pf1, R* pf2, int r1, int c1, int c2, S* pfres, bool badd = false);

// pf2 is transposed before mult
// the columns of both matrices must be the same and equal to c1
// r2 is the number of rows in pf2
// pfres must be an r1xr2 matrix
template <class T, class S, class R>
inline T* multtrans_to2(T* pf1, R* pf2, int r1, int c1, int r2, S* pfres, bool badd = false);

// multiplies rxc matrix pf1 and cxc matrix pf2 and stores the result in pf1, 
// the function needs a temporary buffer the size of c doubles, if pftemp == NULL,
// the function will allocate the necessary memory, otherwise pftemp should be big
// enough to store all the entries
template <class T> inline T* multto1(T* pf1, T* pf2, int r1, int c1, T* pftemp = NULL);

// same as multto1 except stores the result in pf2, pf1 has to be an r2xr2 matrix
// pftemp must be of size r2 if not NULL
template <class T, class S> inline T* multto2(T* pf1, S* pf2, int r2, int c2, S* pftemp = NULL);

// add pf1 + pf2 and store in pf1
template <class T> inline void sub(T* pf1, T* pf2, int r);
template <class T> inline T normsqr(T* pf1, int r);
template <class T> inline T lengthsqr(T* pf1, T* pf2, int length);
template <class T> inline T dot(T* pf1, T* pf2, int length);

template <class T> inline T sum(T* pf, int length);

// takes the inverse of the 3x3 matrix pf and stores it into pfres, returns true if matrix is invertible
template <class T> inline bool inv2(T* pf, T* pfres);

///////////////////////
// Function Definitions
///////////////////////
bool eig2(const dReal* pfmat, dReal* peigs, dReal& fv1x, dReal& fv1y, dReal& fv2x, dReal& fv2y)
{
	// x^2 + bx + c
	dReal a, b, c, d;
	b = -(pfmat[0] + pfmat[3]);
	c = pfmat[0] * pfmat[3] - pfmat[1] * pfmat[2];
	d = b * b - 4.0f * c + 1e-16f;

	if( d < 0 ) return false;
	if( d < 1e-16f ) {
		a = -0.5f * b;
		peigs[0] = a;	peigs[1] = a;
		fv1x = pfmat[1];		fv1y = a - pfmat[0];
		c = 1 / sqrtf(fv1x*fv1x + fv1y*fv1y);
		fv1x *= c;		fv1y *= c;
		fv2x = -fv1y;		fv2y = fv1x;
		return true;
	}
	
	// two roots
	d = sqrtf(d);
	a = -0.5f * (b + d);
	peigs[0] = a;
	fv1x = pfmat[1];		fv1y = a-pfmat[0];
	c = 1 / sqrtf(fv1x*fv1x + fv1y*fv1y);
	fv1x *= c;		fv1y *= c;

	a += d;
	peigs[1] = a;
	fv2x = pfmat[1];		fv2y = a-pfmat[0];
	c = 1 / sqrtf(fv2x*fv2x + fv2y*fv2y);
	fv2x *= c;		fv2y *= c;
	return true;
}

//#ifndef TI_USING_IPP

// Functions that are replacable by ipp library funcs
template <class T> inline T* _mult3(T* pfres, const T* pf1, const T* pf2)
{
	assert( pf1 != NULL && pf2 != NULL && pfres != NULL );

	T* pfres2;
	if( pfres == pf1 || pfres == pf2 ) pfres2 = (T*)alloca(9 * sizeof(T));
	else pfres2 = pfres;

	pfres2[0*4+0] = pf1[0*4+0]*pf2[0*4+0]+pf1[0*4+1]*pf2[1*4+0]+pf1[0*4+2]*pf2[2*4+0];
	pfres2[0*4+1] = pf1[0*4+0]*pf2[0*4+1]+pf1[0*4+1]*pf2[1*4+1]+pf1[0*4+2]*pf2[2*4+1];
	pfres2[0*4+2] = pf1[0*4+0]*pf2[0*4+2]+pf1[0*4+1]*pf2[1*4+2]+pf1[0*4+2]*pf2[2*4+2];

	pfres2[1*4+0] = pf1[1*4+0]*pf2[0*4+0]+pf1[1*4+1]*pf2[1*4+0]+pf1[1*4+2]*pf2[2*4+0];
	pfres2[1*4+1] = pf1[1*4+0]*pf2[0*4+1]+pf1[1*4+1]*pf2[1*4+1]+pf1[1*4+2]*pf2[2*4+1];
	pfres2[1*4+2] = pf1[1*4+0]*pf2[0*4+2]+pf1[1*4+1]*pf2[1*4+2]+pf1[1*4+2]*pf2[2*4+2];

	pfres2[2*4+0] = pf1[2*4+0]*pf2[0*4+0]+pf1[2*4+1]*pf2[1*4+0]+pf1[2*4+2]*pf2[2*4+0];
	pfres2[2*4+1] = pf1[2*4+0]*pf2[0*4+1]+pf1[2*4+1]*pf2[1*4+1]+pf1[2*4+2]*pf2[2*4+1];
	pfres2[2*4+2] = pf1[2*4+0]*pf2[0*4+2]+pf1[2*4+1]*pf2[1*4+2]+pf1[2*4+2]*pf2[2*4+2];

	if( pfres2 != pfres ) memcpy(pfres, pfres2, 9*sizeof(T));

	return pfres;
}

inline dReal* mult3(dReal* pfres, const dReal* pf1, const dReal* pf2) { return _mult3<dReal>(pfres, pf1, pf2); }
inline double* mult3(double* pfres, const double* pf1, const double* pf2) { return _mult3<double>(pfres, pf1, pf2); }

template <class T> 
inline T* _mult4(T* pfres, const T* p1, const T* p2)
{
	assert( pfres != NULL && p1 != NULL && p2 != NULL );

	T* pfres2;
	if( pfres == p1 || pfres == p2 ) pfres2 = (T*)alloca(16 * sizeof(T));
	else pfres2 = pfres;

	pfres2[0*4+0] = p1[0*4+0]*p2[0*4+0] + p1[0*4+1]*p2[1*4+0] + p1[0*4+2]*p2[2*4+0] + p1[0*4+3]*p2[3*4+0];
	pfres2[0*4+1] = p1[0*4+0]*p2[0*4+1] + p1[0*4+1]*p2[1*4+1] + p1[0*4+2]*p2[2*4+1] + p1[0*4+3]*p2[3*4+1];
	pfres2[0*4+2] = p1[0*4+0]*p2[0*4+2] + p1[0*4+1]*p2[1*4+2] + p1[0*4+2]*p2[2*4+2] + p1[0*4+3]*p2[3*4+2];
	pfres2[0*4+3] = p1[0*4+0]*p2[0*4+3] + p1[0*4+1]*p2[1*4+3] + p1[0*4+2]*p2[2*4+3] + p1[0*4+3]*p2[3*4+3];

	pfres2[1*4+0] = p1[1*4+0]*p2[0*4+0] + p1[1*4+1]*p2[1*4+0] + p1[1*4+2]*p2[2*4+0] + p1[1*4+3]*p2[3*4+0];
	pfres2[1*4+1] = p1[1*4+0]*p2[0*4+1] + p1[1*4+1]*p2[1*4+1] + p1[1*4+2]*p2[2*4+1] + p1[1*4+3]*p2[3*4+1];
	pfres2[1*4+2] = p1[1*4+0]*p2[0*4+2] + p1[1*4+1]*p2[1*4+2] + p1[1*4+2]*p2[2*4+2] + p1[1*4+3]*p2[3*4+2];
	pfres2[1*4+3] = p1[1*4+0]*p2[0*4+3] + p1[1*4+1]*p2[1*4+3] + p1[1*4+2]*p2[2*4+3] + p1[1*4+3]*p2[3*4+3];

	pfres2[2*4+0] = p1[2*4+0]*p2[0*4+0] + p1[2*4+1]*p2[1*4+0] + p1[2*4+2]*p2[2*4+0] + p1[2*4+3]*p2[3*4+0];
	pfres2[2*4+1] = p1[2*4+0]*p2[0*4+1] + p1[2*4+1]*p2[1*4+1] + p1[2*4+2]*p2[2*4+1] + p1[2*4+3]*p2[3*4+1];
	pfres2[2*4+2] = p1[2*4+0]*p2[0*4+2] + p1[2*4+1]*p2[1*4+2] + p1[2*4+2]*p2[2*4+2] + p1[2*4+3]*p2[3*4+2];
	pfres2[2*4+3] = p1[2*4+0]*p2[0*4+3] + p1[2*4+1]*p2[1*4+3] + p1[2*4+2]*p2[2*4+3] + p1[2*4+3]*p2[3*4+3];

	pfres2[3*4+0] = p1[3*4+0]*p2[0*4+0] + p1[3*4+1]*p2[1*4+0] + p1[3*4+2]*p2[2*4+0] + p1[3*4+3]*p2[3*4+0];
	pfres2[3*4+1] = p1[3*4+0]*p2[0*4+1] + p1[3*4+1]*p2[1*4+1] + p1[3*4+2]*p2[2*4+1] + p1[3*4+3]*p2[3*4+1];
	pfres2[3*4+2] = p1[3*4+0]*p2[0*4+2] + p1[3*4+1]*p2[1*4+2] + p1[3*4+2]*p2[2*4+2] + p1[3*4+3]*p2[3*4+2];
	pfres2[3*4+3] = p1[3*4+0]*p2[0*4+3] + p1[3*4+1]*p2[1*4+3] + p1[3*4+2]*p2[2*4+3] + p1[3*4+3]*p2[3*4+3];

	if( pfres != pfres2 ) memcpy(pfres, pfres2, sizeof(T)*16);
	return pfres;
}

inline dReal* mult4(dReal* pfres, const dReal* pf1, const dReal* pf2) { return _mult4<dReal>(pfres, pf1, pf2); }
inline double* mult4(double* pfres, const double* pf1, const double* pf2) { return _mult4<double>(pfres, pf1, pf2); }

template <class T> 
inline T* _multtrans3(T* pfres, const T* pf1, const T* pf2)
{
	T* pfres2;
	if( pfres == pf1 ) pfres2 = (T*)alloca(9 * sizeof(T));
	else pfres2 = pfres;

	pfres2[0] = pf1[0]*pf2[0]+pf1[3]*pf2[3]+pf1[6]*pf2[6];
	pfres2[1] = pf1[0]*pf2[1]+pf1[3]*pf2[4]+pf1[6]*pf2[7];
	pfres2[2] = pf1[0]*pf2[2]+pf1[3]*pf2[5]+pf1[6]*pf2[8];

	pfres2[3] = pf1[1]*pf2[0]+pf1[4]*pf2[3]+pf1[7]*pf2[6];
	pfres2[4] = pf1[1]*pf2[1]+pf1[4]*pf2[4]+pf1[7]*pf2[7];
	pfres2[5] = pf1[1]*pf2[2]+pf1[4]*pf2[5]+pf1[7]*pf2[8];

	pfres2[6] = pf1[2]*pf2[0]+pf1[5]*pf2[3]+pf1[8]*pf2[6];
	pfres2[7] = pf1[2]*pf2[1]+pf1[5]*pf2[4]+pf1[8]*pf2[7];
	pfres2[8] = pf1[2]*pf2[2]+pf1[5]*pf2[5]+pf1[8]*pf2[8];

	if( pfres2 != pfres ) memcpy(pfres, pfres2, 9*sizeof(T));

	return pfres;
}

template <class T> 
inline T* _multtrans4(T* pfres, const T* pf1, const T* pf2)
{
	T* pfres2;
	if( pfres == pf1 ) pfres2 = (T*)alloca(16 * sizeof(T));
	else pfres2 = pfres;

	for(int i = 0; i < 4; ++i) {
		for(int j = 0; j < 4; ++j) {
			pfres[4*i+j] = pf1[i] * pf2[j] + pf1[i+4] * pf2[j+4] + pf1[i+8] * pf2[j+8] + pf1[i+12] * pf2[j+12];
		}
	}

	return pfres;
}

inline dReal* multtrans3(dReal* pfres, const dReal* pf1, const dReal* pf2) { return _multtrans3<dReal>(pfres, pf1, pf2); }
inline double* multtrans3(double* pfres, const double* pf1, const double* pf2) { return _multtrans3<double>(pfres, pf1, pf2); }
inline dReal* multtrans4(dReal* pfres, const dReal* pf1, const dReal* pf2) { return _multtrans4<dReal>(pfres, pf1, pf2); }
inline double* multtrans4(double* pfres, const double* pf1, const double* pf2) { return _multtrans4<double>(pfres, pf1, pf2); }

// stride is in T
template <class T> inline T* _inv3(const T* pf, T* pfres, int stride)
{
	T* pfres2;
	if( pfres == pf ) pfres2 = (T*)alloca(3 * stride * sizeof(T));
	else pfres2 = pfres;

	// inverse = C^t / det(pf) where C is the matrix of coefficients

	// calc C^t
	pfres2[0*stride + 0] = pf[1*stride + 1] * pf[2*stride + 2] - pf[1*stride + 2] * pf[2*stride + 1];
	pfres2[0*stride + 1] = pf[0*stride + 2] * pf[2*stride + 1] - pf[0*stride + 1] * pf[2*stride + 2];
	pfres2[0*stride + 2] = pf[0*stride + 1] * pf[1*stride + 2] - pf[0*stride + 2] * pf[1*stride + 1];
	pfres2[1*stride + 0] = pf[1*stride + 2] * pf[2*stride + 0] - pf[1*stride + 0] * pf[2*stride + 2];
	pfres2[1*stride + 1] = pf[0*stride + 0] * pf[2*stride + 2] - pf[0*stride + 2] * pf[2*stride + 0];
	pfres2[1*stride + 2] = pf[0*stride + 2] * pf[1*stride + 0] - pf[0*stride + 0] * pf[1*stride + 2];
	pfres2[2*stride + 0] = pf[1*stride + 0] * pf[2*stride + 1] - pf[1*stride + 1] * pf[2*stride + 0];
	pfres2[2*stride + 1] = pf[0*stride + 1] * pf[2*stride + 0] - pf[0*stride + 0] * pf[2*stride + 1];
	pfres2[2*stride + 2] = pf[0*stride + 0] * pf[1*stride + 1] - pf[0*stride + 1] * pf[1*stride + 0];

	T fdet = pf[0*stride + 2] * pfres2[2*stride + 0] + pf[1*stride + 2] * pfres2[2*stride + 1] +
		pf[2*stride + 2] * pfres2[2*stride + 2];

	if( fabs(fdet) < 1e-6 ) return NULL;

	fdet = 1 / fdet;
	//if( pfdet != NULL ) *pfdet = fdet;

	if( pfres != pf ) {
		pfres[0*stride+0] *= fdet;		pfres[0*stride+1] *= fdet;		pfres[0*stride+2] *= fdet;
		pfres[1*stride+0] *= fdet;		pfres[1*stride+1] *= fdet;		pfres[1*stride+2] *= fdet;
		pfres[2*stride+0] *= fdet;		pfres[2*stride+1] *= fdet;		pfres[2*stride+2] *= fdet;
		return pfres;
	}

	pfres[0*stride+0] = pfres2[0*stride+0] * fdet;
	pfres[0*stride+1] = pfres2[0*stride+1] * fdet;
	pfres[0*stride+2] = pfres2[0*stride+2] * fdet;
	pfres[1*stride+0] = pfres2[1*stride+0] * fdet;
	pfres[1*stride+1] = pfres2[1*stride+1] * fdet;
	pfres[1*stride+2] = pfres2[1*stride+2] * fdet;
	pfres[2*stride+0] = pfres2[2*stride+0] * fdet;
	pfres[2*stride+1] = pfres2[2*stride+1] * fdet;
	pfres[2*stride+2] = pfres2[2*stride+2] * fdet;
	return pfres;
}

inline dReal* inv3(const dReal* pf, dReal* pfres, int stride) { return _inv3<dReal>(pf, pfres, stride); }

// inverse if 92 mults and 39 adds
template <class T> inline T* _inv4(const T* pf, T* pfres)
{
	T* pfres2;
	if( pfres == pf ) pfres2 = (T*)alloca(16 * sizeof(T));
	else pfres2 = pfres;

	// inverse = C^t / det(pf) where C is the matrix of coefficients

	// calc C^t

	// determinants of all possibel 2x2 submatrices formed by last two rows
	T fd0, fd1, fd2;
	T f1, f2, f3;
	fd0 = pf[2*4 + 0] * pf[3*4 + 1] - pf[2*4 + 1] * pf[3*4 + 0];
	fd1 = pf[2*4 + 1] * pf[3*4 + 2] - pf[2*4 + 2] * pf[3*4 + 1];
	fd2 = pf[2*4 + 2] * pf[3*4 + 3] - pf[2*4 + 3] * pf[3*4 + 2];

	f1 = pf[2*4 + 1] * pf[3*4 + 3] - pf[2*4 + 3] * pf[3*4 + 1];
	f2 = pf[2*4 + 0] * pf[3*4 + 3] - pf[2*4 + 3] * pf[3*4 + 0];
	f3 = pf[2*4 + 0] * pf[3*4 + 2] - pf[2*4 + 2] * pf[3*4 + 0];

	pfres2[0*4 + 0] =   pf[1*4 + 1] * fd2 - pf[1*4 + 2] * f1 + pf[1*4 + 3] * fd1;
	pfres2[0*4 + 1] = -(pf[0*4 + 1] * fd2 - pf[0*4 + 2] * f1 + pf[0*4 + 3] * fd1);

	pfres2[1*4 + 0] = -(pf[1*4 + 0] * fd2 - pf[1*4 + 2] * f2 + pf[1*4 + 3] * f3);
	pfres2[1*4 + 1] =   pf[0*4 + 0] * fd2 - pf[0*4 + 2] * f2 + pf[0*4 + 3] * f3;

	pfres2[2*4 + 0] =   pf[1*4 + 0] * f1 -	pf[1*4 + 1] * f2 + pf[1*4 + 3] * fd0;
	pfres2[2*4 + 1] = -(pf[0*4 + 0] * f1 -	pf[0*4 + 1] * f2 + pf[0*4 + 3] * fd0);

	pfres2[3*4 + 0] = -(pf[1*4 + 0] * fd1 - pf[1*4 + 1] * f3 + pf[1*4 + 2] * fd0);
	pfres2[3*4 + 1] =   pf[0*4 + 0] * fd1 - pf[0*4 + 1] * f3 + pf[0*4 + 2] * fd0;

	// determinants of first 2 rows of 4x4 matrix
	fd0 = pf[0*4 + 0] * pf[1*4 + 1] - pf[0*4 + 1] * pf[1*4 + 0];
	fd1 = pf[0*4 + 1] * pf[1*4 + 2] - pf[0*4 + 2] * pf[1*4 + 1];
	fd2 = pf[0*4 + 2] * pf[1*4 + 3] - pf[0*4 + 3] * pf[1*4 + 2];

	f1 = pf[0*4 + 1] * pf[1*4 + 3] - pf[0*4 + 3] * pf[1*4 + 1];
	f2 = pf[0*4 + 0] * pf[1*4 + 3] - pf[0*4 + 3] * pf[1*4 + 0];
	f3 = pf[0*4 + 0] * pf[1*4 + 2] - pf[0*4 + 2] * pf[1*4 + 0];

	pfres2[0*4 + 2] =   pf[3*4 + 1] * fd2 - pf[3*4 + 2] * f1 + pf[3*4 + 3] * fd1;
	pfres2[0*4 + 3] = -(pf[2*4 + 1] * fd2 - pf[2*4 + 2] * f1 + pf[2*4 + 3] * fd1);

	pfres2[1*4 + 2] = -(pf[3*4 + 0] * fd2 - pf[3*4 + 2] * f2 + pf[3*4 + 3] * f3);
	pfres2[1*4 + 3] =   pf[2*4 + 0] * fd2 - pf[2*4 + 2] * f2 + pf[2*4 + 3] * f3;

	pfres2[2*4 + 2] =   pf[3*4 + 0] * f1 -	pf[3*4 + 1] * f2 + pf[3*4 + 3] * fd0;
	pfres2[2*4 + 3] = -(pf[2*4 + 0] * f1 -	pf[2*4 + 1] * f2 + pf[2*4 + 3] * fd0);

	pfres2[3*4 + 2] = -(pf[3*4 + 0] * fd1 - pf[3*4 + 1] * f3 + pf[3*4 + 2] * fd0);
	pfres2[3*4 + 3] =   pf[2*4 + 0] * fd1 - pf[2*4 + 1] * f3 + pf[2*4 + 2] * fd0;

	T fdet = pf[0*4 + 3] * pfres2[3*4 + 0] + pf[1*4 + 3] * pfres2[3*4 + 1] +
			pf[2*4 + 3] * pfres2[3*4 + 2] + pf[3*4 + 3] * pfres2[3*4 + 3];

	if( fabs(fdet) < 1e-6) return NULL;

	fdet = 1 / fdet;
	//if( pfdet != NULL ) *pfdet = fdet;

	if( pfres2 == pfres ) {
		mult(pfres, fdet, 16);
		return pfres;
	}

	int i = 0;
	while(i < 16) {
		pfres[i] = pfres2[i] * fdet;
		++i;
	}

	return pfres;
}

inline dReal* inv4(const dReal* pf, dReal* pfres) { return _inv4<dReal>(pf, pfres); }

template <class T> inline T* _transpose3(const T* pf, T* pfres)
{
	assert( pf != NULL && pfres != NULL );

	if( pf == pfres ) {
		rswap(pfres[1], pfres[3]);
		rswap(pfres[2], pfres[6]);
		rswap(pfres[5], pfres[7]);
		return pfres;
	}

	pfres[0] = pf[0];	pfres[1] = pf[3];	pfres[2] = pf[6];
	pfres[3] = pf[1];	pfres[4] = pf[4];	pfres[5] = pf[7];
	pfres[6] = pf[2];	pfres[7] = pf[5];	pfres[8] = pf[8];

	return pfres;
}

inline dReal* transpose3(const dReal* pf, dReal* pfres) { return _transpose3(pf, pfres); }
inline double* transpose3(const double* pf, double* pfres) { return _transpose3(pf, pfres); }

template <class T> inline T* _transpose4(const T* pf, T* pfres)
{
	assert( pf != NULL && pfres != NULL );

	if( pf == pfres ) {
		rswap(pfres[1], pfres[4]);
		rswap(pfres[2], pfres[8]);
		rswap(pfres[3], pfres[12]);
		rswap(pfres[6], pfres[9]);
		rswap(pfres[7], pfres[13]);
		rswap(pfres[11], pfres[15]);
		return pfres;
	}

	pfres[0] = pf[0];	pfres[1] = pf[4];	pfres[2] = pf[8];		pfres[3] = pf[12];
	pfres[4] = pf[1];	pfres[5] = pf[5];	pfres[6] = pf[9];		pfres[7] = pf[13];
	pfres[8] = pf[2];	pfres[9] = pf[6];	pfres[10] = pf[10];		pfres[11] = pf[14];
	pfres[12] = pf[3];	pfres[13] = pf[7];	pfres[14] = pf[11];		pfres[15] = pf[15];
	return pfres;
}

inline dReal* transpose4(const dReal* pf, dReal* pfres) { return _transpose4(pf, pfres); }
inline double* transpose4(const double* pf, double* pfres) { return _transpose4(pf, pfres); }

inline dReal dot2(const dReal* pf1, const dReal* pf2)
{
	assert( pf1 != NULL && pf2 != NULL );
	return pf1[0]*pf2[0] + pf1[1]*pf2[1];
}

inline dReal dot3(const dReal* pf1, const dReal* pf2)
{
	assert( pf1 != NULL && pf2 != NULL );
	return pf1[0]*pf2[0] + pf1[1]*pf2[1] + pf1[2]*pf2[2];
}

inline dReal dot4(const dReal* pf1, const dReal* pf2)
{
	assert( pf1 != NULL && pf2 != NULL );
	return pf1[0]*pf2[0] + pf1[1]*pf2[1] + pf1[2]*pf2[2] + pf1[3] * pf2[3];
}

inline dReal lengthsqr2(const dReal* pf)
{
	assert( pf != NULL );
	return pf[0] * pf[0] + pf[1] * pf[1];
}

inline dReal lengthsqr3(const dReal* pf)
{
	assert( pf != NULL );
	return pf[0] * pf[0] + pf[1] * pf[1] + pf[2] * pf[2];
}

inline dReal lengthsqr4(const dReal* pf)
{
	assert( pf != NULL );
	return pf[0] * pf[0] + pf[1] * pf[1] + pf[2] * pf[2] + pf[3] * pf[3];
}

inline dReal* normalize2(dReal* pfout, const dReal* pf)
{
	assert(pf != NULL);

	dReal f = pf[0]*pf[0] + pf[1]*pf[1];
	f = 1.0f / sqrtf(f);
	pfout[0] = pf[0] * f;
	pfout[1] = pf[1] * f;

	return pfout;
}

inline dReal* normalize3(dReal* pfout, const dReal* pf)
{
	assert(pf != NULL);

	dReal f = pf[0]*pf[0] + pf[1]*pf[1] + pf[2]*pf[2];

	f = 1.0f / sqrtf(f);
	pfout[0] = pf[0] * f;
	pfout[1] = pf[1] * f;
	pfout[2] = pf[2] * f;

	return pfout;
}

inline dReal* normalize4(dReal* pfout, const dReal* pf)
{
	assert(pf != NULL);

	dReal f = pf[0]*pf[0] + pf[1]*pf[1] + pf[2]*pf[2] + pf[3]*pf[3];

	f = 1.0f / sqrtf(f);
	pfout[0] = pf[0] * f;
	pfout[1] = pf[1] * f;
	pfout[2] = pf[2] * f;
	pfout[3] = pf[3] * f;

	return pfout;
}

inline dReal* cross3(dReal* pfout, const dReal* pf1, const dReal* pf2)
{
	assert( pfout != NULL && pf1 != NULL && pf2 != NULL );

	dReal temp[3];
	temp[0] = pf1[1] * pf2[2] - pf1[2] * pf2[1];
	temp[1] = pf1[2] * pf2[0] - pf1[0] * pf2[2];
	temp[2] = pf1[0] * pf2[1] - pf1[1] * pf2[0];

	pfout[0] = temp[0]; pfout[1] = temp[1]; pfout[2] = temp[2];
	return pfout;
}

template <class T> inline void mult(T* pf, T fa, int r)
{
	assert( pf != NULL );

	while(r > 0) {
		--r;
		pf[r] *= fa;
	}
}

template <class T, class S, class R>
inline T* mult(T* pf1, R* pf2, int r1, int c1, int c2, S* pfres, bool badd)
{
	assert( pf1 != NULL && pf2 != NULL && pfres != NULL);
	int j, k;

	if( !badd ) memset(pfres, 0, sizeof(S) * r1 * c2);

	while(r1 > 0) {
		--r1;

		j = 0;
		while(j < c2) {
			k = 0;
			while(k < c1) {
				pfres[j] += pf1[k] * pf2[k*c2 + j];
				++k;
			}
			++j;
		}

		pf1 += c1;
		pfres += c2;
	}

	return pfres;
}

template <class T, class S, class R>
inline T* multtrans(T* pf1, R* pf2, int r1, int c1, int c2, S* pfres, bool badd)
{
	assert( pf1 != NULL && pf2 != NULL && pfres != NULL);
	int i, j, k;

	if( !badd ) memset(pfres, 0, sizeof(S) * c1 * c2);

	i = 0;
	while(i < c1) {

		j = 0;
		while(j < c2) {

			k = 0;
			while(k < r1) {
				pfres[j] += pf1[k*c1] * pf2[k*c2 + j];
				++k;
			}
			++j;
		}

		pfres += c2;
		++pf1;

		++i;
	}

	return pfres;
}

template <class T, class S, class R>
inline T* multtrans_to2(T* pf1, R* pf2, int r1, int c1, int r2, S* pfres, bool badd)
{
	assert( pf1 != NULL && pf2 != NULL && pfres != NULL);
	int j, k;

	if( !badd ) memset(pfres, 0, sizeof(S) * r1 * r2);

	while(r1 > 0) {
		--r1;

		j = 0;
		while(j < r2) {
			k = 0;
			while(k < c1) {
				pfres[j] += pf1[k] * pf2[j*c1 + k];
				++k;
			}
			++j;
		}

		pf1 += c1;
		pfres += r2;
	}

	return pfres;
}

template <class T> inline T* multto1(T* pf1, T* pf2, int r, int c, T* pftemp)
{
	assert( pf1 != NULL && pf2 != NULL );

	int j, k;
	bool bdel = false;
	
	if( pftemp == NULL ) {
		pftemp = new T[c];
		bdel = true;
	}

	while(r > 0) {
		--r;

		j = 0;
		while(j < c) {
			
			pftemp[j] = 0.0;

			k = 0;
			while(k < c) {
				pftemp[j] += pf1[k] * pf2[k*c + j];
				++k;
			}
			++j;
		}

		memcpy(pf1, pftemp, c * sizeof(T));
		pf1 += c;
	}

	if( bdel ) delete[] pftemp;

	return pf1;
}

template <class T, class S> inline T* multto2(T* pf1, S* pf2, int r2, int c2, S* pftemp)
{
	assert( pf1 != NULL && pf2 != NULL );

	int i, j, k;
	bool bdel = false;
	
	if( pftemp == NULL ) {
		pftemp = new S[r2];
		bdel = true;
	}

	// do columns first
	j = 0;
	while(j < c2) {
		i = 0;
		while(i < r2) {
			
			pftemp[i] = 0.0;

			k = 0;
			while(k < r2) {
				pftemp[i] += pf1[i*r2 + k] * pf2[k*c2 + j];
				++k;
			}
			++i;
		}

		i = 0;
		while(i < r2) {
			*(pf2+i*c2+j) = pftemp[i];
			++i;
		}

		++j;
	}

	if( bdel ) delete[] pftemp;

	return pf1;
}

template <class T> inline void add(T* pf1, T* pf2, int r)
{
	assert( pf1 != NULL && pf2 != NULL);

	while(r > 0) {
		--r;
		pf1[r] += pf2[r];
	}
}

template <class T> inline void sub(T* pf1, T* pf2, int r)
{
	assert( pf1 != NULL && pf2 != NULL);

	while(r > 0) {
		--r;
		pf1[r] -= pf2[r];
	}
}

template <class T> inline T normsqr(T* pf1, int r)
{
	assert( pf1 != NULL );

	T d = 0.0;
	while(r > 0) {
		--r;
		d += pf1[r] * pf1[r];
	}

	return d;
}

template <class T> inline T lengthsqr(T* pf1, T* pf2, int length)
{
	T d = 0;
	while(length > 0) {
		--length;
		d += sqr(pf1[length] - pf2[length]);
	}

	return d;
}

template <class T> inline T dot(T* pf1, T* pf2, int length)
{
	T d = 0;
	while(length > 0) {
		--length;
		d += pf1[length] * pf2[length];
	}

	return d;
}

template <class T> inline T sum(T* pf, int length)
{
	T d = 0;
	while(length > 0) {
		--length;
		d += pf[length];
	}

	return d;
}

template <class T> inline bool inv2(T* pf, T* pfres)
{
	T fdet = pf[0] * pf[3] - pf[1] * pf[2];

	if( fabs(fdet) < 1e-16 ) return false;

	fdet = 1 / fdet;
	//if( pfdet != NULL ) *pfdet = fdet;

	if( pfres != pf ) {
		pfres[0] = fdet * pf[3];		pfres[1] = -fdet * pf[1];
		pfres[2] = -fdet * pf[2];		pfres[3] = fdet * pf[0];
		return true;
	}

	dReal ftemp = pf[0];
	pfres[0] = pf[3] * fdet;
	pfres[1] *= -fdet;
	pfres[2] *= -fdet;
	pfres[3] = ftemp * pf[0];

	return true;
}

#endif
