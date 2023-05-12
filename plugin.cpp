#include "securebootview.h"

static SecureBootViewType *g_secureBootViewType = nullptr;

BN_DECLARE_CORE_ABI_VERSION

extern "C" BINARYNINJAPLUGIN bool CorePluginInit()
{
	g_secureBootViewType = new SecureBootViewType;

	BinaryNinja::BinaryViewType::Register(g_secureBootViewType);
	return true;
}
