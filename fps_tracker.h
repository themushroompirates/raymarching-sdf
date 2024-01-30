#ifndef FPS_TRACKER_H
#define FPS_TRACKER_H

#ifndef FPS_TRACKER_MAX_EVENTS
#define FPS_TRACKER_MAX_EVENTS 8
#endif

typedef struct FpsTrackerData {
    int bufferSize;
    float *samples;
    int sampleIndexRead;
    int sampleIndexWrite;
    int sampleCount;
} FpsTrackerData;

void InitFpsTracker(int bufferSize);
void ProcessFpsTracker();
void UnloadFpsTracker();

#endif

#ifdef FPS_TRACKER_IMPLEMENTATION

FpsTrackerData tracker = { 0 };

void InitFpsTracker(int bufferSize) {
    tracker.bufferSize = bufferSize;
    tracker.samples = (float*)MemAlloc(sizeof(float)*tracker.bufferSize);
    tracker.sampleIndexRead = 0;
    tracker.sampleIndexWrite = tracker.bufferSize/2;
}

void ProcessFpsTracker(Rectangle bounds) {
    float dt = GetDeltaTime();
    
    tracker.samples[tracker.sampleIndexWrite++] = dt;
    tracker.sampleIndexRead++;
    
    tracker.sampleIndexWrite = (tracker.sampleIndexWrite % tracker.bufferSize);
    tracker.sampleIndexRead = (tracker.sampleIndexRead % tracker.bufferSize);
    
    // Find the extrema for scaling
    float minValue = INFINITY, maxValue = -INFINITY;
    if (tracker.sampleIndexRead < tracker.sampleIndexWrite) {
        for (int i = tracker.sampleIndexRead; i <= tracker.sampleIndexWrite; i++) {
            float value = tracker.samples[i];
            minValue = fmin(minValue, value);
            maxValue = fmax(maxValue, value);
        }
    }
    else {
        for (int i = tracker.sampleIndexRead; i <= tracker.sampleIndexWrite; i++) {
            float value = tracker.samples[i];
            minValue = fmin(minValue, value);
            maxValue = fmax(maxValue, value);
        }
    }
    
    // Draw the graph
    DrawRectangleRec(bounds, Fade(BLACK, 0.5));
    
    for (int i = 0; i < tracker.sampleCount; i++) {
        float value = tracker.samples[(tracker.sampleIndexRead+i)%tracker.bufferSize];
        float y = bounds.y + maxValue <= 0.00001f ? 0 : bounds.height - bounds.height * value/maxValue;
        float x = bounds.x + (float)i / (float)tracker.sampleCount;
        DrawPixel(x, y, WHITE);
    }
}

#endif