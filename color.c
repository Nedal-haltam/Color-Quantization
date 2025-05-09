#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "stb_image.h"
#include "stb_image_write.h"

#define K 8
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
int ClosestCluster(Color color, Color *centroids) {
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

void Cluster(Color* Centroids, Point* Points, int count)
{
    // Assign pixels to nearest centroid
    for (int i = 0; i < count; ++i) {
        Points[i].index = ClosestCluster(Points[i].color, Centroids);
    }

    // Recompute Centroids
    int CentroidCount[K] = {0};
    Color NewCentroids[K] = {0};

    for (int i = 0; i < count; ++i) {
        int cluster = Points[i].index;
        NewCentroids[cluster].r += Points[i].color.r;
        NewCentroids[cluster].g += Points[i].color.g;
        NewCentroids[cluster].b += Points[i].color.b;
        CentroidCount[cluster]++;
    }

    for (int i = 0; i < K; ++i) {
        if (CentroidCount[i] > 0) {
            Centroids[i].r = NewCentroids[i].r / CentroidCount[i];
            Centroids[i].g = NewCentroids[i].g / CentroidCount[i];
            Centroids[i].b = NewCentroids[i].b / CentroidCount[i];
        }
    }    
}

Color* Initialize(Point* Points, Color* centroids, unsigned char *Image, int Channels, int count)
{
    // decompose Image -> colors
    for (int i = 0; i < count; ++i) {
        Points[i].color.r = Image[i * Channels];
        Points[i].color.g = Image[i * Channels + 1];
        Points[i].color.b = Image[i * Channels + 2];
    }
    // Initialize centroids randomly
    
    for (int i = 0; i < K; ++i) {
        int rand_index = rand() % count;
        centroids[i] = Points[rand_index].color;
    }
    return centroids;
}

void kmeans_quantization(unsigned char *Image, int Width, int Height, int Channels) {
    int Pixels = Width * Height;
    Point *Points = malloc(sizeof(Point) * Pixels);

    Color centroids[K];
    Initialize(Points, centroids, Image, Channels, Pixels);
    // run k-means
    for (int iter = 0; iter < MAX_ITER; ++iter) {
        Cluster(centroids, Points, Pixels);
    }

    // Apply quantization
    for (int i = 0; i < Pixels; ++i) {
        Color c = centroids[Points[i].index];
        Image[i * Channels]     = (unsigned char)c.r;
        Image[i * Channels + 1] = (unsigned char)c.g;
        Image[i * Channels + 2] = (unsigned char)c.b;
    }

    free(Points);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: %s input_Image output_Image\n", argv[0]);
        return 1;
    }

    int Width, Height, Channels;
    unsigned char *Image = stbi_load(argv[1], &Width, &Height, &Channels, 3);
    if (!Image) {
        fprintf(stderr, "Failed to load Image\n");
        return 1;
    }

    printf("Loaded Image: %dx%d with %d Channels\n", Width, Height, Channels);

    kmeans_quantization(Image, Width, Height, 3);

    if (!stbi_write_png(argv[2], Width, Height, 3, Image, Width * 3)) {
        fprintf(stderr, "Failed to write Image\n");
    }

    stbi_image_free(Image);
    return 0;
}
