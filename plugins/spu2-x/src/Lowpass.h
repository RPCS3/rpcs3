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

#pragma once

template< typename FloatType >
struct LowPassFilter
{
	FloatType	coef[9];
	FloatType	d[4];

	LowPassFilter( FloatType freq, FloatType srate );
	FloatType sample( FloatType inval );
};

struct LowPassFilter32
{
	LowPassFilter<float> impl_lpf;

	LowPassFilter32( float freq, float srate );
	float sample( float inval );
};

struct LowPassFilter64
{
	LowPassFilter<double> impl_lpf;

	LowPassFilter64( double freq, double srate );
	double sample( double inval );
};
