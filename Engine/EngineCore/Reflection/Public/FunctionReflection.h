#pragma once

#include <meta>
#include <utility>

namespace FunctionReflection {
template <class T, std::size_t Index>
consteval std::meta::info MemberAt() {
  return std::meta::members_of(^^T, std::meta::access_context::unchecked())[Index];
}

template <class T, std::size_t Index>
consteval std::meta::info BaseAt() {
  return std::meta::type_of(
      std::meta::bases_of(^^T, std::meta::access_context::unchecked())[Index]
  );
}

template <class T, class Annotation, class Callback, std::size_t... Indices>
void ForEachDirectAnnotatedFunction(Callback& Visitor, std::index_sequence<Indices...>) {
  (
      [&] {
        constexpr std::meta::info Member = MemberAt<T, Indices>();
        if constexpr (
            std::meta::is_function(Member) &&
            !std::meta::annotations_of_with_type(Member, ^^Annotation).empty()
        ) {
          Visitor.template operator()<Member>();
        }
      }(),
      ...);
}

template <class T, class Annotation, class Callback>
void ForEachAnnotatedFunction(Callback& Visitor);

template <class T, class Annotation, class Callback, std::size_t... Indices>
void ForEachAnnotatedBaseFunction(Callback& Visitor, std::index_sequence<Indices...>) {
  (
      [&] {
        using Base = [:BaseAt<T, Indices>():];
        ForEachAnnotatedFunction<Base, Annotation>(Visitor);
      }(),
      ...);
}

template <class T, class Annotation, class Callback>
void ForEachAnnotatedFunction(Callback& Visitor) {
  constexpr std::size_t BaseCount =
      std::meta::bases_of(^^T, std::meta::access_context::unchecked()).size();
  ForEachAnnotatedBaseFunction<T, Annotation>(Visitor, std::make_index_sequence<BaseCount>{});
  constexpr std::size_t MemberCount =
      std::meta::members_of(^^T, std::meta::access_context::unchecked()).size();
  ForEachDirectAnnotatedFunction<T, Annotation>(Visitor, std::make_index_sequence<MemberCount>{});
}
}  // namespace FunctionReflection
