#pragma once

namespace common {
// Helper for std::visit on std::variant.
// - Ts... means "Types..." (a template parameter pack of types).
// - Overloaded inherits from each lambda type in Ts... and exposes all operator() overloads,
//   so std::visit can dispatch to the correct handler.
template <class... Ts>
struct Overloaded : Ts... {
    using Ts::operator()...;
};
// Deduction guide so that it can be written as: Overloaded{ lambda1, lambda2, ... }
template <class... Ts>
Overloaded(Ts...) -> Overloaded<Ts...>;

}  // namespace common
