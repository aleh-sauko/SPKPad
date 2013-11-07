#include "Header.h"

static TCHAR szWindowClass[] = _T("win32app");
static TCHAR szTitle[] = _T("SPKPad");


HINSTANCE hInst;
HWND hWnd;
TextManager* textManager;

// Forward declarations of functions included in this code module:

ATOM MyRegisterClass(HINSTANCE hInstance);
BOOL InitInstance(HINSTANCE, int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow)
{
	MyRegisterClass(hInstance);

	if (!InitInstance(hInstance, nCmdShow))
	{
		return 1;
	}

	HACCEL hAccelTable;
	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_ADVANSEDTEXTEDITOR));

	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR           gdiplusToken;
	Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

	// Main message loop:
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
    }

	Gdiplus::GdiplusShutdown(gdiplusToken);

    return (int) msg.wParam;
}


ATOM MyRegisterClass(HINSTANCE hInstance) 
{
	WNDCLASSEX wcex;

    wcex.cbSize			= sizeof(WNDCLASSEX);
    wcex.style          = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, _T("Ext4_Icon"));
    wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCE(IDC_ADVANSEDTEXTEDITOR);
    wcex.lpszClassName  = szWindowClass;
	wcex.hIconSm        = LoadIcon(wcex.hInstance, _T("Ext4_Icon"));

	return RegisterClassEx(&wcex);
}


BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	hInst = hInstance; 
    
	hWnd = CreateWindow(
        szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        500, 400, NULL, NULL, hInstance, NULL
    );

    if (!hWnd)
    {
		return FALSE;
    }

	textManager = new TextManager(hWnd);

	ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

	return TRUE;
}


void Wm_Paint(HDC hdc);
void Wm_Char(TCHAR char_code);
void Wm_Keydown(TCHAR key_code);
void Wm_Command(UINT commandId);

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    HDC hdc;

    switch (message)
    {
	case WM_CREATE:
		UpdateWindow(hWnd);
		CreateCaret(hWnd, NULL, 1, 16);
		SetCaretPos(0, 0);
		break;
	case WM_COMMAND:
		Wm_Command(LOWORD(wParam));
		break;
	case WM_CHAR:
		Wm_Char(wParam);
		break;
	case WM_KEYDOWN:							
		Wm_Keydown(wParam);
		break;
    case WM_PAINT:
        hdc = BeginPaint(hWnd, &ps);
		Wm_Paint(hdc);
        EndPaint(hWnd, &ps);
        break;
	case WM_LBUTTONUP:
		textManager->MouseUp(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		break;
	case WM_LBUTTONDOWN:
		textManager->MouseDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		break;
	case WM_LBUTTONDBLCLK:
		textManager->MouseDoubleClick();
		break;
	case WM_MOUSEMOVE:
		SetFocus(hWnd);
		textManager->MouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), wParam & MK_LBUTTON);
		break;
	case WM_MOUSEWHEEL:
		(GET_WHEEL_DELTA_WPARAM(wParam) > 0) ? textManager->MouseWheelUp() : textManager->MouseWheelDown();
		break;
	case WM_SETFOCUS:
		textManager->EnterFocus();
		break;
	case WM_KILLFOCUS:
		textManager->LeaveFocus();
		break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
        break;
    }

    return 0;
}


void Wm_Paint(HDC hdc)
{
	RECT rect;
	GetClientRect(hWnd, &rect);
	textManager->RenderText(hdc, &rect);
}

void Wm_Char(TCHAR char_code)
{
	if (char_code == '\b')
	{
		textManager->RemoveChars();
	}
	else
	{
		textManager->InsertChar(char_code);
	}
	InvalidateRect(hWnd, NULL, false);
}


void Wm_Keydown(TCHAR key_code) 
{ 
	switch (key_code) 
	{
		case VK_DELETE:
			textManager->RemoveChars(); 
			break;
		/*case VK_LEFT:
			break;
		case VK_RIGHT:
			break;
		case VK_UP:
			break;
		case VK_DOWN:
			break;*/
		default:	
			break; 
	}
} 

void SaveToClipboard(const std::wstring &str);
std::wstring ReadFromClipboard();

void Wm_Command(UINT commandId)
{
	switch (commandId)
	{
	case ID_FILE_OPEN:
		textManager->LoadFile();
		break;
	case ID_FILE_SAVE:
		textManager->SaveFile();
		break;
	case ID_ACCELERATOR_CUT:
		SaveToClipboard(textManager->GetSelectedText());
		textManager->DeleteSelection();
		InvalidateRect(hWnd, NULL, false);
		break;
	case ID_ACCELERATOR_COPY:
		SaveToClipboard(textManager->GetSelectedText());
		break;
	case ID_ACCELERATOR_PASTE:
		{
		std::wstring text = ReadFromClipboard();
		if (!text.empty())
		{
			textManager->InsertChars(text);
			InvalidateRect(hWnd, NULL, false);
		}
		}
		break;
	case ID_ACCELERATOR_UNDO:
		textManager->Undo();
		break;
	case ID_ACCELERATOR_REDO:
		textManager->Redo();
		break;
	case ID_FONT_ARIAL:
		textManager->ApplyFont(0);
		break;
	case ID_FONT_COURIER:
		textManager->ApplyFont(1);
		break;
	case ID_INSERT_IMAGE:
		textManager->InsertImage();
		break;
	default:
		break;
	}
}

void SaveToClipboard(const std::wstring &str)
{
	if (!OpenClipboard(hWnd))
	{
		return;
	}
	EmptyClipboard();

	HGLOBAL globalMemDescriptor = GlobalAlloc(GMEM_MOVEABLE, (str.size() + 1) * sizeof(TCHAR));
	if (globalMemDescriptor != NULL) 
    {
		LPWSTR stringPtr = (LPWSTR) GlobalLock(globalMemDescriptor);
		memcpy(stringPtr, str.c_str(), str.size() * sizeof(TCHAR));
		stringPtr[str.size()] = 0;
		GlobalUnlock(globalMemDescriptor);
		SetClipboardData(CF_UNICODETEXT, globalMemDescriptor);
    }
	CloseClipboard();
}

std::wstring ReadFromClipboard()
{
	if (!IsClipboardFormatAvailable(CF_UNICODETEXT) || !OpenClipboard(hWnd))
	{
		return L"";
	}
	
	std::wstring result;

	HGLOBAL globalMemDescriptor = GetClipboardData(CF_UNICODETEXT);
	if (globalMemDescriptor != NULL)
	{
		result = (LPWSTR) GlobalLock(globalMemDescriptor);
		GlobalUnlock(globalMemDescriptor);
	}
	CloseClipboard();
	return result;
}