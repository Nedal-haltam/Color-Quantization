
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>
// #define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
// #define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "raylib/include/raylib.h"

int NumberOfColors = 0;
#define MAX_ITER 30

typedef struct
{
    float r;
    float g;
    float b;
} Colorf;

typedef struct {
    Colorf color;
    int index;
} Point;

float color_distance(Colorf a, Colorf b) {
    return sqrtf((a.r - b.r)*(a.r - b.r) + (a.g - b.g)*(a.g - b.g) + (a.b - b.b)*(a.b - b.b));
}

// this returns the index of the closest centroid
int ClosestCluster(Colorf color, Colorf *centroids) {
    int index = 0;
    float min_dist = color_distance(color, centroids[0]);

    for (int i = 1; i < NumberOfColors; ++i) {
        float dist = color_distance(color, centroids[i]);
        if (dist < min_dist) {
            min_dist = dist;
            index = i;
        }
    }
    return index;
}

void Cluster(Colorf* Centroids, Point* Points, int count)
{
    // Assign pixels to nearest centroid
    for (int i = 0; i < count; ++i) {
        Points[i].index = ClosestCluster(Points[i].color, Centroids);
    }

    // Recompute Centroids
    int* CentroidCount = (int*)malloc(sizeof(int) * NumberOfColors);
    Colorf* NewCentroids = (Colorf*)malloc(sizeof(Colorf) * NumberOfColors);
    for (int i = 0; i < NumberOfColors; i++)
    {
        CentroidCount[i] = 0;
        NewCentroids[i] = (Colorf){0};
    }

    for (int i = 0; i < count; ++i) {
        int cluster = Points[i].index;
        NewCentroids[cluster].r += Points[i].color.r;
        NewCentroids[cluster].g += Points[i].color.g;
        NewCentroids[cluster].b += Points[i].color.b;
        CentroidCount[cluster]++;
    }

    for (int i = 0; i < NumberOfColors; ++i) {
        if (CentroidCount[i] > 0) {
            Centroids[i].r = NewCentroids[i].r / CentroidCount[i];
            Centroids[i].g = NewCentroids[i].g / CentroidCount[i];
            Centroids[i].b = NewCentroids[i].b / CentroidCount[i];
        }
    }    
    free(CentroidCount);
    free(NewCentroids);
}

int count_unique_colors(uint32_t *image, int count) {
    // Use a simple hash table for color uniqueness
    // For small images, a fixed-size table is fine
    #define HASH_SIZE 65536
    int unique = 0;
    uint32_t *table = (uint32_t*)calloc(HASH_SIZE, sizeof(uint32_t));
    char *used = (char*)calloc(HASH_SIZE, sizeof(char));
    for (int i = 0; i < count; ++i) {
        uint32_t color = image[i] & 0x00FFFFFF; // ignore alpha
        uint32_t hash = ((color >> 16) ^ (color >> 8) ^ color) % HASH_SIZE;
        // Linear probing
        while (used[hash] && table[hash] != color) {
            hash = (hash + 1) % HASH_SIZE;
        }
        if (!used[hash]) {
            used[hash] = 1;
            table[hash] = color;
            unique++;
        }
    }
    free(table);
    free(used);
    return unique;
}

void Initialize(Point* Points, Colorf* centroids, uint32_t *image, int count)
{
    // decompose image -> colors
    for (int i = 0; i < count; ++i) {
        Points[i].color.r = (image[i] >> 8*0) & 0xFF;
        Points[i].color.g = (image[i] >> 8*1) & 0xFF;
        Points[i].color.b = (image[i] >> 8*2) & 0xFF;
    }
    
    // Initialize centroids randomly
    for (int i = 0; i < NumberOfColors; ++i) {
        int rand_index = rand() % count;
        centroids[i] = Points[rand_index].color;
    }
}

void kmeans_quantization(uint32_t *image, int Width, int Height) {
    int Pixels = Width * Height;
    Point* Points = (Point*)malloc(sizeof(Point) * Pixels);
    Colorf* centroids = (Colorf*)malloc(sizeof(Colorf)*NumberOfColors);
    for (int i = 0; i < NumberOfColors; i++)
    {
        centroids[i] = (Colorf){0};
    }

    Initialize(Points, centroids, image, Pixels);
    // run k-means
    for (int iter = 0; iter < MAX_ITER; ++iter) {
        Cluster(centroids, Points, Pixels);
    }

    for (int i = 0; i < Pixels; ++i) {
        Colorf c = centroids[Points[i].index];
        image[i] = (((int)c.r) << 8*0) | (((int)c.g) << 8*1) | (((int)c.b) << 8*2) | 0xFF000000;
    }
    free(centroids);
    free(Points);
}

int w = 800;
int h = 600;
float scale = 0.4f;
#define BACKGROUND_COLOR ((Color){ .r = 0x20, .g = 0x20, .b = 0x20, .a = 0xFF})

bool ImageLoaded = false;
char ImageFilePath[1024];
const char* OutputFilePath = NULL;
bool LoadImageDropped()
{
    if (IsFileDropped()) {
        FilePathList FilePathList = LoadDroppedFiles();
        printf("file %s dropped\n", FilePathList.paths[0]);
        if (FilePathList.count > 0) {
            snprintf(ImageFilePath, sizeof(ImageFilePath), "%s", FilePathList.paths[0]);
            UnloadDroppedFiles(FilePathList);
            return true;
        }
        UnloadDroppedFiles(FilePathList);
    }
    return false;
}

int main(int argc, char *argv[]) 
{
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_ALWAYS_RUN);
    InitWindow(w, h, "Color Quantization");
    SetTargetFPS(60);
    Image image;
    Texture2D texture;
    if (argc == 2) {
        OutputFilePath = argv[1];
    }
    else if (argc == 3) {
        snprintf(ImageFilePath, sizeof(ImageFilePath), "%s", argv[1]);
        OutputFilePath = argv[2];
        image = LoadImage(ImageFilePath);
        if (image.data == NULL) {
            fprintf(stderr, "Failed to load image: %s\n", ImageFilePath);
            return 1;
        }
        texture = LoadTextureFromImage(image);
        ImageLoaded = true;
    }
    // in general it takes some time...
    // NumberOfColors = count_unique_colors((uint32_t*)image.data, image.width * image.height);
    NumberOfColors = 1;
    float thick = 4.0f;
    int fontsize = 20;
    Rectangle dest = (Rectangle) { .x = w / 2 - scale*texture.width / 2, .y = h / 2 - scale*texture.height / 2, .width = scale*texture.width, .height = scale*texture.height};
    Rectangle lines = (Rectangle) { .x = dest.x - thick, .y = dest.y - thick, .width = dest.width + 2 * thick, .height = dest.height + 2 * thick};
    bool QUANTIZE = false;
    while (!WindowShouldClose())
    {
        w = GetScreenWidth();
        h = GetScreenHeight();
        if (!ImageLoaded)
        {
            BeginDrawing();
            ClearBackground(BACKGROUND_COLOR);
            const char* msg = "Drag&Drop an image here.";
            int msgWidth = MeasureText(msg, fontsize);
            DrawText(msg, w / 2 - msgWidth / 2, h / 2 - fontsize / 2, fontsize, RAYWHITE);
            if (LoadImageDropped())
            {
                ImageLoaded = true;
            }
            DrawFPS(0, 0);
            EndDrawing();
        }
        else
        {
            if (QUANTIZE)
            {
                QUANTIZE = false;
                UnloadImage(image);
                image = LoadImage(ImageFilePath);
                kmeans_quantization((uint32_t*)image.data, image.width, image.height);
                UnloadTexture(texture);
                texture = LoadTextureFromImage(image);
            }
            if (IsKeyPressed(KEY_S))
            {
                if (!OutputFilePath) 
                {
                    fprintf(stderr, "Output File Path was not specified in command line arguments\n");
                }
                else if (!ExportImage(image, OutputFilePath))
                {
                    fprintf(stderr, "Could not save image\n");
                }
            }
            int key = GetKeyPressed();
            if ((KEY_ZERO <= key && key <= KEY_NINE) || (KEY_KP_0 <= key && key <= KEY_KP_9))
            {
                if (KEY_ZERO <= key && key <= KEY_NINE)
                    key -= KEY_ZERO;
                else if (KEY_KP_0 <= key && key <= KEY_KP_9)
                    key -= KEY_KP_0;
                NumberOfColors *= 10;
                NumberOfColors += key;
            }
            if (IsKeyPressed(KEY_R))
                NumberOfColors = 0;
            if (IsKeyPressed(KEY_Q))
                QUANTIZE = true;

            BeginDrawing();
            ClearBackground(BACKGROUND_COLOR);
            
            DrawRectangleLinesEx(lines, thick / 4, RAYWHITE);
            DrawTexturePro(texture, (Rectangle) { .x = 0, .y = 0, .width = texture.width, .height = texture.height}, dest, (Vector2) {.x = 0, .y = 0}, 0, WHITE);

            const char* ColorCountText = TextFormat("Current Number of Colors: %d", NumberOfColors);
            int ColorCountTextWidth = MeasureText(ColorCountText, fontsize);
            DrawText(ColorCountText, w / 2 - ColorCountTextWidth / 2, dest.y + dest.height + fontsize / 2, fontsize, RAYWHITE);
            
            if (QUANTIZE)
            {
                const char* StatusText = "Quantizing...";
                int StatusTextWidth = MeasureText(StatusText, fontsize);
                DrawText(StatusText, w / 2 - StatusTextWidth / 2, dest.y + dest.height + 2 * fontsize, fontsize, RAYWHITE);
            }
            else
            {
                const char* DoneText = "Done.";
                int DoneTextWidth = MeasureText(DoneText, fontsize);
                DrawText(DoneText, w / 2 - DoneTextWidth / 2, dest.y + dest.height + 2 * fontsize, fontsize, RAYWHITE);
            }
            if (LoadImageDropped())
            {
                ImageLoaded = true;
            }
            DrawFPS(0, 0);
            EndDrawing();
        }
    }
    UnloadImage(image);
    UnloadTexture(texture);
    CloseWindow();
    return 0;
}
