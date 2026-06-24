#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include "bcm_host.h"
#include "interface/vmcs_host/vc_dispmanx.h"

/* Docelowa rozdzielczosc wyjsciowa (854x480, ok. 1.17MB jako BMP 24-bit,
 * zostawia margines do limitu 1.5MB). Zmien tutaj, jesli potrzeba innej.
 * Downsampling robiony jest programowo (nearest-neighbor) z buforu
 * w pelnej rozdzielczosci ekranu - snapshot dispmanx pozostaje
 * niezmieniony wzgledem dzialajacej wersji oryginalnej. */
#define OUT_WIDTH  854
#define OUT_HEIGHT 480

/* Zapis BMP 24-bit (bez kanalu alfa), z poprawnym rozmiarem pliku
 * i poprawnym paddingiem wierszy do wielokrotnosci 4 bajtow,
 * zgodnie ze specyfikacja formatu BMP.
 * src_buffer  - bufor RGBA32 w pelnej rozdzielczosci ekranu (src_width x src_height)
 * out_width/out_height - docelowa, mniejsza rozdzielczosc zapisywana do pliku */
static void save_bmp_scaled(const char *filename, uint8_t *src_buffer,
                             int src_width, int src_height,
                             int out_width, int out_height)
{
    int row_size = out_width * 3;
    int padding = (4 - (row_size % 4)) % 4;
    int row_size_padded = row_size + padding;
    int filesize = 54 + row_size_padded * out_height;

    FILE *f = fopen(filename, "wb");
    if (!f) {
        printf("fopen failed for %s\n", filename);
        return;
    }

    unsigned char bmpfileheader[14] = {
        'B','M',
        (unsigned char)(filesize),       (unsigned char)(filesize>>8),
        (unsigned char)(filesize>>16),   (unsigned char)(filesize>>24),
        0,0,0,0,
        54,0,0,0
    };
    unsigned char bmpinfoheader[40] = {
        40,0,0,0,
        (unsigned char)(out_width),          (unsigned char)(out_width>>8),
        (unsigned char)(out_width>>16),      (unsigned char)(out_width>>24),
        (unsigned char)(out_height),         (unsigned char)(out_height>>8),
        (unsigned char)(out_height>>16),     (unsigned char)(out_height>>24),
        1,0,24,0
    };

    fwrite(bmpfileheader, 1, 14, f);
    fwrite(bmpinfoheader, 1, 40, f);

    unsigned char pad_bytes[3] = {0,0,0};

    /* BMP zapisuje wiersze od dolu do gory.
     * Nearest-neighbor: dla kazdego piksela wyjsciowego (x,y) bierzemy
     * najblizszy odpowiadajacy piksel z bufora source. */
    for (int y = out_height - 1; y >= 0; y--) {
        int src_y = (y * src_height) / out_height;
        for (int x = 0; x < out_width; x++) {
            int src_x = (x * src_width) / out_width;
            int i = (src_x + src_y * src_width) * 4; /* bufor wejsciowy to RGBA32 */
            fputc(src_buffer[i+2], f); /* B */
            fputc(src_buffer[i+1], f); /* G */
            fputc(src_buffer[i+0], f); /* R */
        }
        if (padding > 0) {
            fwrite(pad_bytes, 1, padding, f);
        }
    }

    fclose(f);
}

int main(int argc, char **argv)
{
    const char *out = (argc > 1) ? argv[1] : "screen.bmp";

    bcm_host_init();

    uint32_t screen_width, screen_height;
    if (graphics_get_display_size(0, &screen_width, &screen_height) < 0) {
        printf("graphics_get_display_size failed\n");
        return 1;
    }

    DISPMANX_DISPLAY_HANDLE_T display = vc_dispmanx_display_open(0);
    if (!display) {
        printf("vc_dispmanx_display_open failed\n");
        return 1;
    }

    /* Dopasowanie wyjsciowych wymiarow do orientacji ekranu - dluzsza
     * krawedz zawsze ograniczona do OUT_WIDTH (854px), zachowujac
     * proporcje. Dziala automatycznie dla landscape i portret,
     * bez zniekszalcania obrazu i bez przekroczenia limitu rozmiaru. */
    int out_w, out_h;
    if (screen_width >= screen_height) {
        out_w = OUT_WIDTH;
        out_h = (int)((screen_height * OUT_WIDTH) / screen_width);
    } else {
        out_h = OUT_WIDTH;
        out_w = (int)((screen_width * OUT_WIDTH) / screen_height);
    }
    if (out_w < 1) out_w = 1;
    if (out_h < 1) out_h = 1;

    /* Resource w PELNEJ rozdzielczosci ekranu - tak jak w oryginalnej,
     * dzialajacej wersji. Skalowanie do mniejszego pliku robimy
     * programowo w save_bmp_scaled, nie na poziomie GPU. */
    uint32_t vc_image_ptr;
    DISPMANX_RESOURCE_HANDLE_T res =
        vc_dispmanx_resource_create(VC_IMAGE_RGBA32, screen_width, screen_height, &vc_image_ptr);
    if (!res) {
        printf("vc_dispmanx_resource_create failed\n");
        vc_dispmanx_display_close(display);
        return 1;
    }

    if (vc_dispmanx_snapshot(display, res, 0) != 0) {
        printf("vc_dispmanx_snapshot failed\n");
        vc_dispmanx_resource_delete(res);
        vc_dispmanx_display_close(display);
        return 1;
    }

    VC_RECT_T rect;
    vc_dispmanx_rect_set(&rect, 0, 0, screen_width, screen_height);

    uint8_t *data = malloc(screen_width * screen_height * 4);
    if (!data) {
        printf("malloc failed\n");
        vc_dispmanx_resource_delete(res);
        vc_dispmanx_display_close(display);
        return 1;
    }

    if (vc_dispmanx_resource_read_data(res, &rect, data, screen_width * 4) != 0) {
        printf("vc_dispmanx_resource_read_data failed\n");
        free(data);
        vc_dispmanx_resource_delete(res);
        vc_dispmanx_display_close(display);
        return 1;
    }

    save_bmp_scaled(out, data, screen_width, screen_height, out_w, out_h);

    free(data);
    vc_dispmanx_resource_delete(res);
    vc_dispmanx_display_close(display);

    printf("Saved %s (%dx%d, source %ux%u)\n", out, out_w, out_h, screen_width, screen_height);
    return 0;
}
