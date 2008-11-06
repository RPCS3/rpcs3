//[module]	SSBUSC
//[processor]	IOP
//[type]	ELF-IRX
//[name]	ssbus_service
//[version]	0x101
//[memory map]	0xBF801000,0xBF801004,0xBF801008,0xBF80100C,		(r/w)
//		MDECin(T2),8(T2)     ,MDECin(T1),MDECout(T1),
//
//		0xBF801010,0xBF801014,0xBF801018,0xBF80101C,0xBF801020	(r/w)
//		GPU(SIF2)(T1),SPU(T1)spu_delay,PIO(T1),8(T1),SSBUSCconfig
//
//		0xBF801400,0xBF801404,0xBF801408,0xBF80140C,		(r/w)
//		MDECout(T2),SPU(T2)  ,PIO(T2)   ,SIF0(T2),
//
//		0xBF801410,0xBF801414,0xBF801418,0xBF80141C,0xBF801420	(r/w)
//		SIO2in(T2),SIF0(T1)  ,SIF1(T1)  ,SIO2in(T1),SIO2out(T1)
//
//[handlers]	-
//[entry point]	start, export_stub
//[made by]	[RO]man (roman_ps2dev@hotmail.com)

#include "kloadcore.h"
#include "kintrman.h"

int memmap_table[]={
	0xBF801008, 0xBF80100C, 0xBF801010, 	     0, 0xBF801014,//spu_delay
	0xBF801018,	     0,		 0, 0xBF80101C, 0xBF801414,
	0xBF801418, 0xBF80141C, 0xBF801420
};

int memmap_table_2[]={
	0xBF801000, 0xBF801400,		 0,	     0, 0xBF801404,
	0xBF801408,	     0,		 0, 0xBF801004, 0xBF80140C,
		 0, 0xBF801410,		 0
};

int _start();

///////////////////////////////////////////////////////////////////////
int return_0(){
	return 0;
}

///////////////////////////////////////////////////////////////////////
int SSBUSsetTable1(int code, int value){	//set
    int *v;
	if ((code < 13) && (v=(int*)memmap_table[code]))
		return *v=value;
	return -1;
}

///////////////////////////////////////////////////////////////////////
int SSBUSgetTable1(int code){			//get
    int *v;
	if ((code < 13) && (v=(int*)memmap_table[code]))
		return *v;
	return -1;
}

///////////////////////////////////////////////////////////////////////
int SSBUSsetTable2(int code, int value){	//set
    int *v;
	if ((code < 13) && (v=(int*)memmap_table_2[code]))
		return *v=value;
	return -1;
}

///////////////////////////////////////////////////////////////////////
int SSBUSgetTable2(int code){			//get
    int *v;
	if ((code < 13) && (v=(int*)memmap_table_2[code]))
		return *v;
	return -1;
}

///////////////////////////////////////////////////////////////////////
int SSBUSsetCOM_DELAY_1st(int a){			//set lowest nibble
	return	 *(int*)(0xBF801020) = 
		(*(int*)(0xBF801020) & 0xFFFFFFF0) |
		(             a      &        0xF);
}

///////////////////////////////////////////////////////////////////////
int SSBUSgetCOM_DELAY_1st(){				//get lowest nibble
	return   *(int*)(0xBF801020) &        0xF;
}

///////////////////////////////////////////////////////////////////////
int SSBUSsetCOM_DELAY_2nd(int a){			//set
	return	 *(int*)(0xBF801020) = 
		(*(int*)(0xBF801020) & 0xFFFFFF0F) |
		(         (a << 4)   &       0xF0);
}

///////////////////////////////////////////////////////////////////////
int SSBUSgetCOM_DELAY_2nd(){				//get
	return *(unsigned char*)(0xBF801020) >> 4;
}

///////////////////////////////////////////////////////////////////////
int SSBUSsetCOM_DELAY_3rd(int a){			//set
	return	 *(int*)(0xBF801020) = 
		(*(int*)(0xBF801020) & 0xFFFFF0FF) |
		(         (a << 8)   &      0xF00);
}

///////////////////////////////////////////////////////////////////////
int SSBUSgetCOM_DELAY_3rd(){				//get
	return  (*(int*)(0xBF801020) >> 8) & 0xF;
}

///////////////////////////////////////////////////////////////////////
int SSBUSsetCOM_DELAY_4th(int a){			//set
	return	 *(int*)(0xBF801020) = 
		(*(int*)(0xBF801020) & 0xFFFF0FFF) |
		(        (a << 12)   &     0xF000);
}

///////////////////////////////////////////////////////////////////////
int SSBUSgetCOM_DELAY_4th(){				//get
	return  (*(int*)(0xBF801020) >> 12) & 0xF;
}

///////////////////////////////////////////////////////////////////////
int SSBUSsetCOM_DELAY(int a){				//set
	return   *(int*)(0xBF801020) = a;
}

///////////////////////////////////////////////////////////////////////
int SSBUSgetCOM_DELAY(){				//get
	return   *(int*)(0xBF801020);
}

///////////////////////////////////////////////////////////////////////
void retonly(){}

//////////////////////////////entrypoint///////////////////////////////
struct export export_stub={
	0x41C00000,
	0,
	VER(1, 1),	// 1.1 => 0x101
	0,
	"ssbusc",
	(func)_start,	// entrypoint
	(func)retonly,
	(func)return_0,
	(func)retonly,
	(func)SSBUSsetTable1,
	(func)SSBUSgetTable1,
	(func)SSBUSsetTable2,
	(func)SSBUSgetTable2,
	(func)SSBUSsetCOM_DELAY_1st,
	(func)SSBUSgetCOM_DELAY_1st,
	(func)SSBUSsetCOM_DELAY_2nd,
	(func)SSBUSgetCOM_DELAY_2nd,
	(func)SSBUSsetCOM_DELAY_3rd,
	(func)SSBUSgetCOM_DELAY_3rd,
	(func)SSBUSsetCOM_DELAY_4th,
	(func)SSBUSgetCOM_DELAY_4th,
	(func)SSBUSsetCOM_DELAY,
	(func)SSBUSgetCOM_DELAY,
	0		// end of list
};

//////////////////////////////entrypoint///////////////////////////////
int _start(int argc, char* argv[]){
    int x;

	CpuSuspendIntr(&x);
	if (RegisterLibraryEntries(&export_stub)==0) {
		CpuResumeIntr(x);
		return 0;
	}

	CpuResumeIntr(x);
	return 1;
}

