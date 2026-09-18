#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <stdint.h>
#include "config.h"
#include "log.h"


#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#ifndef STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#endif 


//NOTE ======================= Константы ==============================
#define M_PI_MY 3.14159265358979323846

typedef enum
{
    cn_grayscale = 1,
    cn_rgb = 3

} channel;

typedef struct image_mat_structure
{
    unsigned char *data;
    unsigned int width;
    unsigned int height;
    channel cn;

} *image_mat;



//NOTE ============== Методы работы с изображениями ==============  


/// @brief Создание и инициализация изображения нулями
/// @param width Ширина изображения
/// @param height Высота изображения
/// @param channels Каналы (1 - gray, 3 - rgb)
/// @return Инициализированное изображение
image_mat new_image(unsigned int width, unsigned int height, channel channels)
{
    if (width == 0 || height == 0 || channels == 0) print_error("new_image: invalid input parameters.");
       
    image_mat image = (image_mat)malloc(sizeof(struct image_mat_structure));
    if (image == NULL) print_error("new_image: not enough memory.");
    
    image->width = width;
    image->height = height;
    image->cn = channels;

    image->data = (unsigned char *)calloc(width * height * channels, sizeof(unsigned char));
    if (image->data == NULL) print_error("new_image: not enough memory.");

    return image;
}



/// @brief Освобождение памяти, выделенное под изображение
/// @param image Входное изображение
void free_image(image_mat image)
{
    if (image == NULL || image->data == NULL)
    {
        print_error("image_mat: invalid input image_mat.");
    }

    free(image->data);
    free(image);
}


int vertical_kernel[9] = 
{
        -1, -1, -1,
         0,  0,  0,
         1,  1,  1
};

int horizontal_kernel[9] = 
{
        -1, 0, 1,
        -1, 0, 1,
        -1, 0, 1
};

void operator_frei_chen(image_mat image_src, image_mat image_dst)
{
    int width = image_src->width;
    int height = image_src->height;

    for (int x = 0; x < width; x++)
    {
        for (int y = 0; y < height; y++)
        {

            int Gy = 0, Gx = 0;
            for (int i = 0; i < 3; i++)
            {
                for (int j = 0; j < 3; j++)
                {
                    int kv = vertical_kernel[i * 3 + j];
                    int kh = horizontal_kernel[i * 3 + j];

                    if (kv < 0)
                    {
                        Gy += -(image_src->data[(y + i) * width + (x + j)] >> kv);
                    }
                    else
                    {
                        Gy += image_src->data[(y + i) * width + (x + j)] >> kv;
                    }

                    if (kh < 0)
                    {
                        Gx += -(image_src->data[(y + i) * width + (x + j)] >> kh);
                    }
                    else
                    {
                        Gx += image_src->data[(y + i) * width + (x + j)] >> kh;
                    }
                }
            }

           image_dst->data[y * width + x] = (uint8_t)sqrt(Gx * Gx + Gy * Gy);
        }
    }
}

int main(void)
{
   
    // Загрузка изображения и получение его параметров
    int width, height, channels;
    char img_path[256], save_path[256];
    sprintf(img_path, ".\\resources\\result\\output_5.png");
    sprintf(save_path, ".\\resources\\result\\output_1_labeled.png");

    unsigned char *data = stbi_load(img_path, &width, &height, &channels, 0);
    if (data == NULL)
    {
        printf("Error: %s\n", stbi_failure_reason());
        return 1;
    }

    // Создание исходного изображения
    image_mat image_src = new_image(width, height, channels);
    memcpy(image_src->data, data, width * height * channels);
    stbi_image_free(data);

    clock_t start = clock();

    image_mat sobel = new_image(image_src->width, image_src->height, 1);
    operator_frei_chen(image_src, sobel);

    clock_t end = clock();
    double cpu_time_used = ((double) end - start) / CLOCKS_PER_SEC;
    printf("Time: %f sec\n", cpu_time_used);

    // Сохранение резульата
    if (stbi_write_png(save_path,
        sobel->width, sobel->height, sobel->cn, sobel->data, sobel->width * sobel->cn)) 
    {
        printf("Successfully");
    }
    else 
    {   
        printf("Save error: %d\n");

        free_image(image_src);

        return 1;
    }


    free_image(image_src);
    free_image(sobel);
    return 0;
}


