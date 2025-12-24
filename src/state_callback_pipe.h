#pragma once

#include <memory>

#include "duct/duct.h"

namespace duct {

std::unique_ptr<Pipe> make_state_callback_pipe(std::unique_ptr<Pipe> inner, ConnectionCallback cb);

}  // namespace duct

