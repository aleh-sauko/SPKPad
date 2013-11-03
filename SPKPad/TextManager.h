#pragma once
#include "Header.h"

class TextManager
{
public:
	TextManager(HWND hWnd);
	~TextManager(void);

	void InsertChar(TCHAR char_code);
	void RemoveChars(void);

	void RenderText(HDC hdc, LPRECT clientRect);
	void GetItemSize(HDC hdc, TCHAR item, SIZE &size);
	void RenderItem(HDC hdc, int x, int y, TCHAR item);

	void MouseUp(int x, int y);
	void MouseDown(int x, int y);
	void MouseWheelUp();
	void MouseWheelDown();

	int GetCursorPos(int x, int y, int &charIndex);
	
	void ApplyScale();
	void EnterFocus();
	void LeaveFocus();

	void DeleteSelection();
private:
	HWND hWnd;
	TextManagerState textManagerState;
	std::vector<Line> lines;
	std::vector<HFONT> fontDescriptors;

	float scale;
};