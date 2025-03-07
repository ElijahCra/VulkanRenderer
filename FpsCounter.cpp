//
// Created by Elijah Crain on 3/7/25.
//

//
// Created by Elijah Crain on 3/7/25.
//
#pragma once

#include <chrono>
#include <deque>
#include <numeric>

class FPSCounter {
public:
    explicit FPSCounter(size_t sampleWindow = 100) : maxSamples(sampleWindow) {
        lastFrameTime = std::chrono::high_resolution_clock::now();
    }

    // Call this every frame to update the FPS calculation
    void update() {
        auto currentTime = std::chrono::high_resolution_clock::now();
        float deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - lastFrameTime).count();
        lastFrameTime = currentTime;

        // Don't add unreasonable values (e.g., after pausing)
        if (deltaTime > 0.0f && deltaTime < 0.5f) {
            frameTimes.push_back(deltaTime);
        }

        // Keep the deque at the max sample size
        while (frameTimes.size() > maxSamples) {
            frameTimes.pop_front();
        }

        // Calculate current time elapsed
        elapsedTime += deltaTime;
        frameCount++;

        // Update every second for the title bar display
        if (elapsedTime >= updateInterval) {
            currentFPS = calculateFPS();
            elapsedTime = 0.0f;
            frameCount = 0;
        }
    }

    // Get the current FPS
    float getFPS() const {
        return currentFPS;
    }

    // Get the current frame time in milliseconds
    float getFrameTimeMs() const {
        if (frameTimes.empty()) {
            return 0.0f;
        }
        return frameTimes.back() * 1000.0f;
    }

    // Get the average frame time over the sample window in milliseconds
    float getAverageFrameTimeMs() const {
        if (frameTimes.empty()) {
            return 0.0f;
        }

        float sum = std::accumulate(frameTimes.begin(), frameTimes.end(), 0.0f);
        return (sum / frameTimes.size()) * 1000.0f;
    }

private:
    std::chrono::time_point<std::chrono::high_resolution_clock> lastFrameTime;
    std::deque<float> frameTimes;
    size_t maxSamples;

    float elapsedTime = 0.0f;
    unsigned int frameCount = 0;
    float currentFPS = 0.0f;
    float updateInterval = 0.5f; // Update FPS display twice per second

    float calculateFPS() const {
        if (frameTimes.empty()) {
            return 0.0f;
        }

        float avgFrameTime = std::accumulate(frameTimes.begin(), frameTimes.end(), 0.0f) / frameTimes.size();
        return 1.0f / avgFrameTime;
    }
};