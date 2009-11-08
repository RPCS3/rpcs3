#include "CDVD.h"
#include "ReaderModules.h"

Source* TryLoaders(const char* fileName)
{
	Source *src=NULL;
	if((src=PlainIso::TryLoad(fileName))!=NULL) return src;
	//if((src=CueSheet::TryLoad(fileName))!=NULL) return src;
	//if((src=CloneCD::TryLoad(fileName))!=NULL) return src;
	//error
	return NULL;
}