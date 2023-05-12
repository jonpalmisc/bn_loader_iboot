//
//  Copyright (c) 2023 Jon Palmisciano. All rights reserved.
//
//  Use of this source code is governed by the BSD 3-Clause license; a full copy
//  of the license can be found in the LICENSE.txt file.
//

#pragma once

#include <binaryninjaapi.h>

namespace BN = BinaryNinja;

using BinaryViewRef = BN::Ref<BN::BinaryView>;

namespace ViewSupport {

std::string GetStringValue(BinaryViewRef data, BNStringReference const &ref);
std::vector<BNStringReference> GetStringsContaining(BinaryViewRef data, char const *pattern);

}
