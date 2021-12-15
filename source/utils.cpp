#include "utils.h"

#include <stdexcept>

namespace ddn
{

void ValidateResult(HRESULT hr)
{
    if (FAILED(hr)) {
        throw std::runtime_error("Invalid return value");
    }
}

}  // namespace ddn
