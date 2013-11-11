#include "Header.h"

using namespace std;

TextManager::TextManager(HWND hWnd)
{
	TextManager::hWnd = hWnd;

	textManagerState.caretPosition = 0;
	textManagerState.endSelection = -1;
	textManagerState.startSelection = -1;
	imageIndex = -1;
	scale = 1;

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

	for (int i = 0; i < images.size(); ++i)
	{
		delete images.at(i);
	}
	images.clear();
}


void TextManager::InsertChar(TCHAR char_code)
{
	SaveInHistory();
	DeleteSelection();
	textManagerState.caretPosition = min(textManagerState.caretPosition, textManagerState.text.size());
	textManagerState.caretPosition = max(textManagerState.caretPosition, 0);
	textManagerState.text.insert(textManagerState.caretPosition++, &char_code, 1);
	InvalidateRect(hWnd, NULL, false);
}

void TextManager::InsertChars(const std::wstring &str)
{
	SaveInHistory();
	DeleteSelection();
	textManagerState.text.insert(textManagerState.caretPosition, str);
	textManagerState.caretPosition += str.size();
	InvalidateRect(hWnd, NULL, false);
}

void TextManager::RemoveChars()
{
	SaveInHistory();
	if (textManagerState.startSelection != -1 && textManagerState.endSelection != -1)
	{
		DeleteSelection();
		InvalidateRect(hWnd, NULL, false);
		return;
	}
	--textManagerState.caretPosition;
	while (textManagerState.caretPosition > 0 && (textManagerState.text.at(textManagerState.caretPosition - 1) >> 8) == FONT)
	{
		textManagerState.text.erase(--textManagerState.caretPosition, 1);
	}
	++textManagerState.caretPosition;
	if (textManagerState.caretPosition > 0)
	{
		textManagerState.text.erase(--textManagerState.caretPosition, 1);	
	}
	textManagerState.caretPosition = min(textManagerState.caretPosition, textManagerState.text.size() - 1);
	InvalidateRect(hWnd, NULL, false);
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

	int curX = 0;
	int curY = 0;
	int nextLineY = 0;

	Line currentLine;
	
	lines.clear();
	
	SelectObject(bufferDC, fontDescriptors[1]);
	SetCaretPos(0, 0);

	SIZE size;
	for (UINT i = 0; i < textManagerState.text.length(); ++i)
	{
		GetItemSize(bufferDC, textManagerState.text.at(i), size);
		nextLineY = max(nextLineY, curY + size.cy);
		
		if (curX + size.cx >= width || textManagerState.text.at(i) == L'\r')
		{
			currentLine.startY = curY;
			currentLine.endY = nextLineY;
			curX = 0;
			curY = nextLineY;
			lines.push_back(currentLine);
			currentLine.charsStartX.clear();
		}

		if (i >= min(textManagerState.startSelection, textManagerState.endSelection)
			&& i < max(textManagerState.startSelection, textManagerState.endSelection))
		{
			SetTextColor(bufferDC, RGB(255, 255, 255));
			SetBkColor(bufferDC, RGB(100, 100, 100));
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
			currentLine.startY = curY;
			currentLine.endY = nextLineY;
			lines.push_back(currentLine);
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
	if ((item >> 8) == IMAGE)
	{
		Gdiplus::Bitmap *bitmap = images.at(item & IMAGE);
		size.cx = bitmap->GetWidth() * scale;
		size.cy = bitmap->GetHeight() * scale;
	}
	else if ((item >> 8) == FONT)
	{
		size.cx = 0;
		size.cy = 0;
	}
	else
	{
		GetTextExtentPoint32(hdc, &item, 1, &size);
	}

	if (item == L'\r' || (item >> 8) == FONT)
	{
		size.cx = 0;
	}
}


void TextManager::RenderItem(HDC hdc, int x, int y, TCHAR item)
{
	if ((item >> 8) == IMAGE)
	{
		Gdiplus::Bitmap* image = images.at(item & IMAGE);
		HBITMAP hBitmap;
		image->GetHBITMAP(Gdiplus::Color::White, &hBitmap);

		HDC memoryDC = CreateCompatibleDC(hdc);
		SelectObject(memoryDC, hBitmap);
		StretchBlt(hdc, x, y, image->GetWidth() * scale, image->GetHeight() * scale, memoryDC, 
			0, 0, image->GetWidth(), image->GetHeight(), SRCCOPY);
		DeleteDC(memoryDC);
	}
	else if ((item >> 8) == FONT)
	{
		if (item & IMAGE)
		{
			SelectObject(hdc, fontDescriptors[1]);
		}
		else
		{
			SelectObject(hdc, fontDescriptors[item & IMAGE]);
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
	SaveInHistory();
	
	if (imageIndex != -1)
	{
		TCHAR imageTag = textManagerState.text.at(imageIndex);
		textManagerState.text.erase(imageIndex, 1);
		textManagerState.caretPosition = max(imageIndex < textManagerState.caretPosition ? textManagerState.caretPosition - 1 : textManagerState.caretPosition, 0);
		InsertChar(imageTag);
		imageIndex = -1;
	}
	else if (textManagerState.startSelection == textManagerState.endSelection
		|| textManagerState.startSelection == -1
		|| textManagerState.endSelection == -1)
	{
		textManagerState.caretPosition = textManagerState.startSelection;
		textManagerState.endSelection = -1;
		textManagerState.startSelection = -1;
	}
	
	InvalidateRect(hWnd, NULL, false);
}

void TextManager::MouseDown(int x, int y)
{
	textManagerState.endSelection = -1;
	textManagerState.startSelection = -1;
	int charIndex = 0;
	textManagerState.startSelection = GetCursorPos(x, y, charIndex);
	if (!textManagerState.text.empty() && (textManagerState.text.at(charIndex) >> 8) == IMAGE)
	{
		imageIndex = charIndex;
	}
	ShowCaret(hWnd);
}

void TextManager::MouseDoubleClick()
{
	int currentIndex = textManagerState.caretPosition;
	currentIndex = min(currentIndex, textManagerState.text.size() - 1);
	currentIndex = max(currentIndex, 0);
	
	TCHAR ch;
	while (!textManagerState.text.empty() && !((ch = textManagerState.text.at(currentIndex)) == ' ' ||
						(ch >> 8) == IMAGE || ch == '\n' || ch == '\r' || ch == '\t' || ch == ',' || ch == '.'))
	{
		textManagerState.endSelection = currentIndex + 1;
		currentIndex++;
		if (currentIndex >= textManagerState.text.size())
		{
			break;
		}
	}

	currentIndex = min(textManagerState.caretPosition, textManagerState.text.size() - 1);
	while (!textManagerState.text.empty() && !((ch = textManagerState.text.at(currentIndex)) == ' ' ||
						(ch >> 8) == IMAGE || ch == '\n' || ch == '\r' || ch == '\t' || ch == ',' || ch == '.'))
	{
		textManagerState.startSelection = currentIndex;
		currentIndex--;
		if (currentIndex < 0)
		{
			break;
		}
	}

	textManagerState.caretPosition = textManagerState.endSelection;

	InvalidateRect(hWnd, NULL, false);
}

void TextManager::MouseMove(int x, int y, bool leftButtonPressed)
{
	if (leftButtonPressed)
	{
		if (imageIndex != -1)
		{
			int charIndex = 0;
			textManagerState.caretPosition = GetCursorPos(x, y, charIndex);
			InvalidateRect(hWnd, NULL, false);
		}
		else
		{
			int charIndex = 0;
			textManagerState.caretPosition = textManagerState.endSelection = GetCursorPos(x, y, charIndex);
			InvalidateRect(hWnd, NULL, false);
		}
	}
}

void TextManager::MouseWheelDown()
{
	scale = max(scale - 0.1, 0.5);
	TextManager::ApplyScale();
}

void TextManager::MouseWheelUp()
{
	scale = min(scale + 0.1, 5);
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

int TextManager::GetCursorPos(int x, int y, int &charIndex)
{
	int position = 0;
	for (UINT i = 0; i < lines.size(); i++)
	{
		if (lines[i].startY <= y && lines[i].endY >= y)
		{
			for (int j = 0; j < (lines[i].charsStartX.size() - 1); j++)
			{
				if (lines[i].charsStartX[j] <= x && lines[i].charsStartX[j + 1] >= x)
				{
					charIndex = position + j;
					return position + j + 
						(x > (lines[i].charsStartX[j+1]+lines[i].charsStartX[j])/2 ? 1 : 0);
				}
			}
			return position + lines[i].charsStartX.size();
		}
		
		position += lines[i].charsStartX.size();
	}

	charIndex = textManagerState.text.size() - 1;
	return position;
}

void TextManager::ApplyFont(int fontCode)
{
	SaveInHistory();
	if (textManagerState.startSelection == -1 || textManagerState.endSelection == -1)
	{
		return;
	}
	int startPos = min(textManagerState.startSelection, textManagerState.endSelection);
	int endPos = max(textManagerState.startSelection, textManagerState.endSelection);
	TCHAR fontSymbol = 0xfe00 + fontCode;
	for (int i = startPos; i < endPos; ++endPos, i += 2)
	{
		textManagerState.text.insert(i, &fontSymbol, 1);
	}

	textManagerState.caretPosition = endPos;
	textManagerState.startSelection = -1;
	textManagerState.endSelection = -1;
	
	InvalidateRect(hWnd, NULL, false);
}

void TextManager::InsertImage()
{
	OPENFILENAME openFileStruct = {0};
	TCHAR filename[256] = {0};

	openFileStruct.lStructSize = sizeof(openFileStruct);
	openFileStruct.lpstrFile = filename;
	openFileStruct.nMaxFile = sizeof(filename);

	GetOpenFileName(&openFileStruct);
	std::wstring filePath((LPWSTR) &filename);

	if (!filePath.empty())
	{
		Gdiplus::Bitmap *image = new Gdiplus::Bitmap(filePath.c_str());
		images.push_back(image);
		InsertChar((TCHAR) (0xff00 + images.size() - 1));
	}
}

void TextManager::SaveInHistory()
{
	undoDeque.push_front(textManagerState);
	while (undoDeque.size() > 10)
	{
		undoDeque.pop_back();
	}
	redoDeque.clear();
}

void TextManager::Undo()
{
	if (!undoDeque.empty())
	{
		redoDeque.push_front(textManagerState);
		textManagerState = undoDeque.front();
		undoDeque.pop_front();
	}
	InvalidateRect(hWnd, NULL, false);
}

void TextManager::Redo()
{
	if (!redoDeque.empty())
	{
		undoDeque.push_front(textManagerState);
		textManagerState = redoDeque.front();
		redoDeque.pop_front();
	}
	InvalidateRect(hWnd, NULL, false);
}

std::wstring TextManager::GetSelectedText()
{
	if (textManagerState.startSelection == -1 || textManagerState.endSelection == -1 || textManagerState.text.empty())
	{
		return L"";
	}

	int startPos = min(textManagerState.startSelection, textManagerState.endSelection);
	int endPos = max(textManagerState.startSelection, textManagerState.endSelection);
	
	std::wstring result(textManagerState.text, startPos, endPos - startPos);
	return result;
}


void TextManager::SaveFile()
{
	OPENFILENAME openFileStruct = {0};
	TCHAR filename[256] = {0};

	openFileStruct.lStructSize = sizeof(openFileStruct);
	openFileStruct.lpstrFile = filename;
	openFileStruct.nMaxFile = sizeof(filename);

	GetSaveFileName(&openFileStruct);
	
	HANDLE file = CreateFile(filename, GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	SetEndOfFile(file);

	DWORD bytesWritten;

	unsigned int imagesCount = images.size();
	WriteFile(file, &imagesCount, sizeof(imagesCount), &bytesWritten, NULL);

	for (int i = 0; i < imagesCount; ++i)
	{
		Gdiplus::Bitmap *bitmap = images[i];
		DWORD qqq;
		HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, 0);
	
		CComPtr<IStream> stream;
		CreateStreamOnHGlobal(hGlobal, FALSE, &stream);

		CLSID pngClsid;
		GetEncoderClsid(L"image/png", &pngClsid);
		bitmap->Save(stream, &pngClsid);

		LPVOID buffer = GlobalLock(hGlobal);
		unsigned int imageSize = GlobalSize(hGlobal);

		WriteFile(file, &imageSize, sizeof(imageSize), &qqq, NULL);
		WriteFile(file, buffer, imageSize, &qqq, NULL);

		GlobalUnlock(hGlobal);

		GlobalFree(hGlobal);
	}
	unsigned int textSize = textManagerState.text.size() * sizeof(wchar_t);
	WriteFile(file, &textSize, sizeof(textSize), &bytesWritten, NULL);

	WriteFile(file, textManagerState.text.c_str(), textSize, &bytesWritten, NULL);

	CloseHandle(file);
}

void TextManager::LoadFile()
{
	OPENFILENAME openFileStruct = {0};
	TCHAR filename[256] = {0};

	openFileStruct.lStructSize = sizeof(openFileStruct);
	openFileStruct.lpstrFile = filename;
	openFileStruct.nMaxFile = sizeof(filename);

	GetOpenFileName(&openFileStruct);

	HANDLE file = CreateFile(filename, GENERIC_READ, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	DWORD bytesReaden;
	unsigned int countImages = 0;
	ReadFile(file, &countImages, sizeof(countImages), &bytesReaden, NULL);

	for (int i = 0; i < images.size(); ++i)
	{
		delete images[i];
	}

	images.clear();

	for(int i = 0; i < countImages; i++)
	{
		unsigned int imageSize = 0;
		ReadFile(file, &imageSize, sizeof(imageSize), &bytesReaden, NULL);
		HGLOBAL hGlobal = GlobalAlloc(GMEM_FIXED, imageSize);
		void *imageData = GlobalLock(hGlobal);
		ReadFile(file, imageData, imageSize, &bytesReaden, NULL);
		GlobalUnlock(hGlobal);

		CComPtr<IStream> stream;
		CreateStreamOnHGlobal(hGlobal, FALSE, &stream);

		Gdiplus::Bitmap *bitmap = new Gdiplus::Bitmap(stream);
		images.push_back(bitmap);

		GlobalFree(hGlobal);
	}
	unsigned textSize = 0;

	ReadFile(file, &textSize, sizeof(textSize), &bytesReaden, NULL);

	TCHAR *massSimbol = new TCHAR[(textSize / 2) + 1];
	massSimbol[(textSize / 2)] = 0;
	ReadFile(file, massSimbol,textSize, &bytesReaden, NULL);
	textManagerState.text = std::wstring(massSimbol);
	delete[] massSimbol;

	CloseHandle(file);
	InvalidateRect(hWnd, NULL, true);
}

void TextManager::GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
{
   UINT  num = 0;          // number of image encoders
   UINT  size = 0;         // size of the image encoder array in bytes

   Gdiplus::ImageCodecInfo* pImageCodecInfo = NULL;

   Gdiplus::GetImageEncodersSize(&num, &size);
   if(size == 0)
      return;  // Failure

   pImageCodecInfo = (Gdiplus::ImageCodecInfo*)(malloc(size));
   if(pImageCodecInfo == NULL)
      return;  // Failure

   GetImageEncoders(num, size, pImageCodecInfo);

   for(UINT j = 0; j < num; ++j)
   {
      if( wcscmp(pImageCodecInfo[j].MimeType, format) == 0 )
      {
         *pClsid = pImageCodecInfo[j].Clsid;
         free(pImageCodecInfo);
         return;  // Success
      }
   }

   free(pImageCodecInfo);
   return;  // Failure
}
