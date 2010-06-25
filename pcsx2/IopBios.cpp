/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2010  PCSX2 Dev Team
 *
 *  PCSX2 is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU Lesser General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  PCSX2 is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with PCSX2.
 *  If not, see <http://www.gnu.org/licenses/>.
 */


#include "PrecompiledHeader.h"
#include "IopCommon.h"
#include "R5900.h" // for g_GameStarted

#include <ctype.h>
#include <string.h>

#ifndef O_BINARY
#define O_BINARY 0
#endif

namespace R3000A {

#define v0 (psxRegs.GPR.n.v0)
#define a0 (psxRegs.GPR.n.a0)
#define a1 (psxRegs.GPR.n.a1)
#define a2 (psxRegs.GPR.n.a2)
#define a3 (psxRegs.GPR.n.a3)
#define sp (psxRegs.GPR.n.sp)
#define ra (psxRegs.GPR.n.ra)
#define pc (psxRegs.pc)

#define Ra0 (iopVirtMemR<char>(a0))
#define Ra1 (iopVirtMemR<char>(a1))
#define Ra2 (iopVirtMemR<char>(a2))
#define Ra3 (iopVirtMemR<char>(a3))

// TODO: sandbox option, other permissions
class HostFile : public IOManFile
{
public:
	int fd;

	HostFile(int hostfd)
	{
		fd = hostfd;
	}

	static __forceinline int translate_error(int err)
	{
		if (err >= 0)
			return err;

		switch(err)
		{
			case -ENOENT:
				return -IOP_ENOENT;
			case -EACCES:
				return -IOP_EACCES;
			case -EISDIR:
				return -IOP_EISDIR;
			case -EIO:
			default:
				return -IOP_EIO;
		}
	}

	static int open(IOManFile **file, const char *name, s32 flags, u16 mode)
	{
		const char *path = strchr(name, ':') + 1;

		// host: actually DOES let you write!
		//if (flags != IOP_O_RDONLY)
		//	return -IOP_EROFS;

		// WIP code. Works well on win32, not so sure on unixes
		// TODO: get rid of dependency on CWD/PWD
#if USE_HOST_REWRITE
		// we want filenames to be relative to pcs2dir / host

		static char pathMod[1024];

		// partial "rooting",
		// it will NOT avoid a path like "../../x" from escaping the pcsx2 folder!

#ifdef _WIN32
		const char *_local_root = "/usr/local/";
		if(strncmp(path,_local_root,strlen(_local_root))==0)
		{
			strcpy(pathMod,"host/");
			strcat(pathMod,path+strlen(_local_root));
		}
		else
#endif
		if((path[0] == '/') || (path[0] == '\\') || (isalpha(path[0]) && (path[1] == ':'))) // absolute NATIVE path (X:\blah)
		{
			// TODO: allow some way to use native paths in non-windows platforms
			// maybe hack it so linux prefixes the path with "X:"? ;P
			// or have all platforms use a common prefix for native paths
			strcpy(pathMod,path);
		}
		else // relative paths
		{
			// this assumes the PWD/CWD points to pcsx2's data folder,
			// that is, eitehr the same folder as pcsx2.exe or somethign like
			// c:\users\appdata\roaming\pcsx2, documents\pcsx2, $HOME\pcsx2, or similar

			strcpy(pathMod,"host/");
			strcat(pathMod,path);
		}
#else
		const char* pathMod = path;
#endif

		int native_flags = O_BINARY; // necessary in Windows.

		switch(flags&IOP_O_RDWR)
		{
		case IOP_O_RDONLY:	native_flags |= O_RDONLY;	break;
		case IOP_O_WRONLY:	native_flags |= O_WRONLY; break;
		case IOP_O_RDWR:	native_flags |= O_RDWR;	break;
		}

		if(flags&IOP_O_APPEND)	native_flags |= O_APPEND;
		if(flags&IOP_O_CREAT)	native_flags |= O_CREAT;
		if(flags&IOP_O_TRUNC)	native_flags |= O_TRUNC;

		int hostfd = ::open(pathMod, native_flags);
		if (hostfd < 0)
			return translate_error(hostfd);

		*file = new HostFile(hostfd);
		if (!*file)
			return -IOP_ENOMEM;

		return 0;
	}

	virtual void close()
	{
		::close(fd);
		delete this;
	}

	virtual int lseek(s32 offset, s32 whence)
	{
		int err;

		switch (whence)
		{
			case IOP_SEEK_SET:
				err = ::lseek(fd, offset, SEEK_SET);
				break;
			case IOP_SEEK_CUR:
				err = ::lseek(fd, offset, SEEK_CUR);
				break;
			case IOP_SEEK_END:
				err = ::lseek(fd, offset, SEEK_END);
				break;
			default:
				return -IOP_EIO;
		}

		return translate_error(err);
	}

	virtual int read(void *buf, u32 count)
	{
		return translate_error(::read(fd, buf, count));
	}

	virtual int write(void *buf, u32 count)
	{
		return translate_error(::write(fd, buf, count));
	}
};

namespace ioman {
	const int firstfd = 0x100;
	const int maxfds = 0x100;
	int openfds = 0;

	int freefdcount()
	{
		return maxfds - openfds;
	}

	struct filedesc
	{
		enum {
			FILE_FREE,
			FILE_FILE,
			FILE_DIR,
		} type;
		union {
			IOManFile *file;
			IOManDir *dir;
		};

		operator bool() const { return type != FILE_FREE; }
		operator IOManFile*() const { return type == FILE_FILE ? file : NULL; }
		operator IOManDir*() const { return type == FILE_DIR ? dir : NULL; }
		void operator=(IOManFile *f) { type = FILE_FILE; file = f; openfds++; }
		void operator=(IOManDir *d) { type = FILE_DIR; dir = d; openfds++; }

		void close()
		{
			if (type == FILE_FREE)
				return;

			switch (type)
			{
				case FILE_FILE:
					file->close();
					file = NULL;
					break;
				case FILE_DIR:
					dir->close();
					dir = NULL;
					break;
			}

			type = FILE_FREE;
			openfds--;
		}
	};

	filedesc fds[maxfds];

	template<typename T>
	T* getfd(int fd)
	{
		fd -= firstfd;

		if (fd < 0 || fd >= maxfds)
			return NULL;

		return fds[fd];
	}

	template <typename T>
	int allocfd(T *obj)
	{
		for (int i = 0; i < maxfds; i++)
		{
			if (!fds[i])
			{
				fds[i] = obj;
				return firstfd + i;
			}
		}

		obj->close();
		return -IOP_EMFILE;
	}

	void freefd(int fd)
	{
		fd -= firstfd;

		if (fd < 0 || fd >= maxfds)
			return;

		fds[fd].close();
	}

	void reset()
	{
		for (int i = 0; i < maxfds; i++)
			fds[i].close();
	}

	int open_HLE()
	{
		IOManFile *file = NULL;
		const char *name = Ra0;
		s32 flags = a1;
		u16 mode = a2;

		if ((!g_GameStarted || EmuConfig.HostFs)
			&& !strncmp(name, "host", 4) && name[4 + strspn(name + 4, "0123456789")] == ':')
		{
			if (!freefdcount())
			{
				v0 = -IOP_EMFILE;
				pc = ra;
				return 1;
			}

			int err = HostFile::open(&file, name, flags, mode);

			if (err != 0 || !file)
			{
				if (err == 0) // ???
					err = -IOP_EIO;
				if (file) // ??????
					file->close();
				v0 = err;
			}
			else
			{
				v0 = allocfd(file);
				if ((s32)v0 < 0)
					file->close();
			}

			pc = ra;
			return 1;
		}

		return 0;
	}

	int close_HLE()
	{
		s32 fd = a0;

		if (getfd<IOManFile>(fd))
		{
			freefd(fd);
			v0 = 0;
			pc = ra;
			return 1;
		}

		return 0;
	}

	int lseek_HLE()
	{
		s32 fd = a0;
		s32 offset = a1;
		s32 whence = a2;

		if (IOManFile *file = getfd<IOManFile>(fd))
		{
			v0 = file->lseek(offset, whence);
			pc = ra;
			return 1;
		}

		return 0;
	}

	int read_HLE()
	{
		s32 fd = a0;
		u32 buf = a1;
		u32 count = a2;

		if (IOManFile *file = getfd<IOManFile>(fd))
		{
			if (!iopVirtMemR<void>(buf))
				return 0;

			v0 = file->read(iopVirtMemW<void>(buf), count);
			pc = ra;
			return 1;
		}

		return 0;
	}

	int write_HLE()
	{
		s32 fd = a0;
		u32 buf = a1;
		u32 count = a2;

#ifdef PCSX2_DEVBUILD
		if (fd == 1) // stdout
		{
			Console.Write(ConColor_IOP, L"%s", ShiftJIS_ConvertString(Ra1, a2).c_str());
			pc = ra;
			v0 = a2;
		}
		else
#endif
		if (IOManFile *file = getfd<IOManFile>(fd))
		{
			if (!iopVirtMemR<void>(buf))
				return 0;

			v0 = file->write(iopVirtMemW<void>(buf), count);
			pc = ra;
			return 1;
		}

		return 0;
	}
}

namespace sysmem {
	int Kprintf_HLE()
	{
		char tmp[1024], tmp2[1024];
		char *ptmp = tmp;
		int n=1, i=0, j = 0;

		iopMemWrite32(sp, a0);
		iopMemWrite32(sp + 4, a1);
		iopMemWrite32(sp + 8, a2);
		iopMemWrite32(sp + 12, a3);

		while (Ra0[i])
		{
			switch (Ra0[i])
			{
				case '%':
					j = 0;
					tmp2[j++] = '%';
_start:
					switch (Ra0[++i])
					{
						case '.':
						case 'l':
							tmp2[j++] = Ra0[i];
							goto _start;
						default:
							if (Ra0[i] >= '0' && Ra0[i] <= '9')
							{
								tmp2[j++] = Ra0[i];
								goto _start;
							}
							break;
					}

					tmp2[j++] = Ra0[i];
					tmp2[j] = 0;

					switch (Ra0[i])
					{
						case 'f': case 'F':
							ptmp+= sprintf(ptmp, tmp2, (float)iopMemRead32(sp + n * 4));
							n++;
							break;

						case 'a': case 'A':
						case 'e': case 'E':
						case 'g': case 'G':
							ptmp+= sprintf(ptmp, tmp2, (double)iopMemRead32(sp + n * 4));
							n++;
							break;

						case 'p':
						case 'i':
						case 'd': case 'D':
						case 'o': case 'O':
						case 'x': case 'X':
							ptmp+= sprintf(ptmp, tmp2, (u32)iopMemRead32(sp + n * 4));
							n++;
							break;

						case 'c':
							ptmp+= sprintf(ptmp, tmp2, (u8)iopMemRead32(sp + n * 4));
							n++;
							break;

						case 's':
							ptmp+= sprintf(ptmp, tmp2, iopVirtMemR<char>(iopMemRead32(sp + n * 4)));
							n++;
							break;

						case '%':
							*ptmp++ = Ra0[i];
							break;

						default:
							break;
					}
					i++;
					break;

				default:
					*ptmp++ = Ra0[i++];
					break;
			}
		}
		*ptmp = 0;

		// Use "%s" even though it seems indirect: this avoids chaos if/when the IOP decides
		// to feed us strings that contain percentages or other printf formatting control chars.
		Console.Write( ConColor_IOP, L"%s", ShiftJIS_ConvertString(tmp).c_str(), 1023 );
		
		pc = ra;
		return 1;
	}
}

namespace loadcore {
	void RegisterLibraryEntries_DEBUG()
	{
		DbgCon.WriteLn(Color_Gray, "RegisterLibraryEntries: %8.8s", iopVirtMemR<char>(a0 + 12));
	}
}

namespace intrman {
	static const char* intrname[] = {
		"INT_VBLANK",   "INT_GM",       "INT_CDROM",   "INT_DMA",		//00
		"INT_RTC0",     "INT_RTC1",     "INT_RTC2",    "INT_SIO0",		//04
		"INT_SIO1",     "INT_SPU",      "INT_PIO",     "INT_EVBLANK",	//08
		"INT_DVD",      "INT_PCMCIA",   "INT_RTC3",    "INT_RTC4",		//0C
		"INT_RTC5",     "INT_SIO2",     "INT_HTR0",    "INT_HTR1",		//10
		"INT_HTR2",     "INT_HTR3",     "INT_USB",     "INT_EXTR",		//14
		"INT_FWRE",     "INT_FDMA",     "INT_1A",      "INT_1B",		//18
		"INT_1C",       "INT_1D",       "INT_1E",      "INT_1F",		//1C
		"INT_dmaMDECi", "INT_dmaMDECo", "INT_dmaGPU",  "INT_dmaCD",		//20
		"INT_dmaSPU",   "INT_dmaPIO",   "INT_dmaOTC",  "INT_dmaBERR",	//24
		"INT_dmaSPU2",  "INT_dma8",     "INT_dmaSIF0", "INT_dmaSIF1",	//28
		"INT_dmaSIO2i", "INT_dmaSIO2o", "INT_2E",      "INT_2F",		//2C
		"INT_30",       "INT_31",       "INT_32",      "INT_33",		//30
		"INT_34",       "INT_35",       "INT_36",      "INT_37",		//34
		"INT_38",       "INT_39",       "INT_3A",      "INT_3B",		//38
		"INT_3C",       "INT_3D",       "INT_3E",      "INT_3F",		//3C
		"INT_MAX"														//40
	};

	void RegisterIntrHandler_DEBUG()
	{
		DbgCon.WriteLn(Color_Gray, "RegisterIntrHandler: intr %s, handler %x", intrname[a0], a2);
	}
}

namespace sifcmd {
	void sceSifRegisterRpc_DEBUG()
	{
		DbgCon.WriteLn( Color_Gray, "sifcmd sceSifRegisterRpc: rpc_id %x", a1);
	}
}

const char* irxImportLibname(u32 entrypc)
{
	u32 i;

	i = entrypc;
	while (iopMemRead32(i -= 4) != 0x41e00000) // documented magic number
		;

	return iopVirtMemR<char>(i + 12);
}

const char* irxImportFuncname(const char libname[8], u16 index)
{
	#include "IopModuleNames.cpp"

	switch (index) {
		case 0: return "start";
		// case 1: reinit?
		case 2: return "shutdown";
		// case 3: ???
	}

	return 0;
}

#define MODULE(n) if (!strncmp(libname, #n, 8)) { using namespace n; switch (index) {
#define END_MODULE }}
#define EXPORT_D(i, n) case (i): return n ## _DEBUG;
#define EXPORT_H(i, n) case (i): return n ## _HLE;

irxHLE irxImportHLE(const char libname[8], u16 index)
{
#ifdef PCSX2_DEVBUILD
	// debugging output
	MODULE(sysmem)
		EXPORT_H( 14, Kprintf)
	END_MODULE
#endif
	MODULE(ioman)
		EXPORT_H(  4, open)
		EXPORT_H(  5, close)
		EXPORT_H(  6, read)
		EXPORT_H(  7, write)
		EXPORT_H(  8, lseek)
	END_MODULE

	return 0;
}

irxDEBUG irxImportDebug(const char libname[8], u16 index)
{
	MODULE(loadcore)
		EXPORT_D(  6, RegisterLibraryEntries)
	END_MODULE
	MODULE(intrman)
		EXPORT_D(  4, RegisterIntrHandler)
	END_MODULE
	MODULE(sifcmd)
		EXPORT_D( 17, sceSifRegisterRpc)
	END_MODULE

	return 0;
}

#undef MODULE
#undef END_MODULE
#undef EXPORT_D
#undef EXPORT_H

void __fastcall irxImportLog(const char libname[8], u16 index, const char *funcname)
{
	PSXBIOS_LOG("%8.8s.%03d: %s (%x, %x, %x, %x)",
		libname, index, funcname ? funcname : "unknown",
		a0, a1, a2, a3);
}

int __fastcall irxImportExec(const char libname[8], u16 index)
{
	const char *funcname = irxImportFuncname(libname, index);
	irxHLE hle = irxImportHLE(libname, index);
	irxDEBUG debug = irxImportDebug(libname, index);

	irxImportLog(libname, index, funcname);

	if (debug)
		debug();
	
	if (hle)
		return hle();
	else
		return 0;
}

}	// end namespace R3000A
