#ifndef RENDERING_APP
#define RENDERING_APP

#include "RenderingServer.h"
#include <QObject>


class Renderer: public QObject
{
    Q_OBJECT

public:
    Renderer();

    void startServer();

    void render();

private slots:
    void computeSamples(float *samples);

private:
    RenderingServer *server;
};


#endif
