//
// Created by Elijah Crain on 2/10/25.
//
#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <cmath>

// Include the stb image libraries (make sure to define STB_IMAGE_IMPLEMENTATION in one source file)
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

// Structure to hold atlas region information for a single image.
struct AtlasRegion {
    // Pixel coordinates in the atlas.
    int x;      // Left offset (in pixels)
    int y;      // Top offset (in pixels)
    int width;  // Width of the region
    int height; // Height of the region

    // Normalized UV coordinates (range [0,1]) for convenience.
    float u1;   // Left (u coordinate)
    float v1;   // Top (v coordinate)
    float u2;   // Right (u coordinate)
    float v2;   // Bottom (v coordinate)
};

// Structure representing the complete texture atlas.
struct TextureAtlas {
    int width;    // Atlas image width in pixels
    int height;   // Atlas image height in pixels
    int channels; // Number of channels (e.g. 3 for RGB, 4 for RGBA)
    std::vector<unsigned char> data; // Pixel data for the atlas image
    // Mapping from the original image file name (or identifier) to its atlas region.
    std::map<std::string, AtlasRegion> regions;
};

///
/// @brief Creates a texture atlas from a list of image file names.
/// @param filenames A vector containing the paths to your source images.
/// @param imageWidth Expected width of each source image (all must be the same size).
/// @param imageHeight Expected height of each source image.
/// @param channels The number of color channels to load (e.g. 3 or 4).
/// @return A TextureAtlas struct containing the atlas pixel data and region mapping.
///
TextureAtlas createTextureAtlas(const std::vector<std::string>& filenames,
                                int imageWidth, int imageHeight,
                                int channels)
{
    // Determine how many images we have.
    int count = static_cast<int>(filenames.size());

    // Choose a grid layout: for simplicity we use a square-ish grid.
    int cols = static_cast<int>(std::ceil(std::sqrt(count)));
    int rows = static_cast<int>(std::ceil(static_cast<float>(count) / cols));

    // The atlas dimensions (in pixels) are the number of columns/rows multiplied by the individual image dimensions.
    int atlasWidth = cols * imageWidth;
    int atlasHeight = rows * imageHeight;

    // Allocate a buffer for the atlas pixels and fill it with zeros (or a default background).
    std::vector<unsigned char> atlasData(atlasWidth * atlasHeight * channels, 0);

    // Map each image file to its region inside the atlas.
    std::map<std::string, AtlasRegion> regions;

    for (int i = 0; i < count; i++) {
        // Compute grid position.
        int col = i % cols;
        int row = i / cols;
        int offsetX = col * imageWidth;
        int offsetY = row * imageHeight;

        // Load the image using stb_image.
        int w, h, n;
        unsigned char* imgData = stbi_load(filenames[i].c_str(), &w, &h, &n, channels);
        if (!imgData) {
            std::cerr << "Failed to load image: " << filenames[i] << std::endl;
            continue;
        }
        if (w != imageWidth || h != imageHeight) {
            std::cerr << "Image " << filenames[i] << " does not match expected dimensions. "
                      << "Expected: " << imageWidth << "x" << imageHeight
                      << ", Got: " << w << "x" << h << std::endl;
            stbi_image_free(imgData);
            continue;
        }

        // Copy the loaded image data into the atlas buffer at the correct offset.
        for (int j = 0; j < imageHeight; j++) {
            for (int k = 0; k < imageWidth; k++) {
                for (int c = 0; c < channels; c++) {
                    int atlasIndex = ((offsetY + j) * atlasWidth + (offsetX + k)) * channels + c;
                    int imageIndex = (j * imageWidth + k) * channels + c;
                    atlasData[atlasIndex] = imgData[imageIndex];
                }
            }
        }

        stbi_image_free(imgData);

        // Compute normalized UV coordinates for this image’s region.
        AtlasRegion region;
        region.x = offsetX;
        region.y = offsetY;
        region.width = imageWidth;
        region.height = imageHeight;
        region.u1 = static_cast<float>(offsetX) / atlasWidth;
        region.v1 = static_cast<float>(offsetY) / atlasHeight;
        region.u2 = static_cast<float>(offsetX + imageWidth) / atlasWidth;
        region.v2 = static_cast<float>(offsetY + imageHeight) / atlasHeight;

        regions[filenames[i]] = region;
    }

    TextureAtlas atlas;
    atlas.width = atlasWidth;
    atlas.height = atlasHeight;
    atlas.channels = channels;
    atlas.data = std::move(atlasData);
    atlas.regions = std::move(regions);
    return atlas;
}


/// Optional: A helper function to write the atlas image to disk (e.g. as a PNG file).
bool writeAtlasToFile(const TextureAtlas& atlas, const std::string& filename)
{
    // Use stbi_write_png (make sure you included stb_image_write.h)
    int result = stbi_write_png(filename.c_str(), atlas.width, atlas.height,
                                atlas.channels, atlas.data.data(), atlas.width * atlas.channels);
    return (result != 0);
}


/// Example usage.
int create(std::vector<std::string> filenames, std::string output = "atlas")
{
    // List your image filenames here (all images must be the same dimensions).
    std::vector<std::string> images = {
        "image1.png",
        "image2.png",
        "image3.png",
        "image4.png"
        // ... add as many as needed.
    };

    // Specify the expected dimensions and number of channels for your images.
    // (You might want to determine these dynamically; here we assume, for example, 128x128 RGBA images.)
    const int imageWidth = 128;
    const int imageHeight = 128;
    const int channels = 4;

    TextureAtlas atlas = createTextureAtlas(images, imageWidth, imageHeight, channels);

    // Optionally write the atlas image out so you can inspect it.
    if (writeAtlasToFile(atlas, "texture_atlas.png")) {
        std::cout << "Atlas written successfully to texture_atlas.png" << std::endl;
    } else {
        std::cerr << "Failed to write atlas image." << std::endl;
    }

    // Print out each image’s atlas region information.
    for (const auto& entry : atlas.regions) {
        const std::string& name = entry.first;
        const AtlasRegion& region = entry.second;
        std::cout << "Image: " << name << "\n"
                  << "  Pixel Region: (" << region.x << ", " << region.y << ") "
                  << region.width << "x" << region.height << "\n"
                  << "  UV Region: (" << region.u1 << ", " << region.v1 << ") to ("
                  << region.u2 << ", " << region.v2 << ")\n";
    }

    return 0;
}
