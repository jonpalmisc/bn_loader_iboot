//
//  Copyright (c) 2023 Jon Palmisciano. All rights reserved.
//
//  Use of this source code is governed by the BSD 3-Clause license; a full copy
//  of the license can be found in the LICENSE.txt file.
//

#include "aifview.h"

#include <cinttypes>

using namespace BinaryNinja;

static constexpr auto OFFSET_BUILD_BANNER = 0x200;
static constexpr auto OFFSET_BUILD_STYLE = 0x240;
static constexpr auto OFFSET_BUILD_TAG = 0x240;

static constexpr auto VIEW_DISPLAY_NAME = "iBoot";

AIFView::AIFView(BinaryView *data)
    : BinaryView(VIEW_DISPLAY_NAME, data->GetFile(), data)
    , m_logger(LogRegistry::CreateLogger("BinaryView.iBoot"))
    , m_completionEvent(nullptr)
    , m_name("iBoot")
{
	std::vector<std::string> otherVariants = {
		"SecureROM",
		"iBEC",
		"iBSS",
		"AVPBooter",
	};

	auto rawName = data->ReadBuffer(OFFSET_BUILD_BANNER, 9).ToEscapedString();
	for (auto const &v : otherVariants)
	{
		if (rawName.find(v) != std::string::npos)
		{
			m_name = v;
			break;
		}
	}
}

uint64_t AIFView::GetPredictedBaseAddress()
{
	auto parentView = GetParentView();
	if (!parentView)
	{
		m_logger->LogError("Failed to get parent view while detecting base address!");
		return 0;
	}

	auto arch = GetDefaultArchitecture();
	if (!arch)
	{
		m_logger->LogError("Failed to get default architecture while detecting base address!");
		return 0;
	}

	std::uint8_t rawInsn[4] = { 0 };
	std::vector<InstructionTextToken> tokens;
	std::size_t insnLen = 0;

	for (uint64_t i = 0; i < OFFSET_BUILD_BANNER; i += 4)
	{
		parentView->Read(rawInsn, i, sizeof(rawInsn));

		auto ok = arch->GetInstructionText((uint8_t const *)&rawInsn, i, insnLen, tokens);
		if (!ok || tokens.empty())
		{
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

		try
		{
			return reader.Read64();
		}
		catch (...)
		{
			m_logger->LogError("Failed to read parent view while predicting base address!");
			break;
		}
	}

	return 0;
}

struct FixedOffsetSymbol
{
	std::uint32_t offset;
	BNSymbolType type;
	char const *name;
};

static std::vector<FixedOffsetSymbol> g_knownFixedOffsetSymbols = {
	{ 0x0, FunctionSymbol, "_start" },
	{ OFFSET_BUILD_BANNER, DataSymbol, "build_banner_string" },
	{ OFFSET_BUILD_STYLE, DataSymbol, "build_style_string" },
	{ OFFSET_BUILD_TAG, DataSymbol, "build_tag_string" },
};

void AIFView::DefineFixedOffsetSymbols()
{
	for (auto const &def : g_knownFixedOffsetSymbols)
	{
		DefineAutoSymbol(new Symbol(def.type, def.name, m_base + def.offset));
		m_logger->LogDebug("Defined fixed-offset symbol `%s` at 0x%" PRIx64 ".", def.name, m_base + def.offset);
	}
}

uint64_t AIFView::PerformGetStart() const
{
	return m_base;
}

uint64_t AIFView::PerformGetEntryPoint() const
{
	return m_base;
}

static constexpr auto SETTING_DEFINE_FIXED_SYMS = "loader.iboot.defineFixedSymbols";

bool AIFView::Init()
{
	SetDefaultPlatform(Platform::GetByName("aarch64"));
	SetDefaultArchitecture(GetDefaultPlatform()->GetArchitecture());

	m_base = GetPredictedBaseAddress();

	auto settings = GetLoadSettings(GetTypeName());
	if (settings)
	{
		if (settings->Contains("loader.imageBase"))
			m_base = settings->Get<uint64_t>("loader.imageBase", this);

		if (settings->Contains("loader.platform"))
		{
			auto overridePlatformName = settings->Get<std::string>("loader.platform", this);
			auto overridePlatform = Platform::GetByName(overridePlatformName);
			if (overridePlatform)
			{
				SetDefaultPlatform(overridePlatform);
				SetDefaultArchitecture(overridePlatform->GetArchitecture());
			}
			else
			{
				m_logger->LogError("Invalid platform override provided!");
			}
		}
	}

	if (!m_base)
		m_logger->LogWarn("No base address provided or detected; analysis will be poor!");

	auto parent = GetParentView();
	AddAutoSegment(m_base, parent->GetLength(), 0, parent->GetLength(), SegmentReadable | SegmentExecutable);
	AddAutoSection(m_name, m_base, parent->GetLength(), ReadOnlyCodeSectionSemantics);
	AddEntryPointForAnalysis(GetDefaultPlatform(), m_base);

	if (!settings || settings->Get<bool>(SETTING_DEFINE_FIXED_SYMS))
		DefineFixedOffsetSymbols();

	return true;
}

AIFViewType::AIFViewType()
    : BinaryViewType(VIEW_DISPLAY_NAME, VIEW_DISPLAY_NAME)
    , m_logger(LogRegistry::CreateLogger("BinaryView.iBoot"))
{
}

Ref<BinaryView> AIFViewType::Create(BinaryView *data)
{
	try
	{
		return new AIFView(data);
	}
	catch (std::exception &e)
	{
		m_logger->LogError("Failed to create AIFView!");
		return nullptr;
	}
}

Ref<BinaryView> AIFViewType::Parse(BinaryView *data)
{
	try
	{
		return new AIFView(data);
	}
	catch (...)
	{
		m_logger->LogError("Failed to create AIFView!");
		return nullptr;
	}
}

bool AIFViewType::IsTypeValidForData(BinaryView *data)
{
	if (!data)
		return false;

	// A legit iBoot family image should be much larger than this, but it
	// should at least be large enough to hold the build tag region and
	// table of pointers that follows.
	//
	// This also assures the read below will be in-bounds.
	if (data->GetLength() < 0x400)
		return false;

	auto tag = data->ReadBuffer(OFFSET_BUILD_BANNER, 9).ToEscapedString();
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

bool AIFViewType::IsDeprecated()
{
	return false;
}

Ref<Settings> AIFViewType::GetLoadSettingsForData(BinaryView *data)
{
	auto view = Parse(data);
	if (!view || !view->Init())
	{
		m_logger->LogError("Failed to initialize view while getting load settings!");
		return nullptr;
	}

	auto settings = GetDefaultLoadSettingsForData(view);

	// Allow changes in case the auto-detected base address is wrong.
	if (settings->Contains("loader.imageBase"))
		settings->UpdateProperty("loader.imageBase", "readOnly", false);

	// Defaults to AArch64, but allow changing this in case someone is
	// trying to load an ancient 32-bit iBoot.
	if (settings->Contains("loader.platform"))
		settings->UpdateProperty("loader.platform", "readOnly", false);

	// We don't define a lot of fixed-offset symbols, but I suppose there
	// should be an escape hatch for that as well.
	settings->RegisterSetting(SETTING_DEFINE_FIXED_SYMS,
	    R"({
		"title" : "Define Fixed-Offset Symbols",
		"type" : "boolean",
		"default" : true,
		"description" : "Define symbols known to reside at fixed offsets."
	    })");

	return settings;
}
