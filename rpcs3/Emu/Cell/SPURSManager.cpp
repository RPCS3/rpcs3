#include "stdafx.h"
#include "SPURSManager.h"

SPURSManager::SPURSManager(SPURSManagerAttribute *attr)
{
	this->attr = attr;
}

void SPURSManager::Finalize()
{
	delete this->attr;
}