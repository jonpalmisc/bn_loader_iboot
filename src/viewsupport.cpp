//
//  Copyright (c) 2023 Jon Palmisciano. All rights reserved.
//
//  Use of this source code is governed by the BSD 3-Clause license; a full copy
//  of the license can be found in the LICENSE.txt file.
//

#include "viewsupport.h"

namespace ViewSupport {

std::string GetStringValue(BinaryViewRef data, BNStringReference const &ref)
{
	return data->ReadBuffer(ref.start, ref.length).ToEscapedString();
}

std::vector<BNStringReference> GetStringsContaining(BinaryViewRef data, char const *pattern)
{
	auto strings = data->GetStrings();

	std::vector<BNStringReference> matches;
	std::copy_if(strings.begin(), strings.end(), std::back_inserter(matches), [data, pattern](BNStringReference const &ref) {
		return GetStringValue(data, ref).find(pattern) != std::string::npos;
	});

	return matches;
}

}
