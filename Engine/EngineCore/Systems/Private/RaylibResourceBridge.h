#pragma once

#include <string>

#include "BroccoliRaylib.h"

const Texture2D* GetRaylibTexture(int Handle);
Rectangle GetRaylibTextureSource(int Handle);
bool IsRaylibRenderTexture(int Handle);
bool GetRaylibTextureSize(int Handle, int& OutWidth, int& OutHeight);

const Font* GetRaylibFont(int Handle, const std::string& Text = {});
float GetRaylibFontSize(int Handle);
Vector2 MeasureRaylibText(int Handle, const std::string& Text);

int CreateRaylibRenderTexture(int Width, int Height);
void ReleaseRaylibTexture(int Handle);
bool BeginRaylibRenderTexture(int Handle);
void EndRaylibRenderTexture();
