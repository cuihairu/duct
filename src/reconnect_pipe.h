#pragma once

#include <functional>
#include <memory>

#include "duct/duct.h"
#include "duct/status.h"

namespace duct {

using DialOnceFn = std::function<Result<std::unique_ptr<Pipe>>()>;

std::unique_ptr<Pipe> make_reconnect_pipe(DialOnceFn dial_once,
                                         ReconnectPolicy policy,
                                         ConnectionCallback on_state_change);

}  // namespace duct

