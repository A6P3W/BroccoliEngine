#pragma once
#include "PlayerController.h"

class EditorMode;
class MEnhancedInputComponent;
class EditorController : public APlayerController
{
public:
	EditorController();

	void SetupPlayerInputComponent(MEnhancedInputComponent* PlayerInputComponent) override;
	void SetPlayerId(int id);

private:
	void OnSelectModePressed();
	void OnMoveModePressed();
	void OnRotateModePressed();
	void OnScaleModePressed();

	EditorMode* EditorModePtr = nullptr;
};
