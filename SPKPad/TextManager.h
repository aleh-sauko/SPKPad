#pragma once
#include "Header.h"

class TextManager
{
public:
	TextManager(HWND hWnd);
	~TextManager(void);

	void InsertChar(TCHAR char_code);
	void InsertChars(const std::wstring &str);
	void RemoveChars(void);

	void RenderText(HDC hdc, LPRECT clientRect);
	void GetItemSize(HDC hdc, TCHAR item, SIZE &size);
	void RenderItem(HDC hdc, int x, int y, TCHAR item);

	void MouseUp(int x, int y);
	void MouseDown(int x, int y);
	void MouseMove(int x, int y, bool leftButtonPressed);
	void MouseDoubleClick();
	void MouseWheelUp();
	void MouseWheelDown();

	void ApplyFont(int font);
	void InsertImage();

	void Undo();
	void Redo();
	void SaveInHistory();

	void LoadFile();
	void SaveFile();

	std::wstring GetSelectedText();

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
	std::vector<Gdiplus::Bitmap*> images;

	std::deque<TextManagerState> undoDeque;
	std::deque<TextManagerState> redoDeque;

	float scale;
	int imageIndex;

	static const BYTE FONT= 0xfe;
	static const BYTE IMAGE = 0xff;
};