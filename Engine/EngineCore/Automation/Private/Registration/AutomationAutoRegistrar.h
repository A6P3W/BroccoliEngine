#pragma once

class FAutomationComponentMethodRegistry;
class FAutomationMethodRegistry;

void RegisterAllAutomationMethods(
    FAutomationMethodRegistry& MethodRegistry,
    FAutomationComponentMethodRegistry& ComponentMethodRegistry
);
