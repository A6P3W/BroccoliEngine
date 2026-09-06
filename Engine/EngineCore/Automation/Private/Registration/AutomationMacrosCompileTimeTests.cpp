#include <string_view>

#include "AutomationMacros.h"

namespace {
struct FUnsupportedAutomationValue {};

class FAutomationMacroTestActor final : public AActor {
 public:
  DEFINE_ACTOR_CLASS(FAutomationMacroTestActor)

  void SetEnabled(bool bEnabled);
  FUnsupportedAutomationValue GetUnsupportedValue() const;
};

struct FInvalidAutomationResultAdapter {
  nlohmann::json operator()(std::string_view Value) const;
};

using TSetEnabledMethod = decltype(&FAutomationMacroTestActor::SetEnabled);

template <class TMethod, size_t N>
constexpr bool bCanRegisterWithMetadata =
    requires(BroccoliAutomationDetail::FAutomationRegistrationContext& Context, TMethod Method) {
      BroccoliAutomationDetail::RegisterMethod(
          Context,
          std::string{},
          std::string{},
          EAutomationPermission::ReadOnly,
          Method,
          std::array<FAutomationParameterMetadata, N>{}
      );
    };

static_assert(BroccoliAutomationDetail::AutomationMemberFunction<TSetEnabledMethod>);
static_assert(BroccoliAutomationDetail::AutomationOwner<FAutomationMacroTestActor>);
static_assert(BroccoliAutomationDetail::AutomationMethodArgumentsJsonReadable<TSetEnabledMethod>);
static_assert(!BroccoliAutomationDetail::AutomationMemberFunction<void (*)()>);
static_assert(!BroccoliAutomationDetail::AutomationOwner<int>);
static_assert(!AutomationJsonReadable<FUnsupportedAutomationValue>);
static_assert(!AutomationJsonWritable<FUnsupportedAutomationValue>);
static_assert(
    !BroccoliAutomationDetail::
        AutomationResultAdapter<FInvalidAutomationResultAdapter, FAutomationMacroTestActor, bool>
);
static_assert(!bCanRegisterWithMetadata<TSetEnabledMethod, 0>);
}  // namespace
