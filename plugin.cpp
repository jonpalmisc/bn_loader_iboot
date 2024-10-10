#include "aifview.h"

BN_DECLARE_CORE_ABI_VERSION

extern "C" BINARYNINJAPLUGIN bool CorePluginInit()
{
	static AIFViewType *aifViewType = nullptr;
	aifViewType = new AIFViewType;

	BinaryNinja::BinaryViewType::Register(aifViewType);
	return true;
}
