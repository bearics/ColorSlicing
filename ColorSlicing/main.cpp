#define _CRT_SECURE_NO_WARNINGS

#include <iostream>

#define HEIGHT 512
#define WIDTH 512

using namespace std;

typedef struct RGB {
	unsigned char r;
	unsigned char g;
	unsigned char b;
};

typedef struct Point {
	int x;
	int y;
};

template<typename T> T** MemAlloc2D(int nHeight, int nWidth, RGB initRGB)
{
	T** rtn = new T*[nHeight];
	for (int h = 0; h < nHeight; h++)
	{
		rtn[h] = new T[nWidth];
		for (int w = 0; w < nWidth; w++)
		{
			rtn[h][w] = initRGB;
		}
	}
	return rtn;
}

template<typename T> void MemFree2D(T** arr2D, int nHeight)
{
	for (int h = 0; h < nHeight; h++)
	{
		delete[] arr2D[h];
	}
	delete[] arr2D;
}

void FileRead(const char* strFilename, RGB** arr2D, int nHeight, int nWidth)
{
	FILE* fp_in = fopen(strFilename, "rb");
	for (int h = 0; h < nHeight; h++)
	{
		fread(arr2D[h], sizeof(RGB), nWidth, fp_in);
	}

	fclose(fp_in);
}

void FileWrite(const char* strFilename, RGB** arr2D, int nHeight, int nWidth)
{
	FILE* fp_out = fopen(strFilename, "wb");
	for (int h = 0; h < nHeight; h++)
	{
		fwrite(arr2D[h], sizeof(RGB), nWidth, fp_out);
	}

	fclose(fp_out);
}

template<typename T> void ConverterRgb(T** in)
{	// update order BGR file to RGB file.
	unsigned char temp;

	for (int y = 0; y < HEIGHT; y++)
	{
		for (int x = 0; x < WIDTH; x++)
		{
			temp = in[y][x].r;
			in[y][x].r = in[y][x].b;
			in[y][x].b = temp;
		}
	}
}

template<typename T> void FindRgb(T** in)
{	// find specular pixel
	unsigned char temp;
	int a = 1;

	for (int y = 0; y < HEIGHT; y++)
	{
		for (int x = 0; x < WIDTH; x++)
		{
			if (in[y][x].r <= 199 + a && in[y][x].r >= 199 - a)
			{
				if (in[y][x].g <= 147 + a && in[y][x].g >= 147 - a)
				{
					if (in[y][x].b <= 125 + a && in[y][x].b >= 125 - a)
					{
						cout << y << ", " << x << endl;
					}
				}
			}
		}
	}
}

template<typename T> void DetectFace(T** in, T** out, T** reversOut, Point pos, double radius)
{	// update order BGR file to RGB file.
	RGB point{ in[pos.y][pos.x].r, in[pos.y][pos.x].g, in[pos.y][pos.x].b };

	for (int y = 0; y < HEIGHT; y++)
	{
		for (int x = 0; x < WIDTH; x++)
		{
			RGB* pixel = (RGB*)addressof(in[y][x]);
			if ( pow (pos.x - x , 2) + pow(pos.y - y, 2) < 70000 )
			{
				if (sqrt(pow(point.r - pixel->r, 2) + pow(point.g - pixel->g, 2) + pow(point.b - pixel->b, 2)) < radius)
				{
					memcpy(addressof(out[y][x]), pixel, sizeof(unsigned char) * 3);
				}
				else
				{
					memcpy(addressof(reversOut[y][x]), pixel, sizeof(unsigned char) * 3);
				}
			}
			else
			{
				memcpy(addressof(reversOut[y][x]), pixel, sizeof(unsigned char) * 3);
			}
		}
	}
}

template<typename T> void SmoothFilter(T** in, T** out, int filterSize)
{
	unsigned char** filter = new unsigned char*[filterSize];
	int midSize = (int)(filterSize / 2);

	for (int y = 0; y < HEIGHT - filterSize; y++)
	{
		for (int x = 0; x < WIDTH - filterSize; x++)
		{
			int sum[3] = { 0,0,0 };
			int count[3] = { 0,0,0 };
			for (int v = 0; v < filterSize; v++)
			{
				for (int u = 0; u < filterSize; u++)
				{
					for (int i = 0; i < 3; i++)
					{
						unsigned char* temp = (unsigned char*)addressof(in[y + v][x + u]) + i;
						if (*temp != 0)
						{
							count[i]++;
							sum[i] += *temp;
						}
					}
				}
			}
			for (int i = 0; i < 3; i++)
			{
				if (count[i] != 0)
				{
					unsigned char* temp = (unsigned char*)addressof(out[y + midSize][x + midSize]) + i;
					*temp = sum[i] / count[i];
				}
			}
		}
	}
}

template<typename T> void MergeFaceAndBackground(T** background, T** face, T** smoothFace, T** out)
{
	for (int y = 0; y < HEIGHT; y++)
	{
		for (int x = 0; x < WIDTH; x++)
		{
			bool FaceFlag = false;	// if true, face data is alive.
			bool BackgroundFlag = false;	// if true, face data is alive.
			for (int i = 0; i < 3; i++)
			{
				unsigned char* temp = (unsigned char*)addressof(face[y][x]) + i;
				if (*temp != 0)
					FaceFlag = true;
			}
			out[y][x] = FaceFlag ? smoothFace[y][x] : background[y][x];
		}
	}
}

void main()
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	// File Open & Memory Allocation

	RGB** arr2D = MemAlloc2D<RGB>(HEIGHT, WIDTH, {0,0,0});
	RGB** faceArr2D = MemAlloc2D<RGB>(HEIGHT, WIDTH, { 0, 0, 0 });
	RGB** removeFaceArr2D = MemAlloc2D<RGB>(HEIGHT, WIDTH, { 0, 0, 0 });
	RGB** smoothFaceArr2D = MemAlloc2D<RGB>(HEIGHT, WIDTH, { 0, 0, 0 });
	RGB** afterFaceArr2D = MemAlloc2D<RGB>(HEIGHT, WIDTH, {0, 0, 0});

	FileRead("face.rgb", arr2D, HEIGHT, WIDTH);

	DetectFace(arr2D, faceArr2D, removeFaceArr2D,{ 189, 136 }, 56.26446841*3);
	SmoothFilter(faceArr2D, smoothFaceArr2D, 7);
	MergeFaceAndBackground(arr2D, faceArr2D, smoothFaceArr2D, afterFaceArr2D);
	
	FileWrite("ReverseFace.raw", removeFaceArr2D, HEIGHT, WIDTH);
	FileWrite("OnlyFace.raw", faceArr2D, HEIGHT, WIDTH);
	FileWrite("OnlySmoothFace.raw", smoothFaceArr2D, HEIGHT, WIDTH);
	FileWrite("AfterFace.raw", afterFaceArr2D, HEIGHT, WIDTH);

}