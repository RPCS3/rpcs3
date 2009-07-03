/* SPU2-X, A plugin for Emulating the Sound Processing Unit of the Playstation 2
 * Developed and maintained by the Pcsx2 Development Team.
 * 
 * Original portions from SPU2ghz are (c) 2008 by David Quintana [gigaherz]
 *
 * SPU2-X is free software: you can redistribute it and/or modify it under the terms
 * of the GNU Lesser General Public License as published by the Free Software Found-
 * ation, either version 3 of the License, or (at your option) any later version.
 *
 * SPU2-X is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with SPU2-X.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "Lowpass.h"
#include <math.h>
#include <float.h>

LPF_data::LPF_data( double freq, double srate )
{
	double omega = 2.0 * freq / srate;
	static const double g = 1.0; 

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
	        
	coef[0] = p;
	coef[1] = 4.0*p;
	coef[2] = 6.0*p;
	coef[3] = 4.0*p;
	coef[4] = p;
	coef[5] = -a1*a0;
	coef[6] = -a2*a0;
	coef[7] = -a3*a0;
	coef[8] = -a4*a0;
}

// Processes a single sample into the LPF.
double LPF_data::sample( double inval )
{
	const double out = (coef[0]*inval) + d[0];
	d[0] = (coef[1]*inval) + (coef[5]*out) + d[1];
	d[1] = (coef[2]*inval) + (coef[6]*out) + d[2];
	d[2] = (coef[3]*inval) + (coef[7]*out) + d[3];
	d[3] = (coef[4]*inval) + (coef[8]*out);	

	return out;
}
