//
// Created by Elijah Crain on 3/7/25.
//

//
// Created by Elijah Crain on 3/7/25.
//
#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

// A camera path point with position, rotation, and optional speed
struct CameraPathPoint {
    glm::vec3 position;
    glm::quat rotation; // Quaternion for smooth rotation
    float speed = 1.0f; // Units per second
    float dwell = 0.0f; // Optional dwell time at this point
};

class CameraPath {
public:
    CameraPath() = default;

    // Add a point to the path
    void addPoint(const glm::vec3& position, const glm::quat& rotation, float speed = 1.0f, float dwell = 0.0f) {
        CameraPathPoint point;
        point.position = position;
        point.rotation = rotation;
        point.speed = speed;
        point.dwell = dwell;
        points.push_back(point);
    }

    // Get the current camera position and rotation based on time
    std::pair<glm::vec3, glm::quat> getCurrentPosition(float time) {
        if (points.empty()) {
            return {glm::vec3(0.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f)};
        }

        if (points.size() == 1) {
            return {points[0].position, points[0].rotation};
        }

        // Find the current segment and interpolation factor
        float totalTime = 0.0f;
        size_t currentIndex = 0;
        float dwellRemainder = 0.0f;

        // Calculate total path length and segment times
        std::vector<float> segmentTimes(points.size());

        // Initialize first segment
        segmentTimes[0] = points[0].dwell;

        // Calculate time for each segment
        for (size_t i = 0; i < points.size() - 1; i++) {
            float distance = glm::distance(points[i].position, points[i+1].position);
            float speed = points[i].speed;
            float travelTime = distance / speed;

            segmentTimes[i] += travelTime;
            segmentTimes[i+1] = points[i+1].dwell;
        }

        // Find current segment based on time
        float elapsedTime = 0.0f;
        for (size_t i = 0; i < points.size() - 1; i++) {
            float segmentTime = segmentTimes[i];

            if (time < elapsedTime + segmentTime) {
                // We're in a dwell period
                currentIndex = i;
                dwellRemainder = time - elapsedTime;
                return {points[i].position, points[i].rotation};
            }

            elapsedTime += segmentTime;

            float travelTime = glm::distance(points[i].position, points[i+1].position) / points[i].speed;

            if (time < elapsedTime + travelTime) {
                // We're traveling between points
                currentIndex = i;
                float t = (time - elapsedTime) / travelTime;

                // Linear interpolation for position
                glm::vec3 position = glm::mix(points[i].position, points[i+1].position, t);

                // Spherical interpolation for rotation (smoother camera movement)
                glm::quat rotation = glm::slerp(points[i].rotation, points[i+1].rotation, t);

                return {position, rotation};
            }

            elapsedTime += travelTime;
        }

        // If we're past the end of the path, return the last point
        return {points.back().position, points.back().rotation};
    }

    // Get path duration in seconds
    float getTotalDuration() const {
        if (points.empty()) return 0.0f;

        float totalTime = 0.0f;

        // Add dwell time for first point
        totalTime += points[0].dwell;

        // Calculate travel times between points
        for (size_t i = 0; i < points.size() - 1; i++) {
            float distance = glm::distance(points[i].position, points[i+1].position);
            float speed = points[i].speed;
            totalTime += distance / speed;

            // Add dwell time for next point
            if (i < points.size() - 1) {
                totalTime += points[i+1].dwell;
            }
        }

        return totalTime;
    }

    // Create a default circular path around the origin
    static CameraPath createDefaultPath() {
        CameraPath path;

        // Create a circular path around the origin
        const int numPoints = 8;
        const float radius = 15.0f;
        const float height = 5.0f;
        const float speed = 2.0f;

        for (int i = 0; i < numPoints; i++) {
            float angle = (float)i / numPoints * 2.0f * glm::pi<float>();
            glm::vec3 position(
                radius * std::cos(angle),
                radius * std::sin(angle),
                -height
            );

            // Calculate rotation to look at the center
            glm::vec3 direction = glm::normalize(-position);
            glm::vec3 up(0.0f, 1.0f, 0.0f);
            glm::vec3 right = glm::normalize(glm::cross(up, direction));
            up = glm::cross(direction, right);

            // Create rotation matrix and convert to quaternion
            glm::mat3 rotMatrix(right, up, direction);
            glm::quat rotation = glm::quat_cast(rotMatrix);

            path.addPoint(position, rotation, speed);
        }

        return path;
    }

    // Create a more complex circular path with varying heights and speeds
    static CameraPath createComplexPath() {
        CameraPath path;

        // Create a figure-8 path around the origin
        const int numPoints = 16;
        const float radius = 20.0f;
        const float heightVariation = 8.0f;
        const float baseHeight = 10.0f;

        for (int i = 0; i < numPoints; i++) {
            float angle = (float)i / numPoints * 2.0f * glm::pi<float>();

            // Figure-8 pattern
            float x = radius * std::cos(angle);
            float y = radius * std::sin(angle) * std::cos(angle);

            // Varying height
            float height = baseHeight + heightVariation * std::sin(angle * 2.0f);

            // Varying speed - slower at peaks
            float speed = 2.0f + 1.0f * std::cos(angle * 2.0f);

            // Add dwell time at certain points
            float dwell = (i % 4 == 0) ? 1.0f : 0.0f;

            glm::vec3 position(x, height, y);

            // Calculate rotation to look at the center
            glm::vec3 direction = glm::normalize(-position);
            glm::vec3 up(0.0f, 1.0f, 0.0f);
            glm::vec3 right = glm::normalize(glm::cross(up, direction));
            up = glm::cross(direction, right);

            // Create rotation matrix and convert to quaternion
            glm::mat3 rotMatrix(right, up, direction);
            glm::quat rotation = glm::quat_cast(rotMatrix);

            path.addPoint(position, rotation, speed, dwell);
        }

        return path;
    }

// Make points public so it can be accessed for initialization
public:
    std::vector<CameraPathPoint> points;
};