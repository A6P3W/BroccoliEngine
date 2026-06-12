#pragma once
#include "PlayerController.h"

class EditorMode;
class MEnhancedInputComponent;
class EditorController : public APlayerController
{
public:
	EditorController();

	void SetupPlayerInputComponent(MEnhancedInputComponent* PlayerInputComponent) override;
	void SetupInputMappings() override;
private:
	void OnSelectModePressed();
	void OnMoveModePressed();
	void OnRotateModePressed();
	void OnScaleModePressed();

	void OnCopyPressed();
	void OnPastePressed();
	void OnCutPressed();
	void OnDeletePressed();

	void OnSavePressed();

	EditorMode* EditorModePtr = nullptr;
};
