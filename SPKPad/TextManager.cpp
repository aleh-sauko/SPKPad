#include "Header.h"

using namespace std;

TextManager::TextManager(HWND hWnd)
{
	TextManager::hWnd = hWnd;

	textManagerState.caretPosition = 0;
	textManagerState.endSelection = -1;
	textManagerState.startSelection = -1;
	scale = 1;

	wstring fontNames[] = {L"comic.ttf", L"cour.ttf"};
	int countOfFonts = sizeof(fontNames) / sizeof(fontNames[0]);
	for (int i = 0; i < countOfFonts; ++i)
	{
		AddFontResourceEx(fontNames[i].c_str(), FR_PRIVATE, NULL);
	}

	wstring fontNames2[] = {L"Comic Sans MS", L"Courier New"};
	for (int i = 0; i < 2; ++i)
	{
		LOGFONT logFont;
		ZeroMemory(&logFont, sizeof(LOGFONT));
		logFont.lfHeight = (LONG) (16 * scale);
		wcscpy_s(logFont.lfFaceName, fontNames2[i].c_str());
		fontDescriptors.push_back(CreateFontIndirect(&logFont));
	}
}

TextManager::~TextManager(void) 
{
	for (unsigned int i = 0; i < fontDescriptors.size(); ++i)
	{
		DeleteObject(fontDescriptors.at(i));
	}
	wstring fontNames[] = {L"comic.ttf", L"cour.ttf"};
	int countOfFonts = sizeof(fontNames) / sizeof(fontNames[0]);
	for (int i = 0; i < countOfFonts; ++i)
	{
		RemoveFontResourceEx(fontNames[i].c_str(), FR_PRIVATE, NULL);
	}
}


void TextManager::InsertChar(TCHAR char_code)
{
	DeleteSelection();
	textManagerState.caretPosition = min(textManagerState.caretPosition, textManagerState.text.size());
	textManagerState.caretPosition = max(textManagerState.caretPosition, 0);
	textManagerState.text.insert(textManagerState.caretPosition++, &char_code, 1);
	//historyManager.SetCurrentState(currentState);
	InvalidateRect(hWnd, NULL, false);
}


void TextManager::RemoveChars()
{
	if (textManagerState.startSelection != -1 && textManagerState.endSelection != -1)
	{
		DeleteSelection();
		//historyManager.SetCurrentState(currentState);
		return;
	}
	--textManagerState.caretPosition;
	while (textManagerState.caretPosition > 0 && (textManagerState.text.at(textManagerState.caretPosition - 1) >> 8) == 0xfe)
	{
		textManagerState.text.erase(--textManagerState.caretPosition, 1);
	}
	++textManagerState.caretPosition;
	if (textManagerState.caretPosition > 0)
	{
		textManagerState.text.erase(--textManagerState.caretPosition, 1);
		
	}
	textManagerState.caretPosition = min(textManagerState.caretPosition, textManagerState.text.size() - 1);
	
	//historyManager.SetCurrentState(currentState);
}

void TextManager::DeleteSelection()
{
	if (textManagerState.startSelection == -1 || textManagerState.endSelection == -1 || 
		textManagerState.text.empty())
	{
		return;
	}

	int startPos = min(textManagerState.startSelection, textManagerState.endSelection);
	int endPos = max(textManagerState.startSelection, textManagerState.endSelection);

	textManagerState.text.erase(startPos, endPos - startPos);
	textManagerState.caretPosition = startPos;
	textManagerState.endSelection = -1;
	textManagerState.startSelection = -1;
}


void TextManager::RenderText(HDC hdc, LPRECT clientRect)
{
	HDC bufferDC = CreateCompatibleDC(hdc);
	
	int width = clientRect->right - clientRect->left;
	int height = clientRect->bottom - clientRect->top;
	
	HBITMAP bitmap = CreateCompatibleBitmap(hdc, width, height);
	SelectObject(bufferDC, bitmap);
	
	RECT r;
	SetRect(&r, 0, 0, width, height);
	FillRect(bufferDC, &r, (HBRUSH) GetStockObject(WHITE_BRUSH));

	//clientRect->right -= RIGHT_MARGIN;
	int curX = 0;
	int curY = 0;
	int nextLineY = 0;
	int startLinePos = -1;

	Line currentLine;
	
	lines.clear();
	
	SelectObject(bufferDC, fontDescriptors[1]);

	HFONT hOldFont = fontDescriptors[0];
	
	SetCaretPos(0, 0);

	if (!textManagerState.text.empty())
	{
		for (UINT i = 0; i < textManagerState.text.length(); ++i)
		{
			SIZE size;
			GetItemSize(bufferDC, textManagerState.text.at(i), size);
			if (curX + size.cx >= (clientRect->right - clientRect->left) || textManagerState.text.at(i) == L'\r')
			{
				currentLine.startY = curY;
				currentLine.endY = nextLineY;
				curX = 0;
				curY = nextLineY;
				lines.push_back(currentLine);
				currentLine.charsStartX.clear();
			}

			nextLineY = max(nextLineY, curY + size.cy);

			if (i >= min(textManagerState.startSelection, textManagerState.endSelection)
				&& i < max(textManagerState.startSelection, textManagerState.endSelection))
			{
				SetTextColor(bufferDC, RGB(255, 255, 255));
				SetBkColor(bufferDC, RGB(80, 80, 80));
			}
			else
			{
				SetTextColor(bufferDC, RGB(0, 0, 0));
				SetBkColor(bufferDC, RGB(255, 255, 255));
			}

			if (textManagerState.text.at(i) != L'\r')
			{
				RenderItem(bufferDC, curX, curY, textManagerState.text.at(i));
			}

			currentLine.charsStartX.push_back(curX);
			curX += size.cx;

			if (textManagerState.caretPosition == (i + 1))
			{
				SetCaretPos(curX, curY);
			}

			if (i + 1 == textManagerState.text.size())
			{
				lines.push_back(currentLine);
			}
		}
	}

	if (lines.size() == 0)
	{
		currentLine.startY = curY;
		currentLine.endY = nextLineY;
		lines.push_back(currentLine);
	}

	BitBlt(hdc, 0, 0, width, height, bufferDC, 0, 0, SRCCOPY);
	DeleteObject(bitmap);
	DeleteDC(bufferDC);
	ShowCaret(hWnd);
}


void TextManager::GetItemSize(HDC hdc, TCHAR item, SIZE &size)
{
	ZeroMemory(&size, sizeof(size));
	if ((item >> 8) == 0xff)
	{
		//Gdiplus::Bitmap *bitmap = imagesManager->GetImage(item & 0xff);
		//size.cx = bitmap->GetWidth() * scale;
		//size.cy = bitmap->GetHeight() * scale;
	}
	else if ((item >> 8) == 0xfe)
	{
		size.cx = 0;
		size.cy = 0;
	}
	else
	{
		GetTextExtentPoint32(hdc, &item, 1, &size);
	}

	if (item == L'\r' || (item >> 8) == 0xfe)
	{
		size.cx = 0;
	}
}


void TextManager::RenderItem(HDC hdc, int x, int y, TCHAR item)
{
	if ((item >> 8) == 0xff)
	{
		//DrawImage(hdc, x, y, imagesManager->GetImage(item & 0xff));
	}
	else if ((item >> 8) == 0xfe)
	{
		if (item & 0xff)
		{
			SelectObject(hdc, fontDescriptors[1]);
		}
		else
		{
			SelectObject(hdc, fontDescriptors[item & 0xff]);
		}
	}
	else
	{
		TextOut(hdc, x, y, &item, 1);
		SelectObject(hdc, fontDescriptors[1]);
	}
}

void TextManager::ApplyScale() 
{
	for (unsigned int i = 0; i < fontDescriptors.size(); ++i)
	{
		DeleteObject(fontDescriptors.at(i));
	}
	fontDescriptors.clear();

	std::wstring fontNames[] = {L"Comic Sans MS", L"Courier New"};
	for (int i = 0; i < 2; ++i)
	{
		LOGFONT logFont;
		ZeroMemory(&logFont, sizeof(LOGFONT));
		logFont.lfHeight = (LONG) (16 * scale);
		wcscpy_s(logFont.lfFaceName, fontNames[i].c_str());
		fontDescriptors.push_back(CreateFontIndirect(&logFont));
	}
	LeaveFocus();
	EnterFocus();
	InvalidateRect(hWnd, NULL, false);
}


void TextManager::MouseUp(int x, int y)
{
	/*
	if (dragImageIndex != -1)
	{
		TCHAR imageTag = textManagerState.text.at(dragImageIndex);
		textManagerState.text.erase(dragImageIndex, 1);
		textManagerState.caretPosition = max(dragImageIndex < textManagerState.caretPosition ? textManagerState.caretPosition - 1 : textManagerState.caretPosition, 0);
		Insert(imageTag);
		dragImageIndex = -1;
	}
	else*/ if (textManagerState.startSelection == textManagerState.endSelection
		|| textManagerState.startSelection == -1
		|| textManagerState.endSelection == -1)
	{
		textManagerState.caretPosition = textManagerState.startSelection;
		textManagerState.endSelection = -1;
		textManagerState.startSelection = -1;
	}
	
	//historyManager.SetCurrentState(currentState);
	InvalidateRect(hWnd, NULL, false);
}

void TextManager::MouseDown(int x, int y)
{
	textManagerState.endSelection = -1;
	textManagerState.startSelection = -1;
	int charIndex = 0;
	textManagerState.startSelection = GetCursorPos(x, y, charIndex);
	if (!textManagerState.text.empty() &&(textManagerState.text.at(charIndex) >> 8) == 0xff)
	{
		//dragImageIndex = charIndex;
	}
	ShowCaret(hWnd);
}

void TextManager::MouseWheelDown()
{
	scale = max(scale - 0.1, 0.5);
	TextManager::ApplyScale();
}

void TextManager::MouseWheelUp()
{
	scale = min(scale + 0.1, 2);
	TextManager::ApplyScale();
}

void TextManager::EnterFocus()
{
	CreateCaret(hWnd, (HBITMAP) NULL, 1, 16 * scale);
	ShowCaret(hWnd);
}

void TextManager::LeaveFocus()
{
	DestroyCaret();
}

int TextManager::GetCursorPos(int x, int y, int &outIndex)
{
	Line *line = &lines.at(0);
	int position = 0;
	for (UINT i = 0; i < lines.size(); ++i)
	{
		line = &lines[i];
		if (lines[i].startY <= y && lines[i].endY >= y)
		{
			break;
		}
		else if (i + 1 != lines.size())
		{
			position += lines[i].charsStartX.size();
		}
	}

	for (int i = 0; i < ((int) line->charsStartX.size() - 1); ++i)
	{
		if (line->charsStartX[i + 1] - line->charsStartX[i] == 0)
		{
			continue;
		}
		if (line->charsStartX[i] <= x && line->charsStartX[i + 1] >= x)
		{
			int charMid = (line->charsStartX[i + 1] + line->charsStartX[i]) / 2;
			outIndex = position + i;
			return position + (x > charMid ? ++i : i);
		}
	}
	outIndex = textManagerState.text.size() - 1;
	return max((lines.size() == 1 ? 0 : position) + (int)line->charsStartX.size(), 0);
}