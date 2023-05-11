//
//  Copyright (c) 2023 Jon Palmisciano. All rights reserved.
//
//  Use of this source code is governed by the BSD 3-Clause license; a full copy
//  of the license can be found in the LICENSE.txt file.
//

#pragma once

#include <binaryninjaapi.h>

namespace BN = BinaryNinja;

class SecureBootView : public BN::BinaryView {
	BN::Ref<BN::Logger> m_logger;
	BN::Ref<BN::AnalysisCompletionEvent> m_completionEvent;

	std::uint64_t m_base;
	std::string m_name;

	std::uint64_t GetPredictedBaseAddress();

	std::string GetStringValue(BNStringReference const &ref);
	std::vector<BNStringReference> GetStringsContaining(char const *pattern);
	BNStringReference GetFirstStringContaining(char const *pattern);

	void DefineFixedOffsetSymbols();
	void DefineStringAssociatedSymbols();

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
	bool IsDeprecated() override;

	BN::Ref<BN::Settings> GetLoadSettingsForData(BN::BinaryView *data) override;
};
