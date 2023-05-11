#pragma once

#include <binaryninjaapi.h>

namespace BN = BinaryNinja;

class SecureBootView : public BN::BinaryView {
	bool m_isROM;
	std::string m_name;
	std::uint64_t m_base;

	BN::Ref<BN::Logger> m_logger;

	std::uint64_t GetPredictedBaseAddress();

public:
	SecureBootView(BinaryView *data);

	bool Init() override;
};

class SecureBootViewType : public BN::BinaryViewType {
public:
	SecureBootViewType();

	BN::Ref<BN::BinaryView> Create(BN::BinaryView *data) override;
	BN::Ref<BN::BinaryView> Parse(BN::BinaryView *data) override;
	bool IsTypeValidForData(BN::BinaryView *data) override;
	BN::Ref<BN::Settings> GetLoadSettingsForData(BN::BinaryView *data) override;
	bool IsDeprecated() override;
};
