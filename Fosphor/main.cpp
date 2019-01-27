#include "Fosphor.h"
#include <QtWidgets/QApplication>
#include <QtOpenGL/QGL>

#ifdef _DEBUG
#pragma comment( lib , "opengl32")
#pragma comment( lib , "../3rdparty/debug/lib/zlibd")
#pragma comment( lib , "../3rdparty/debug/lib/OpenCL")
#pragma comment( lib , "../3rdparty/debug/lib/libpng16d")
#pragma comment( lib , "../3rdparty/debug/lib/glew32d")
#pragma comment( lib , "../3rdparty/debug/lib/freetyped")
#pragma comment( lib , "../3rdparty/debug/lib/bz2d")
#else

#pragma comment( lib , "opengl32")
#pragma comment( lib , "../3rdparty/lib/zlib")
#pragma comment( lib , "../3rdparty/lib/OpenCL")
#pragma comment( lib , "../3rdparty/lib/libpng16")
#pragma comment( lib , "../3rdparty/lib/glew32")
#pragma comment( lib , "../3rdparty/lib/freetype")
#pragma comment( lib , "../3rdparty/lib/bz2")
#endif // _DEBUG

int main(int argc, char *argv[])
{

	/*QSurfaceFormat format;
	format.setDepthBufferSize(24);
	format.setStencilBufferSize(8);
	format.setVersion(4, 3);
	format.setProfile(QSurfaceFormat::CoreProfile);
	QSurfaceFormat::setDefaultFormat(format);*/



	QApplication a(argc, argv);
	Fosphor w;
	w.showMaximized();
	return a.exec();
}
