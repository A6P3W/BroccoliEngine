#pragma once

class FAutomationComponentMethodRegistry;
class FAutomationActorMethodRegistry;

void RegisterAllAutomationMethods(
    FAutomationActorMethodRegistry& MethodRegistry,
    FAutomationComponentMethodRegistry& ComponentMethodRegistry
);
