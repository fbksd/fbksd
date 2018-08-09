#include "RenderingApp.h"

#include <QCoreApplication>
#include <iostream>
using namespace std;



int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    Renderer *renderer = new Renderer(); 
    renderer->startServer();

    return a.exec();
}
