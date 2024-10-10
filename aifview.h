//
//  Copyright (c) 2023 Jon Palmisciano. All rights reserved.
//
//  Use of this source code is governed by the BSD 3-Clause license; a full copy
//  of the license can be found in the LICENSE.txt file.
//

#pragma once

#include <binaryninjaapi.h>

namespace BN = BinaryNinja;

/// Apple iBoot family view.
///
/// Supports loading Apple's iBoot and related firmware images from the same
/// codebase, e.g. SecureROM, iBSS, AVPBooter, etc.
class AIFView : public BN::BinaryView {
	BN::Ref<BN::Logger> m_logger;
	BN::Ref<BN::AnalysisCompletionEvent> m_completionEvent;

	std::uint64_t m_base = 0;
	std::string m_name;

	std::uint64_t GetPredictedBaseAddress();

	void DefineFixedOffsetSymbols();
	void DefineStringAssociatedSymbols();

public:
	explicit AIFView(BinaryView *data);

	bool Init() override;
};

class AIFViewType : public BN::BinaryViewType {
	BN::Ref<BN::Logger> m_logger;

public:
	AIFViewType();

	BN::Ref<BN::BinaryView> Create(BN::BinaryView *data) override;
	BN::Ref<BN::BinaryView> Parse(BN::BinaryView *data) override;

	bool IsTypeValidForData(BN::BinaryView *data) override;
	bool IsDeprecated() override;

	BN::Ref<BN::Settings> GetLoadSettingsForData(BN::BinaryView *data) override;
};
