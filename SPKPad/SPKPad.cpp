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

	// Main message loop:
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int) msg.wParam;
}


ATOM MyRegisterClass(HINSTANCE hInstance) 
{
	WNDCLASSEX wcex;

    wcex.cbSize			= sizeof(WNDCLASSEX);
    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, _T("Ext4_Icon"));
    wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = _T("IDC_MENU1");
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
	case WM_CHAR:
		Wm_Char(wParam);
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
	case WM_MOUSEWHEEL:
		(GET_WHEEL_DELTA_WPARAM(wParam) > 0) ? textManager->MouseWheelUp() : textManager->MouseWheelDown();
		break;
	case WM_COMMAND:
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
