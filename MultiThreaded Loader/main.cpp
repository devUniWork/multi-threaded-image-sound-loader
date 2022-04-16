// INCLUDES -------------------------------------
#include <Windows.h>
#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include "resource.h"
#include <SDL.h>
#include <iostream>

// CONFIGURATION --------------------------------
#define WINDOW_CLASS_NAME L"MultiThreaded Loader Tool"
const unsigned int _kuiWINDOWWIDTH = 512;
const unsigned int _kuiWINDOWHEIGHT = 570;
#define MAX_FILES_TO_OPEN 50
#define MAX_CHARACTERS_IN_FILENAME 25

// GLOBAL DATA ----------------------------------
std::vector<std::wstring> g_vecImageFileNames;
std::vector<std::wstring> g_vecSoundFileNames;
std::vector<HBITMAP> g_vecLoadedImages;
std::vector<LPCWSTR> g_vecLoadedSounds;
std::mutex g_LoadImageIntoImages_mutex;
HINSTANCE g_hInstance;
bool g_bIsFileLoaded = false;

/*
	This function populates g_vecImageFileNames with the
	file paths that we want to load.
*/
bool ChooseImageFilesToLoad(HWND _hwnd)
{
	OPENFILENAME ofn;
	SecureZeroMemory(&ofn, sizeof(OPENFILENAME)); // Better to use than ZeroMemory
	wchar_t wsFileNames[MAX_FILES_TO_OPEN * MAX_CHARACTERS_IN_FILENAME + MAX_PATH]; //The string to store all the filenames selected in one buffer togther with the complete path name.
	wchar_t _wsPathName[MAX_PATH + 1];
	wchar_t _wstempFile[MAX_PATH + MAX_CHARACTERS_IN_FILENAME]; //Assuming that the filename is not more than 20 characters
	wchar_t _wsFileToOpen[MAX_PATH + MAX_CHARACTERS_IN_FILENAME];
	ZeroMemory(wsFileNames, sizeof(wsFileNames));
	ZeroMemory(_wsPathName, sizeof(_wsPathName));
	ZeroMemory(_wstempFile, sizeof(_wstempFile));

	//Fill out the fields of the structure
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = _hwnd;
	ofn.lpstrFile = wsFileNames;
	ofn.nMaxFile = MAX_FILES_TO_OPEN * 20 + MAX_PATH;  //The size, in charactesr of the buffer pointed to by lpstrFile. The buffer must be atleast 256(MAX_PATH) characters long; otherwise GetOpenFileName and 
													   //GetSaveFileName functions return False
													   // Set lpstrFile[0] to '\0' so that GetOpenFileName does not 
													   // use the contents of wsFileNames to initialize itself.
	ofn.lpstrFile[0] = '\0';
	ofn.lpstrFilter = L"Bitmap Images(.bmp)\0*.bmp\0"; //Filter for bitmap images
	ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT;

	//If the user makes a selection from the  open dialog box, the API call returns a non-zero value
	if (GetOpenFileName(&ofn) != 0) //user made a selection and pressed the OK button
	{
		//Extract the path name from the wide string -  two ways of doing it
		//First way: just work with wide char arrays
		wcsncpy_s(_wsPathName, wsFileNames, ofn.nFileOffset);
		int i = ofn.nFileOffset;
		int j = 0;

		while (true)
		{
			if (*(wsFileNames + i) == '\0')
			{
				_wstempFile[j] = *(wsFileNames + i);
				wcscpy_s(_wsFileToOpen, _wsPathName);
				wcscat_s(_wsFileToOpen, L"\\");
				wcscat_s(_wsFileToOpen, _wstempFile);
				g_vecImageFileNames.push_back(_wsFileToOpen);
				j = 0;
			}
			else
			{
				_wstempFile[j] = *(wsFileNames + i);
				j++;
			}
			if (*(wsFileNames + i) == '\0' && *(wsFileNames + i + 1) == '\0')
			{
				break;
			}
			else
			{
				i++;
			}

		}

		g_bIsFileLoaded = true;
		return true;
	}
	else // user pressed the cancel button or closed the dialog box or an error occured
	{
		return false;
	}

}
bool ChooseSoundFilesToLoad(HWND _hwnd)
{
	OPENFILENAME ofn;
	SecureZeroMemory(&ofn, sizeof(OPENFILENAME)); // Better to use than ZeroMemory
	wchar_t wsFileNames[MAX_FILES_TO_OPEN * MAX_CHARACTERS_IN_FILENAME + MAX_PATH]; //The string to store all the filenames selected in one buffer togther with the complete path name.
	wchar_t _wsPathName[MAX_PATH + 1];
	wchar_t _wstempFile[MAX_PATH + MAX_CHARACTERS_IN_FILENAME]; //Assuming that the filename is not more than 20 characters
	wchar_t _wsFileToOpen[MAX_PATH + MAX_CHARACTERS_IN_FILENAME];
	ZeroMemory(wsFileNames, sizeof(wsFileNames));
	ZeroMemory(_wsPathName, sizeof(_wsPathName));
	ZeroMemory(_wstempFile, sizeof(_wstempFile));

	//Fill out the fields of the structure
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = _hwnd;
	ofn.lpstrFile = wsFileNames;
	ofn.nMaxFile = MAX_FILES_TO_OPEN * 20 + MAX_PATH;  //The size, in charactesr of the buffer pointed to by lpstrFile. The buffer must be atleast 256(MAX_PATH) characters long; otherwise GetOpenFileName and 
													   //GetSaveFileName functions return False
													   // Set lpstrFile[0] to '\0' so that GetOpenFileName does not 
													   // use the contents of wsFileNames to initialize itself.
	ofn.lpstrFile[0] = '\0';
	ofn.lpstrFilter = L"Wave Files (*.wav)\0*.wav\0All Files (*.*)\0*.*\0"; //Filter for wav files
	ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT;

	//If the user makes a selection from the  open dialog box, the API call returns a non-zero value
	if (GetOpenFileName(&ofn) != 0) //user made a selection and pressed the OK button
	{
		//Extract the path name from the wide string -  two ways of doing it
		//Second way: work with wide strings and a char pointer 

		std::wstring _wstrPathName = ofn.lpstrFile;

		_wstrPathName.resize(ofn.nFileOffset, '\\');

		wchar_t *_pwcharNextFile = &ofn.lpstrFile[ofn.nFileOffset];

		while (*_pwcharNextFile)
		{
			std::wstring _wstrFileName = _wstrPathName + _pwcharNextFile;

			g_vecSoundFileNames.push_back(_wstrFileName);

			_pwcharNextFile += lstrlenW(_pwcharNextFile) + 1;
		}

		g_bIsFileLoaded = true;
		return true;
	}
	else // user pressed the cancel button or closed the dialog box or an error occured
	{
		return false;
	}

}

/*
	This function loads an image into g_vecLoadedImages.
*/
void LoadImageIntoImages(std::wstring pathToImageToLoad)
{
	HBITMAP loadedImage = HBITMAP( LoadImageW(NULL, (LPCWSTR)pathToImageToLoad.c_str(), IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE ));
	const bool loadFailed = loadedImage == NULL;
	if(loadFailed)
	{
		throw "Failed to load image :(.";

	}

	g_LoadImageIntoImages_mutex.lock();
	g_vecLoadedImages.push_back(loadedImage);
	g_LoadImageIntoImages_mutex.unlock();

	// tell windows to render bitmap



	// By the end here, we have loaded the image into g_vecLoadedImages.
}

bool PlaySoundsInParallel(std::wstring pathToSoundToLoad)
{
	bool play = bool(PlaySound((LPCWSTR)pathToSoundToLoad.c_str(), NULL, SND_FILENAME|SND_ASYNC));
	const bool loadFailed = play == NULL;
	if (loadFailed)
	{
		throw "Failed to sound image :(.";

	}
	return play;
}


void LoadSoundIntoSounds(std::wstring pathToSoundToLoad)
{


	bool(*fcnPtr1)(std::wstring){ &PlaySoundsInParallel };

	std::vector<std::thread> threads;
	for (int i = 0; i < 2; ++i) {
		threads.push_back(std::thread(PlaySoundsInParallel, std::ref(pathToSoundToLoad)));
	}

	for (int i = 0; i < 2 ; i++) {
		threads[i].join();
	}
}

void DrawImages(HWND _hwnd, HBITMAP loadedImage, int imageIndex, int bottomIndex)
{

		RECT rect;
		HDC hdc = GetDC(_hwnd);
		HBRUSH brush = CreatePatternBrush(loadedImage);
		rect.left = 0;

		if (imageIndex < 2) {
			SetRect(&rect, 256 * imageIndex, 0, 256 * imageIndex + 256, 256);
		}
		else {
			SetRect(&rect, 256 * bottomIndex, 256, 256 * bottomIndex + 256, 512);
		}


		FillRect(hdc, &rect, brush);
		DeleteObject(brush);
		ReleaseDC(_hwnd, hdc);
}

/*
	Handle messages from windows -- such as input to our menu.
*/
LRESULT CALLBACK WindowProc(HWND _hwnd, UINT _uiMsg, WPARAM _wparam, LPARAM _lparam)
{
	PAINTSTRUCT ps;
	HDC _hWindowDC;
	//RECT rect;
	switch (_uiMsg)
	{
	case WM_KEYDOWN:
	{
		switch (_wparam)
		{
		case VK_ESCAPE:
		{
			SendMessage(_hwnd, WM_CLOSE, 0, 0);
			return(0);
		}
		break;
		default:
			break;
		}
	}
	break;
	case WM_PAINT:
	{

		_hWindowDC = BeginPaint(_hwnd, &ps);
		//Do all our painting here

		EndPaint(_hwnd, &ps);
		return (0);
	}
	break;
	case WM_COMMAND:
	{
		switch (LOWORD(_wparam))
		{
		case ID_FILE_LOADIMAGE:
		{
			if (ChooseImageFilesToLoad(_hwnd))
			{
				// Now we have a vector of image file names to actually load.
				// g_vecImageFileNames.

				// Step 1: LOAD IMAGES IN PARALLEL.
				// Now for each file name, we need to load the image in parallel using threads.
				// I can use c++ to do this, using <fstream>.
				std::vector<std::thread> threadPool;
				for(std::wstring imageFileName : g_vecImageFileNames)
				{
					threadPool.push_back(std::thread(LoadImageIntoImages, imageFileName));
				}

				for(auto& thread : threadPool)
				{
					thread.join();
				}

				// Step 2: DRAW IMAGES IN PARALLEL. (Shared Resource).
				// For marking I will use 4 unique square images -- each 256x256 pixels,
				// and a screen size of 512x512 -- thus you can stack two on each horizontal and vertical.
				// Drawing images in parallel requires using the win32 API.


				std::vector<std::thread> threads;
				std::mutex imageLock;
				int imageIndex = 0;
				int bottomIndex = 0;
				for (HBITMAP loadedImage : g_vecLoadedImages)
				{
					imageLock.lock();
					threads.push_back(std::thread(DrawImages, std::ref(_hwnd), loadedImage, imageIndex, bottomIndex));
					if (imageIndex > 1) {
						++bottomIndex;
				    }
					++imageIndex;
					imageLock.unlock();
				}
				int idx = 0;
				for (HBITMAP loadedImage : g_vecLoadedImages){
					threads[idx].join();
					++idx;
				}
				
			}
			else
			{
				MessageBox(_hwnd, L"No Image File selected", L"Error Message", MB_ICONWARNING);
			}

			return (0);
		}
		break;
		case ID_FILE_LOADSOUND:
		{
			if (ChooseSoundFilesToLoad(_hwnd))
			{
				// Audio will be marked with 4 audio clips.

							// Step 1: LOAD IMAGES IN PARALLEL.
				// Now for each file name, we need to load the image in parallel using threads.
				// I can use c++ to do this, using <fstream>.
				std::vector<std::thread> threadPool;
				for (std::wstring soundFileName : g_vecSoundFileNames)
				{
					threadPool.push_back(std::thread(LoadSoundIntoSounds, soundFileName));
				}

				for (auto& thread : threadPool)
				{
					thread.join();
				}

			}
			else
			{
				MessageBox(_hwnd, L"No Sound File selected", L"Error Message", MB_ICONWARNING);
			}
			return (0);
		}
		break;
		case ID_EXIT:
		{
			SendMessage(_hwnd, WM_CLOSE, 0, 0);
			return (0);
		}
		break;
		default:
			break;
		}
	}
	break;
	case WM_CLOSE:
	{
		PostQuitMessage(0);
	}
	break;
	default:
		break;
	}
	return (DefWindowProc(_hwnd, _uiMsg, _wparam, _lparam));
}

/*
	Register a template to use for all windows, and create a window.
*/
HWND CreateAndRegisterWindow(HINSTANCE _hInstance)
{
	WNDCLASSEX winclass; // This will hold the class we create.
	HWND hwnd;           // Generic window handle.

						 // First fill in the window class structure.
	winclass.cbSize = sizeof(WNDCLASSEX);
	winclass.style = CS_DBLCLKS | CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
	winclass.lpfnWndProc = WindowProc;
	winclass.cbClsExtra = 0;
	winclass.cbWndExtra = 0;
	winclass.hInstance = _hInstance;
	winclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	winclass.hCursor = LoadCursor(NULL, IDC_ARROW);
	winclass.hbrBackground =
		static_cast<HBRUSH>(GetStockObject(WHITE_BRUSH));
	winclass.lpszMenuName = NULL;
	winclass.lpszClassName = WINDOW_CLASS_NAME;
	winclass.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

	// register the window class
	if (!RegisterClassEx(&winclass))
	{
		return (0);
	}

	HMENU _hMenu = LoadMenu(_hInstance, MAKEINTRESOURCE(IDR_MENU1));

	// create the window
	hwnd = CreateWindowEx(NULL, // Extended style.
		WINDOW_CLASS_NAME,      // Class.
		L"MultiThreaded Loader Tool",   // Title.
		WS_OVERLAPPEDWINDOW | WS_VISIBLE,
		10, 10,                    // Initial x,y.
		_kuiWINDOWWIDTH, _kuiWINDOWHEIGHT,                // Initial width, height.
		NULL,                   // Handle to parent.
		_hMenu,                   // Handle to menu.
		_hInstance,             // Instance of this application.
		NULL);                  // Extra creation parameters.

	return hwnd;
}

/*
	Entry point to our application.
*/
int WINAPI WinMain(HINSTANCE _hInstance,
	HINSTANCE _hPrevInstance,
	LPSTR _lpCmdLine,
	int _nCmdShow)
{
	// Create and register a window, duh!
	HWND _hwnd = CreateAndRegisterWindow(_hInstance);
	// SDL_Init(SDL_INIT_EVERYTHING);


	// We failed to create a window :(.
	if (!_hwnd)
	{
		return 1;
	}




	const int init_result = SDL_Init(SDL_INIT_EVERYTHING);
	const int init_result_success = 0;
	if (init_result != init_result_success)
	{
		std::cout << "Failed to initialize SDL" << std::endl;
		std::cout << "SDL Error: " << SDL_GetError() << std::endl;
		exit(1);
	}

	// Init Mixer
//	const int mixer_init_result = Mix_Init(MIX_INIT_MOD);
	const int mixer_init_failure = 0;
	if (mixer_init_result == mixer_init_failure)
	{
		std::cout << "Failed to initialize SDL Mixer" << std::endl;
		std::cout << "SDL Error: " << SDL_GetError() << std::endl;
	//	std::cout << "Mixer Error: " << Mix_GetError() << std::endl;
		exit(1);
	}

	// Ready Mixer for audio
//	const int mixer_open_audio_result = Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 1024);
	const int mixer_open_audio_success = 0;
	if (mixer_open_audio_result != mixer_open_audio_success)
	{
		std::cout << "Failed to open audio" << std::endl;
		std::cout << "SDL Error: " << SDL_GetError() << std::endl;
		exit(1);
	}

	// Create 3 channels
//	Mix_AllocateChannels(3);

	// Play 3 audio clips on different channels
	Mix_Chunk* data = nullptr;
	data = Mix_LoadWAV("1.wav");
	Mix_PlayChannel(0, data, -1);

	data = Mix_LoadWAV("2.wav");
	Mix_PlayChannel(1, data, -1);

	data = Mix_LoadWAV("3.wav");
	Mix_PlayChannel(2, data, -1);



	// Enter main event loop
	MSG msg;
	while (true)
	{
		// Test if there is a message in queue, if so get it.
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			// Test if this is a quit.
			if (msg.message == WM_QUIT)
			{
				break;
			}

			// Translate any accelerator keys.
			TranslateMessage(&msg);
			// Send the message to the window proc.
			DispatchMessage(&msg);
		}

	}

	// Return to Windows like this...
	return (static_cast<int>(msg.wParam));
}