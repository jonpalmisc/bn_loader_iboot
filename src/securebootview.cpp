#include "securebootview.h"

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
	} catch (std::exception &e) {
		LogError("Failed to create SecureBootView!");
		return nullptr;
	}
}

bool SecureBootViewType::IsTypeValidForData(BinaryView *data)
{
	if (!data)
		return false;

	auto tag = data->ReadBuffer(0x200, 9).GetData();
	if (memcmp(tag, "iBoot", 5) == 0)
		return true;
	if (memcmp(tag, "iBEC", 4) == 0)
		return true;
	if (memcmp(tag, "iBSS", 4) == 0)
		return true;
	if (memcmp(tag, "SecureROM", 9) == 0)
		return true;
	if (memcmp(tag, "AVPBooter", 9) == 0)
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
{
	m_logger = CreateLogger("BinaryView.iBoot");

	std::vector<std::string> supportedVariants = {
		"SecureROM",
		"iBoot",
		"iBEC",
		"iBSS",
		"AVPBooter",
	};

	auto rawName = data->ReadBuffer(0x200, 9).ToEscapedString();
	for (auto const &v : supportedVariants) {
		if (rawName.find(v) == std::string::npos)
			continue;

		m_name = v;
		if (v == "SecureROM")
			m_isROM = true;

		break;
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

struct KnownOffsetSymbol {
	std::uint32_t offset;
	BNSymbolType type;
	char const *name;
};

std::vector<KnownOffsetSymbol> KnownOffsetSymbols = {
	{ 0x0, FunctionSymbol, "_start" },
	{ 0x200, DataSymbol, "build_banner_string" },
	{ 0x240, DataSymbol, "build_style_string" },
	{ 0x280, DataSymbol, "build_tag_string" },
};

bool SecureBootView::Init()
{
	auto aarch64 = Architecture::GetByName("aarch64");
	SetDefaultArchitecture(aarch64);
	SetDefaultPlatform(aarch64->GetStandalonePlatform());

	m_base = GetPredictedBaseAddress();
	if (!m_base) {
		m_logger->LogError("Failed to predict base address via relocation loop; analysis will be poor!");
	} else {
		m_logger->LogInfo("Predicted base address is 0x%" PRIx64 ".", m_base);
	}

	auto parentView = GetParentView();
	AddAutoSegment(m_base, parentView->GetLength(), 0, parentView->GetLength(), SegmentReadable | SegmentExecutable);
	AddUserSection(m_name, m_base, parentView->GetLength(), ReadOnlyCodeSectionSemantics);

	for (auto const &s : KnownOffsetSymbols) {
		DefineUserSymbol(new Symbol(s.type, s.name, m_base + s.offset));
		m_logger->LogInfo("Defined known symbol `%s` at 0x%" PRIx64 ".", s.name, m_base + s.offset);
	}

	AddEntryPointForAnalysis(GetDefaultPlatform(), m_base);

	return true;
}
