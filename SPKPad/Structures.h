#pragma once
#include "Header.h"

struct TextManagerState
{
	int caretPosition;
	int startSelection;
	int endSelection;
	std::wstring text;
};

struct Line
{
	int startY;
	int endY;
	std::vector<int> charsStartX;
};
