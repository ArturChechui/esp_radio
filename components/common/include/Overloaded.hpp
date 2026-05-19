/**
 * @file Overloaded.hpp
 * @brief Helper utility for variant visitation.
 *
 * This file defines the Overloaded struct, which enables the "vistor" pattern
 * for std::variant by aggregating multiple lambda functions into a single
 * callable object.
 */

#pragma once

/**
 * @namespace common
 * @brief Contains shared utility and messaging interfaces.
 */
namespace common {

/**
 * @struct Overloaded
 * @brief A helper for visiting std::variant types.
 *
 * Overloaded uses variadic templates and multiple inheritance to combine
 * several callable objects (typically lambdas) into one. This allows
 * std::visit to dispatch to the correct overload based on the type currently
 * held by the variant.
 *
 * @tparam Ts... A template parameter pack of callable types (lambdas).
 */
template <class... Ts>
struct Overloaded : Ts... {
    /** @brief Exposes the operator() of all base classes to the derived struct. */
    using Ts::operator()...;
};

/**
 * @brief Template deduction guide for the Overloaded struct.
 * * This allows the compiler to automatically deduce the template types
 * when writing: `Overloaded{ lambda1, lambda2, ... }`.
 */
template <class... Ts>
Overloaded(Ts...) -> Overloaded<Ts...>;

}  // namespace common
