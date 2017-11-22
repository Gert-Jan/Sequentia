#include "Sequentia.h";

#pragma comment(lib, "SDL2.lib")
#pragma comment(lib, "SDL2main.lib")
#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "avformat.lib")
#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "avutil.lib")

int main(int argc, char** argv)
{
	if (argc >= 2)
		return Sequentia::Run(argv[1]);
	else
		return Sequentia::Run(nullptr);
}
