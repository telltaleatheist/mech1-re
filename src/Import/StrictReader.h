// The story's record files are read strictly (src/Support/StrictReader.h),
// every error an ImportError naming the file. Internal to src/Import.
#pragma once

#include "../Support/StrictReader.h"
#include "Story.h"

namespace mw1o::import::detail {

namespace fs = std::filesystem;
using support::Json;
using Reader = support::StrictReader<ImportError>;

inline std::string read_text_file(const fs::path& path) { return support::read_text_file<ImportError>(path); }

}  // namespace mw1o::import::detail
