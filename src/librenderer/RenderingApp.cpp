#include "RenderingApp.h"

Renderer::Renderer() :
    QObject(0)
{
    server = new RenderingServer();
    connect(server, &RenderingServer::computeSamples, this, &Renderer::computeSamples);
}

void Renderer::startServer()
{
    server->startServer(2227);
}

void Renderer::render()
{

}


void Renderer::computeSamples(float *samples)
{
    float v = samples[0] * samples[1];

    qDebug("computing samples %f, %f", samples[0], samples[1]);

    server->sendSamples(1, &v);
}
