#include <stdint.h>
#include <windows.h>
#include <winuser.h>

#define internal static
#define local_persist static
#define global_variable static

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

///////////////////////////////////////////////////////////////
/// Structs
//////////////////////////////////////////////////////////////

struct win32_offscreen_buffer {
  BITMAPINFO Info;
  void *Memory;
  int Width;
  int Height;
  int Pitch;
  int BytesPerPixel = 4;
};

struct win32_window_dimension {
  int Width;
  int Height;
};

win32_window_dimension Win32GetWindowDimension(HWND Window) {
  win32_window_dimension Result;
  RECT ClientRect;
  GetClientRect(Window, &ClientRect);
  Result.Width = ClientRect.right - ClientRect.left;
  Result.Height = ClientRect.bottom - ClientRect.top;

  return (Result);
}

///////////////////////////////////////////////////////////////
/// Variables
//////////////////////////////////////////////////////////////

global_variable bool Running;
global_variable win32_offscreen_buffer GlobalBackBuffer;

///////////////////////////////////////////////////////////////
/// Functions
///////////////////////////////////////////////////////////////

internal void RenderWeirdGradient(win32_offscreen_buffer Buffer, int XOffset,
                                  int YOffset) {
  int Width = Buffer.Width;
  int Height = Buffer.Height;
  uint8 *Row = (uint8 *)Buffer.Memory;
  for (int Y = 0; Y < Buffer.Height; ++Y) {
    uint32 *Pixel = (uint32 *)Row;
    for (int X = 0; X < Buffer.Width; ++X) {
      uint8 Blue = (X + XOffset);
      uint8 Green = (Y + YOffset);

      *Pixel++ = ((Green << 8) | Blue);
    }
    Row += Buffer.Pitch;
  }
}

///////////////////////////////////////////////////////////////
/// Win32ResizeDIBSection
///////////////////////////////////////////////////////////////

internal void Win32ResizeDIBSection(win32_offscreen_buffer *Buffer, int Width,
                                    int Height) {

  if (Buffer->Memory) {
    VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
  }
  Buffer->Width = Width;
  Buffer->Height = Height;
  Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
  Buffer->Info.bmiHeader.biWidth = Buffer->Width;
  Buffer->Info.bmiHeader.biHeight = -Buffer->Height;
  Buffer->Info.bmiHeader.biPlanes = 1;
  Buffer->Info.bmiHeader.biBitCount = 32;
  Buffer->Info.bmiHeader.biCompression = BI_RGB;

  int BitmapMemorySize =
      Buffer->BytesPerPixel * (Buffer->Width * Buffer->Height);
  Buffer->Memory =
      VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);
  Buffer->Pitch = Buffer->Width * Buffer->BytesPerPixel;
}

internal void Win32DisplayBufferInWindow(HDC DeviceContext, int WindowWidth,
                                         int WindowHeight,
                                         win32_offscreen_buffer Buffer, int X,
                                         int Y, int Width, int Height) {

  StretchDIBits(DeviceContext, 0, 0, WindowWidth, WindowHeight, 0, 0,
                Buffer.Width, Buffer.Height, Buffer.Memory, &Buffer.Info,
                DIB_RGB_COLORS, SRCCOPY);
}

///////////////////////////////////////////////////////////////
/// Win32MainWindowCallback
//////////////////////////////////////////////////////////////

LRESULT CALLBACK Win32MainWindowCallback(HWND Window, UINT Message,
                                         WPARAM WParam, LPARAM LParam) {
  LRESULT Result = 0;

  switch (Message) {
  case WM_SIZE: {
    OutputDebugStringA("WM_SIZE\n");
  } break;

  case WM_DESTROY: {
    Running = false;
    OutputDebugStringA("WM_DESTROY\n");
  } break;

  case WM_CLOSE: {
    Running = false;
    OutputDebugStringA("WM_CLOSE n");
  } break;

  case WM_ACTIVATEAPP: {
    OutputDebugStringA("WM_ACTIVATEAPP\n");
  } break;

  case WM_PAINT: {
    PAINTSTRUCT Paint;
    HDC DeviceContext = BeginPaint(Window, &Paint);
    int X = Paint.rcPaint.left;
    int Y = Paint.rcPaint.top;
    int Width = Paint.rcPaint.right - Paint.rcPaint.left;
    int Height = Paint.rcPaint.bottom - Paint.rcPaint.top;

    win32_window_dimension Dimension = Win32GetWindowDimension(Window);
    Win32DisplayBufferInWindow(DeviceContext, Dimension.Width, Dimension.Height,
                               GlobalBackBuffer, X, Y, Width, Height);
    EndPaint(Window, &Paint);
  } break;
  default: {
    /*OutputDebugStringA("default");*/
    Result = DefWindowProc(Window, Message, WParam, LParam);
  } break;
  }
  return (Result);
}

///////////////////////////////////////////////////////////////
/// WinMain
//////////////////////////////////////////////////////////////

int CALLBACK WinMain(HINSTANCE Instance, HINSTANCE PrevInstance,
                     LPSTR CommandLine, int ShowCode) {

  WNDCLASSA WindowClass = {};

  Win32ResizeDIBSection(&GlobalBackBuffer, 1280, 720);
  WindowClass.style = CS_HREDRAW | CS_VREDRAW;
  WindowClass.lpfnWndProc = Win32MainWindowCallback;
  WindowClass.hInstance = Instance;
  WindowClass.lpszClassName = "HandmadeHeroWindowClass";

  if (RegisterClass(&WindowClass)) {
    HWND Window = CreateWindowEx(0, WindowClass.lpszClassName, "Handmade Hero",
                                 WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                 CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                                 CW_USEDEFAULT, 0, 0, Instance, 0);
    if (Window) {
      Running = true;
      MSG Message;
      int XOffset = 0;
      int YOffset = 0;
      while (Running) {
        while (PeekMessage(&Message, 0, 0, 0, PM_REMOVE)) {
          if (Message.message == WM_QUIT) {
            Running = false;
          }
          TranslateMessage(&Message);
          DispatchMessage(&Message);
        }

        HDC DeviceContext = GetDC(Window);
        win32_window_dimension Dimension = Win32GetWindowDimension(Window);
        RenderWeirdGradient(GlobalBackBuffer, XOffset, YOffset);
        Win32DisplayBufferInWindow(DeviceContext, Dimension.Width,
                                   Dimension.Height, GlobalBackBuffer, 0, 0,
                                   Dimension.Width, Dimension.Height);
        ReleaseDC(Window, DeviceContext);
        ++XOffset;
        // YOffset += 2;
      }
    }
  } else {
  }
  return (0);
}
