#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "stb_image.h"
#include "stb_image_write.h"

#define K 2 // Number of color clusters
#define MAX_ITER 20

typedef struct {
    float r, g, b;
} Color;

typedef struct {
    Color color;
    int index;
} Point;

float color_distance(Color a, Color b) {
    return sqrtf((a.r - b.r)*(a.r - b.r) + (a.g - b.g)*(a.g - b.g) + (a.b - b.b)*(a.b - b.b));
}

// this returns the index of the closest centroid
int closest_cluster(Color color, Color *centroids) {
    int index = 0;
    float min_dist = color_distance(color, centroids[0]);

    for (int i = 1; i < K; ++i) {
        float dist = color_distance(color, centroids[i]);
        if (dist < min_dist) {
            min_dist = dist;
            index = i;
        }
    }
    return index;
}

void kmeans_quantization(unsigned char *image, int width, int height, int channels) {
    int pixels = width * height;
    Point *points = malloc(sizeof(Point) * pixels);

    // decompose image -> colors
    for (int i = 0; i < pixels; ++i) {
        points[i].color.r = image[i * channels];
        points[i].color.g = image[i * channels + 1];
        points[i].color.b = image[i * channels + 2];
    }
    // Initialize centroids randomly
    Color centroids[K];
    for (int i = 0; i < K; ++i) {
        int rand_index = rand() % pixels;
        centroids[i] = points[rand_index].color;
    }


    for (int iter = 0; iter < MAX_ITER; ++iter) {
        // Assign pixels to nearest centroid
        for (int i = 0; i < pixels; ++i) {
            points[i].index = closest_cluster(points[i].color, centroids);
        }

        // Recompute centroids
        int count[K] = {0};
        Color new_centroids[K] = {0};

        for (int i = 0; i < pixels; ++i) {
            int cluster = points[i].index;
            new_centroids[cluster].r += points[i].color.r;
            new_centroids[cluster].g += points[i].color.g;
            new_centroids[cluster].b += points[i].color.b;
            count[cluster]++;
        }

        for (int i = 0; i < K; ++i) {
            if (count[i] > 0) {
                centroids[i].r = new_centroids[i].r / count[i];
                centroids[i].g = new_centroids[i].g / count[i];
                centroids[i].b = new_centroids[i].b / count[i];
            }
        }
    }

    // Apply quantization
    for (int i = 0; i < pixels; ++i) {
        Color c = centroids[points[i].index];
        image[i * channels]     = (unsigned char)c.r;
        image[i * channels + 1] = (unsigned char)c.g;
        image[i * channels + 2] = (unsigned char)c.b;
    }

    free(points);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: %s input_image output_image\n", argv[0]);
        return 1;
    }

    int width, height, channels;
    unsigned char *image = stbi_load(argv[1], &width, &height, &channels, 3);
    if (!image) {
        fprintf(stderr, "Failed to load image\n");
        return 1;
    }

    printf("Loaded image: %dx%d with %d channels\n", width, height, channels);

    kmeans_quantization(image, width, height, 3);

    if (!stbi_write_png(argv[2], width, height, 3, image, width * 3)) {
        fprintf(stderr, "Failed to write image\n");
    }

    stbi_image_free(image);
    return 0;
}
