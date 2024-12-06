// Sandbox.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <glfw.h>

#pragma comment(lib, "Window.lib")
#pragma comment(lib, "glfw3.lib")

using namespace eXngine;

int WinMain(char** argv, int argc)
{
    Applications::GLFWApplication app("Sandbox", Point(50, 50), Size(1024, 768), false);

    return app.Run();
}