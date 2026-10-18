#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <stdint.h>
#include "config.h"
#include "log.h"
#include "omp.h"

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

#define INV_DOUBLE_SQRT2 0.3535533906f
#define SQRT2_DIV_DOUBLE_SQRT2 0.5f
#define SQRT2 1.4142135624f

static const float v[9][9] = {

    // W1 - ISOTROPIC AVERAGE GRADIENT
    {
        1.0f, SQRT2, 1.0f,
        0.0f, 0.0f,  0.0f,
       -1.0f, -SQRT2,-1.0f
    },

    // W2 - ISOTROPIC AVERAGE GRADIENT
    {
        1.0f, 0.0f, -1.0f,
        SQRT2, 0.0f, -SQRT2,
        1.0f, 0.0f, -1.0f
    },

    // W3 - Ripple operator
    {
        0.0f,  -1.0f,   SQRT2,
        1.0f,   0.0f,   -1.0f,
       -SQRT2,  1.0f,   0.0f
    },

    // W4 - Ripple operator
    {
        SQRT2,  -1.0f,   0.0f,
        -1.0f,   0.0f,   1.0f,
        0.0f,   1.0f,   -SQRT2
    },

    // W5 - Line
    {
        0.0f,   1.0f,   0.0f,
       -1.0f,   0.0f,  -1.0f,
        0.0f,   1.0f,   0.0f
    },

    // W6 - Line
    {
       -1.0f,   0.0f,   1.0f,
        0.0f,   0.0f,   0.0f,
        1.0f,   0.0f,  -1.0f
    },


    // W7 - Discrete Laplacian
    {
        1.0f,  -2.0f,   1.0f,
       -2.0f,   4.0f,  -2.0f,
        1.0f,  -2.0f,   1.0f
    },

    // W8 - Discrete Laplacian
    {
       -2.0f,   1.0f,  -2.0f,
        1.0f,   4.0f,   1.0f,
       -2.0f,   1.0f,  -2.0f
    },

    // W9 - Average
    {
        1.0f,   1.0f,   1.0f,
        1.0f,   1.0f,   1.0f,
        1.0f,   1.0f,   1.0f
    }
};


static void draw_line(image_mat image_map, int x0, int y0, int x1, int y1)
{
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);

    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;

    int error = dx - dy;

    int width = (int)image_map->width;
    int height = (int)image_map->height;

    for (;;)
    {
        if (x0 >= 0 && x0 < width &&
            y0 >= 0 && y0 < height)
        {
            image_map->data[y0 * width + x0] = 255;
        }

        if (x0 == x1 && y0 == y1)
            break;

        int e2 = 2 * error;

        if (e2 > -dy)
        {
            error -= dy;
            x0 += sx;
        }

        if (e2 < dx)
        {
            error += dx;
            y0 += sy;
        }
    }
}


void draw_arrow(image_mat image_map, int x0, int y0, int x1, int y1)
{
    /* Основная линия */
    draw_line(image_map, x0, y0, x1, y1);

    /* Направление линии */
    float angle = atan2f((float)(y1 - y0), (float)(x1 - x0));

    int arrow_length = 5;
    float arrow_angle = 0.7f;

    /* Левая часть наконечника */
    int ax1 = x1 - (int)(cosf(angle + arrow_angle) * arrow_length);

    int ay1 = y1 - (int)(sinf(angle + arrow_angle) * arrow_length);

    /* Правая часть наконечника */
    int ax2 = x1 - (int)(cosf(angle - arrow_angle) * arrow_length);

    int ay2 = y1 - (int)(sinf(angle - arrow_angle) * arrow_length);

    draw_line(image_map, x1, y1, ax1, ay1);
    draw_line(image_map, x1, y1, ax2, ay2);
}


void create_map_direction(image_mat image_map, double* angle_frei_chen)
{   
    unsigned int width = image_map->width;
    unsigned int height = image_map->height;

    int length = 7;
    int step = 13; 
    
    for (unsigned int x = 0; x < width - 2; x+=step)
    {
        for (unsigned int y = 0; y < height - 2; y+=step)
        {
            int x1 = (int)x + 1;
            int y1 = (int)y + 1;

            double angle = angle_frei_chen[y1 * width + x1];

            int x2 = x1 + (int)(cos(angle) * length);
            int y2 = y1 - (int)(sin(angle) * length);

            draw_arrow(image_map, x1, y1, x2, y2);
        }
    }
}



void operator_frei_chen(image_mat image_src, image_mat image_dst_edge, image_mat image_dst_line, double* angle_frei_chen)
{
    unsigned int width = image_src->width;
    unsigned int height = image_src->height;

    float gy, gx, magnitude_edge, magnitude_line;

    
    for (unsigned int x = 0; x < width - 2; x++)
    {
        for (unsigned int y = 0; y < height -2; y++)
        {
            // 8 фильтров Фрей-Чена
            float sum_vi_mul_b[8] = {0.0f};
            float total_energy = 0.0f;

            // Расчет полной энергии пикселей в окне 3x3
            for (int i = 0; i < 3; i++)
            {
                for (int j = 0; j < 3; j++)
                {
                    float b = (float)image_src->data[(y + i) * width + (x + j)];
                    total_energy += b * b;
                }
            }

            // Выполнение свертки с фильтрами Фрей-Чена
            for (int k = 0; k < 8; k++)
            {
                for (int i = 0; i < 3; i++)
                {
                    for (int j = 0; j < 3; j++)
                    {
                        float b = (float)image_src->data[(y + i) * width + (x + j)];
                        sum_vi_mul_b[k] += v[k][i * 3 + j] * b;                   
                    }
                }
            }

            float threshold = 0.3f;
            // Расчет энергии и статистики для границ
            float energy_edge = sum_vi_mul_b[0] * sum_vi_mul_b[0] + sum_vi_mul_b[1] * sum_vi_mul_b[1] + sum_vi_mul_b[2] * sum_vi_mul_b[2] + sum_vi_mul_b[3] * sum_vi_mul_b[3];

            // Расчет энергии и статистики для линий
            float energy_line = sum_vi_mul_b[4] * sum_vi_mul_b[4] + sum_vi_mul_b[5] * sum_vi_mul_b[5] + sum_vi_mul_b[6] * sum_vi_mul_b[6] + sum_vi_mul_b[7] * sum_vi_mul_b[7];
            
            float max_energy = 255 * 255 * 9;
            float min_energy_threshold = max_energy / 500.0f; 

            float angle = atan2f(gy, gx);
            if (total_energy > min_energy_threshold && angle > M_PI_2)
            {
                // Считаем контуры
                float stat_edge = energy_edge / total_energy;       
                if (stat_edge > 1.0f) stat_edge = 1.0f;
                if (stat_edge < threshold) stat_edge = 0.0f;
                image_dst_edge->data[(y + 1) * width + (x + 1)] = (uint8_t)(stat_edge * 255.0f);
                magnitude_edge = sqrtf(energy_edge);

                // Считаем линии
                float stat_line = energy_line / total_energy;
                if (stat_line > 1.0f) stat_line = 1.0f;
                if (stat_line < threshold) stat_line = 0.0f;
                image_dst_line->data[(y + 1) * width + (x + 1)] = (uint8_t)(stat_line * 255.0f);
                magnitude_line = sqrtf(energy_line);
            }
            else
            {
                // Если энергии не хватает — это гарантированно темнота или плоский фон
                image_dst_edge->data[(y + 1) * width + (x + 1)] = 0;
                image_dst_line->data[(y + 1) * width + (x + 1)] = 0;
            }


            // Расчет угла градиента
            gy = sum_vi_mul_b[0];
            gx = sum_vi_mul_b[1];
            
            angle_frei_chen[(y + 1) * width + (x + 1)] = atan2f(gy, gx);
        }
    }
}



int main(void)
{
    for (int i = 1; i < 6; i++)
    {
        // Загрузка изображения и получение его параметров
        int width, height, channels;
        char img_path[256], save_path[256];
        sprintf(img_path, ".\\resources\\src\\image_%d.png", i);
        sprintf(save_path, ".\\resources\\result\\output_%d.png", i);

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

        double* angle_frei_chen = (double*)malloc(width * height * sizeof(double));
        
        clock_t start = clock();

        image_mat edge_frei_chen = new_image(image_src->width, image_src->height, cn_grayscale);
        image_mat line_frei_chen = new_image(image_src->width, image_src->height, cn_grayscale);
        image_mat direction_map = new_image(image_src->width, image_src->height, cn_grayscale);
        
        operator_frei_chen(image_src, edge_frei_chen, line_frei_chen, angle_frei_chen);
        create_map_direction(direction_map, angle_frei_chen);

        clock_t end = clock();
        double cpu_time_used = ((double) end - start) / CLOCKS_PER_SEC;
        printf("\nTime: %f sec\n", cpu_time_used);

        // Сохранение резульата
        if (stbi_write_png(save_path,
            line_frei_chen->width, line_frei_chen->height, line_frei_chen->cn, line_frei_chen->data, line_frei_chen->width * line_frei_chen->cn)) 
        {
            printf("\nSuccessfully saved: %s\n", save_path);
        }
        else 
        {   
            printf("Save error: %d\n");

            free_image(image_src);

            return 1;
        }


        free_image(image_src);
        free_image(edge_frei_chen);
        free_image(line_frei_chen);
        free_image(direction_map);
        free(angle_frei_chen);
    }

    return 0;
}


