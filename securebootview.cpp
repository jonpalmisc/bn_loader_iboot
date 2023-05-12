//
//  Copyright (c) 2023 Jon Palmisciano. All rights reserved.
//
//  Use of this source code is governed by the BSD 3-Clause license; a full copy
//  of the license can be found in the LICENSE.txt file.
//

#include "securebootview.h"
#include "viewsupport.h"

#include <cinttypes>

using namespace BinaryNinja;

constexpr auto SecureBootViewDisplayName = "iBoot";

SecureBootViewType::SecureBootViewType()
    : BinaryViewType(SecureBootViewDisplayName, SecureBootViewDisplayName)
{
}

Ref<BinaryView> SecureBootViewType::Create(BinaryView *data)
{
	try {
		return new SecureBootView(data);
	} catch (std::exception &e) {
		LogError("Failed to create SecureBootView!");
		return nullptr;
	}
}

Ref<BinaryView> SecureBootViewType::Parse(BinaryView *data)
{
	try {
		return new SecureBootView(data);
	} catch (...) {
		LogError("Failed to create SecureBootView!");
		return nullptr;
	}
}

bool SecureBootViewType::IsTypeValidForData(BinaryView *data)
{
	if (!data)
		return false;

	auto tag = data->ReadBuffer(0x200, 9).ToEscapedString();
	if (tag.find("iBoot") != std::string::npos)
		return true;
	if (tag.find("iBEC") != std::string::npos)
		return true;
	if (tag.find("iBSS") != std::string::npos)
		return true;
	if (tag.find("SecureROM") != std::string::npos)
		return true;
	if (tag.find("AVPBooter") != std::string::npos)
		return true;

	return false;
}

Ref<Settings> SecureBootViewType::GetLoadSettingsForData(BinaryView *data)
{
	return nullptr;
}

bool SecureBootViewType::IsDeprecated()
{
	return false;
}

SecureBootView::SecureBootView(BinaryView *data)
    : BinaryView(SecureBootViewDisplayName, data->GetFile(), data)
    , m_logger(CreateLogger("BinaryView.iBoot"))
    , m_completionEvent(nullptr)
    , m_base(0)
    , m_name("iBoot")
{
	std::vector<std::string> otherVariants = {
		"SecureROM",
		"iBEC",
		"iBSS",
		"AVPBooter",
	};

	auto rawName = data->ReadBuffer(0x200, 9).ToEscapedString();
	for (auto const &v : otherVariants) {
		if (rawName.find(v) != std::string::npos) {
			m_name = v;
			break;
		}
	}
}

uint64_t SecureBootView::GetPredictedBaseAddress()
{
	auto parentView = GetParentView();
	auto arch = GetDefaultArchitecture();

	std::uint8_t rawInsn[4] = { 0 };
	std::vector<InstructionTextToken> tokens;
	std::size_t insnLen = 0;

	for (uint64_t i = 0; i < 0x200; i += 4) {
		parentView->Read(rawInsn, i, sizeof(rawInsn));

		auto ok = arch->GetInstructionText((uint8_t const *)&rawInsn, i, insnLen, tokens);
		if (!ok || tokens.empty()) {
			m_logger->LogError("Failed to get instruction text at offset 0x%x.", i);
			return 0;
		}

		// A LDR should be present in the first few instructions to get
		// the address the image should be copied to.
		if (tokens[0].text != "ldr")
			continue;

		// The last token will be the offset referenced. The value field
		// can be used to get the offset as an integer.
		auto lastToken = --tokens.end();
		auto offset = lastToken->value;

		BinaryReader reader(parentView);
		reader.Seek(offset);

		try {
			return reader.Read64();
		} catch (...) {
			m_logger->LogError("Failed to read parent view while predicting base address!");
			break;
		}
	}

	return 0;
}

struct FixedOffsetSymbol {
	std::uint32_t offset;
	BNSymbolType type;
	char const *name;
};

std::vector<FixedOffsetSymbol> KnownFixedOffsetSymbols = {
	{ 0x0, FunctionSymbol, "_start" },
	{ 0x200, DataSymbol, "build_banner_string" },
	{ 0x240, DataSymbol, "build_style_string" },
	{ 0x280, DataSymbol, "build_tag_string" },
};

void SecureBootView::DefineFixedOffsetSymbols()
{
	for (auto const &def : KnownFixedOffsetSymbols) {
		auto userSymbol = new Symbol(def.type, def.name, m_base + def.offset);
		DefineUserSymbol(userSymbol);

		m_logger->LogInfo("Defined fixed-offset symbol `%s` at 0x%" PRIx64 ".", def.name, m_base + def.offset);
	}
}

struct StringAssociatedSymbol {
	char const *name;
	char const *pattern;
};

std::vector<StringAssociatedSymbol> KnownStringAssociatedSymbols = {
	{ "_panic", "double panic in" },
	{ "_platform_get_usb_serial_number_string", "CPID:" },
	{ "_platform_get_usb_more_other_string", " NONC:" },
	{ "_image4_get_partial", "IMG4" },
	{ "_UpdateDeviceTree", "fuse-revision" },
	{ "_main_task", "debug-uarts" },
	{ "_platform_init_display", "backlight-level" },
	{ "_do_printf", "<null>" },
	{ "_do_memboot", "Combo image too large" },
	{ "_do_go", "Memory image not valid" },
	{ "_task_init", "idle task" },
	{ "_sys_setup_default_environment", "/System/Library/Caches/com.apple.kernelcaches/kernelcache" },
	{ "_check_autoboot", "aborting autoboot due to user intervention" },
	{ "_do_setpict", "picture too large" },
	{ "_arm_exception_abort", "ARM %s abort at 0x%016llx:" },
	{ "_do_devicetree", "Device Tree image not valid" },
	{ "_do_ramdisk", "Ramdisk image not valid" },
	{ "_usb_serial_init", "Apple USB Serial Interface" },
	{ "_nvme_bdev_create", "construct blockdev for namespace %d" },
	{ "_image4_dump_list", "image %p: bdev %p type" },
	{ "_prepare_and_jump", "End of %s serial output" },
	{ "_boot_upgrade_system", "/boot/kernelcache" },
};

void SecureBootView::DefineStringAssociatedSymbols()
{
	for (auto const &def : KnownStringAssociatedSymbols) {
		auto strings = ViewSupport::GetStringsContaining(this, def.pattern);
		if (strings.empty()) {
			m_logger->LogDebug("Failed to find string with pattern \"%s\".", def.pattern);
			continue;
		}

		auto refs = GetCodeReferences(strings.front().start);
		if (refs.empty()) {
			m_logger->LogDebug("Failed to find code references to string with pattern \"%s\".", def.pattern);
			continue;
		}

		auto function = refs.front().func;
		DefineUserSymbol(new Symbol(FunctionSymbol, def.name, function->GetStart()));

		m_logger->LogInfo("Defined symbol `%s` for function at 0x%" PRIx64 " based on string reference(s).", def.name, function->GetStart());
	}
}

bool SecureBootView::Init()
{
	auto aarch64 = Architecture::GetByName("aarch64");
	SetDefaultArchitecture(aarch64);
	SetDefaultPlatform(aarch64->GetStandalonePlatform());

	m_base = GetPredictedBaseAddress();
	if (!m_base)
		m_logger->LogError("Failed to predict base address via relocation loop; analysis will be poor!");
	else
		m_logger->LogInfo("Predicted base address is 0x%" PRIx64 ".", m_base);

	auto parentView = GetParentView();
	AddAutoSegment(m_base, parentView->GetLength(), 0, parentView->GetLength(), SegmentReadable | SegmentExecutable);
	AddUserSection(m_name, m_base, parentView->GetLength(), ReadOnlyCodeSectionSemantics);

	// TODO: Put these behind settings.
	DefineFixedOffsetSymbols();
	m_completionEvent = new AnalysisCompletionEvent(this, [this] {
		m_logger->LogInfo("Analysis complete, searching for strings to help define symbols...");
		DefineStringAssociatedSymbols();
	});

	AddEntryPointForAnalysis(GetDefaultPlatform(), m_base);

	return true;
}