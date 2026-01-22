#include "ibfview.h"

BN_DECLARE_CORE_ABI_VERSION

extern "C" BINARYNINJAPLUGIN bool CorePluginInit()
{
	static IBFViewType *ibfViewType = nullptr;
	ibfViewType = new IBFViewType;

	BinaryNinja::BinaryViewType::Register(ibfViewType);
	return true;
}
