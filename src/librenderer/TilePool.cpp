#include "TilePool.h"
using namespace fbksd;
#include <iostream>


float* TilePool::sm_samples = nullptr;
int64_t TilePool::sm_tileNumSamples = 0;
int TilePool::sm_numTiles = 20;
std::vector<int> TilePool::sm_freeIndices; // indexes free to be written by the server
std::queue<Tile> TilePool::sm_waitingInputTiles;
std::queue<int> TilePool::sm_readyInputTiles;
std::queue<Tile> TilePool::sm_workedIndices; // indexes waiting to be consumed by the client
std::mutex TilePool::sm_mutex;
std::condition_variable TilePool::sm_hasFreeTile;
std::condition_variable TilePool::sm_hasTileForClient;
std::condition_variable TilePool::sm_isInputReady;
int64_t TilePool::sm_numSamples = 0;
int64_t TilePool::sm_numWorkedSamples = 0;
bool TilePool::sm_waitInput = false;


void TilePool::init(int64_t numSamples, int64_t tileNumSamples, int sampleSize, float* samples, bool waitInput)
{
    sm_numSamples = numSamples;
    sm_tileNumSamples = tileNumSamples;
    sm_numWorkedSamples = 0;
    sm_samples = samples;
    sm_waitInput = waitInput;

    // Initializes the free tiles list.
    // Each index in the position of a tile in the samples buffer.
    sm_freeIndices.clear();
    sm_freeIndices.reserve(sm_numTiles);
    const size_t tileSize = tileNumSamples * sampleSize;
    for(int i = sm_numTiles - 1; i >= 0; --i)
        sm_freeIndices.push_back(i * tileSize);
}

float* TilePool::getFreeTile(const Point2l& begin, const Point2l& end, int64_t numSamples)
{
    assert(numSamples <= sm_tileNumSamples);
    int tileIndex = 0;
    float* tmp = nullptr;
    std::unique_lock<std::mutex> lock(sm_mutex);
    sm_hasFreeTile.wait(lock, [&](){return !sm_freeIndices.empty();});
    tileIndex = sm_freeIndices.back();
    sm_freeIndices.pop_back();
    tmp = &sm_samples[tileIndex];

    if(sm_waitInput)
    {
        Tile tile(CropWindow(begin, end), tileIndex, numSamples);
        sm_waitingInputTiles.push(tile);
        sm_hasTileForClient.notify_one();
        sm_isInputReady.wait(lock, [&](){
            return !sm_readyInputTiles.empty() &&
                    sm_readyInputTiles.front() == tileIndex;
        });
        sm_readyInputTiles.pop();
    }

    lock.unlock();
    return tmp;
}

Tile TilePool::getClientTile(bool& hasNext, bool& isInput)
{
    std::unique_lock<std::mutex> lock(sm_mutex);
    sm_hasTileForClient.wait(lock, [&](){
        return sm_waitingInputTiles.size() || sm_workedIndices.size();});

    Tile tile;
    if(sm_waitingInputTiles.size())
    {
        tile = sm_waitingInputTiles.front();
        sm_waitingInputTiles.pop();
        hasNext = true;
        isInput = true;
        lock.unlock();
        return tile;
    }
    else
    {
        tile = sm_workedIndices.front();
        sm_workedIndices.pop();
        hasNext = (sm_numWorkedSamples < sm_numSamples) || (!sm_workedIndices.empty());
        isInput = false;
    }

    lock.unlock();
    return tile;
}

void TilePool::releaseInputTile(int index)
{
    std::unique_lock<std::mutex> lock(sm_mutex);
    sm_readyInputTiles.push(index);
    lock.unlock();
    sm_isInputReady.notify_all();
}

void TilePool::releaseWorkedTile(const SamplesPipe &pipe)
{
    if(!pipe.m_samples)
        return;

    auto index = pipe.m_samples - sm_samples;
    auto numSamples = pipe.getNumSamples();
    if(sm_numWorkedSamples + numSamples > sm_numSamples)
        throw std::logic_error("Rendererd samples exceed maximum quantity.");
    if(pipe.m_informedNumSamples != numSamples)
        throw std::logic_error("Number of rendered samples differ from informed at pipe construction.");

    std::unique_lock<std::mutex> lock(sm_mutex);
    sm_workedIndices.push(Tile({pipe.m_begin, pipe.m_end}, index, numSamples));
    sm_numWorkedSamples += numSamples;
    lock.unlock();
    sm_hasTileForClient.notify_one();
}

void TilePool::releaseConsumedTile(int index)
{
    std::unique_lock<std::mutex> lock(sm_mutex);
    sm_freeIndices.push_back(index);
    lock.unlock();
    sm_hasFreeTile.notify_one();
}
