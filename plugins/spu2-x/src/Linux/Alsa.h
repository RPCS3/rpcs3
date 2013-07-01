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

 #ifndef __LINUX_H__
 #define __LINUX_H__

#include <assert.h>
#include <stdlib.h>

#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/soundcard.h>
#include <unistd.h>

#define SAMPLE_RATE 48000L

// Pull in from Alsa.cpp
extern int AlsaSetupSound();
extern void AlsaRemoveSound();
extern int AlsaSoundGetBytesBuffered();
extern void AlsaSoundFeedVoiceData(unsigned char* pSound,long lBytes);

extern int SetupSound();
extern void RemoveSound();
extern int SoundGetBytesBuffered();
extern void SoundFeedVoiceData(unsigned char* pSound,long lBytes);

#endif // __LINUX_H__
