//GiGaHeRz's SPU2 Driver
//Copyright (c) 2003-2008, David Quintana <gigaherz@gmail.com>
//
//This library is free software; you can redistribute it and/or
//modify it under the terms of the GNU Lesser General Public
//License as published by the Free Software Foundation; either
//version 2.1 of the License, or (at your option) any later version.
//
//This library is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//Lesser General Public License for more details.
//
//You should have received a copy of the GNU Lesser General Public
//License along with this library; if not, write to the Free Software
//Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
#include "lowpass.h"
#include <math.h>
#include <float.h>

void LPF_init(LPF_data*lpf,double freq, double srate)
{
	double omega = 2*freq/srate;
	double g = 1.0; 

	// calculating coefficients:

	double k,p,q,a;
	double a0,a1,a2,a3,a4;

	k=(4.0*g-3.0)/(g+1.0);
	p=1.0-0.25*k;p*=p;

	// LP:
	a=1.0/(tan(0.5*omega)*(1.0+p));
	p=1.0+a;
	q=1.0-a;
	        
	a0=1.0/(k+p*p*p*p);
	a1=4.0*(k+p*p*p*q);
	a2=6.0*(k+p*p*q*q);
	a3=4.0*(k+p*q*q*q);
	a4=    (k+q*q*q*q);
	p=a0*(k+1.0);
	        
	lpf->coef[0]=p;
	lpf->coef[1]=4.0*p;
	lpf->coef[2]=6.0*p;
	lpf->coef[3]=4.0*p;
	lpf->coef[4]=p;
	lpf->coef[5]=-a1*a0;
	lpf->coef[6]=-a2*a0;
	lpf->coef[7]=-a3*a0;
	lpf->coef[8]=-a4*a0;
}
double LPF(LPF_data* lpf, double in)
{
// per sample:

	double out=lpf->coef[0]*in+lpf->d[0];
	lpf->d[0] =lpf->coef[1]*in+lpf->coef[5]*out+lpf->d[1];
	lpf->d[1] =lpf->coef[2]*in+lpf->coef[6]*out+lpf->d[2];
	lpf->d[2] =lpf->coef[3]*in+lpf->coef[7]*out+lpf->d[3];
	lpf->d[3] =lpf->coef[4]*in+lpf->coef[8]*out;	
return out;
}
