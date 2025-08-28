#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>

#include "tjpgd.h"
#include "zjpgd.h"


typedef struct {
    uint16_t x;
    uint16_t y;
    zjd_ctx_t snapshot;
} image_roi_t;

typedef struct {
    struct {
        uint8_t *data;  // Pointer to JPEG data
        size_t size;         // Size of JPEG data
        size_t offset;       // Current offset in the data
    } ifile;
    struct {
        uint8_t *data;  // Pointer to JPEG data
        size_t size;         // Size of JPEG data
        size_t offset;       // Current offset in the data
        size_t pixels;
    } ofile;
    image_roi_t rois[3];
} image_t;

int load_jpeg(const char *filename, image_t *img) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        perror("Failed to open file");
        return -1;
    }

    fseek(file, 0, SEEK_END);
    img->ifile.size = ftell(file);
    fseek(file, 0, SEEK_SET);

    img->ifile.data = (uint8_t *)malloc(img->ifile.size);
    if (!img->ifile.data) {
        perror("Failed to allocate memory");
        fclose(file);
        return -1;
    }

    fread((void *)img->ifile.data, 1, img->ifile.size, file);
    fclose(file);
    img->ifile.offset = 0;

    img->ofile.pixels = 0;
    img->ofile.size = 4096 * 4096 * 3;
    img->ofile.data = (uint8_t *)malloc(img->ofile.size);
    if (!img->ofile.data) {
        perror("Failed to allocate memory for output");
        free(img->ifile.data);
        return -1;
    }


    memset(img->ofile.data, 0, img->ofile.size);

    return 0;
}

int save_bmp(const char *filename, image_t *img, int width, int height) {
    FILE *file = fopen(filename, "wb");
    if (!file) {
        perror("Failed to open file for writing");
        return -1;
    }

    // BMP Header
    uint8_t bmp_header[54] = {
        0x42, 0x4D,             // Signature 'BM'
        0, 0, 0, 0,             // File size in bytes
        0, 0,                   // Reserved
        0, 0,                   // Reserved
        54, 0, 0, 0,            // Offset to pixel data
        40, 0, 0, 0,            // Info header size
        0, 0, 0, 0,             // Width
        0, 0, 0, 0,             // Height
        1, 0,                   // Planes
        24, 0,                  // Bits per pixel
        0, 0, 0, 0,             // Compression (none)
        0, 0, 0, 0,             // Image size (can be zero for uncompressed)
        0x13, 0x0B, 0, 0,       // Horizontal resolution (pixels per meter)
        0x13, 0x0B, 0, 0,       // Vertical resolution (pixels per meter)
        0, 0, 0, 0,             // Number of colors in palette (none)
        0, 0, 0, 0              // Important colors (all)
    };

    int row_size = (width * 3 + 3) & ~3; // Row size aligned to multiple of 4 bytes
    int pixel_data_size = row_size * height;
    int file_size = pixel_data_size + sizeof(bmp_header);

    // Fill in file size
    bmp_header[2] = (uint8_t)(file_size & 0xFF);
    bmp_header[3] = (uint8_t)((file_size >> 8) & 0xFF);
    bmp_header[4] = (uint8_t)((file_size >> 16) & 0xFF);
    bmp_header[5] = (uint8_t)((file_size >> 24) & 0xFF);
    // Fill in width
    bmp_header[18] = (uint8_t)(width & 0xFF);
    bmp_header[19] = (uint8_t)((width >> 8) & 0xFF);
    bmp_header[20] = (uint8_t)((width >> 16) & 0xFF);
    bmp_header[21] = (uint8_t)((width >> 24) & 0xFF);
    // Fill in height
    bmp_header[22] = (uint8_t)(height & 0xFF);
    bmp_header[23] = (uint8_t)((height >> 8) & 0xFF);
    bmp_header[24] = (uint8_t)((height >> 16) & 0xFF);
    bmp_header[25] = (uint8_t)((height >> 24) & 0xFF);
    // Fill in image size
    bmp_header[34] = (uint8_t)(pixel_data_size & 0xFF);

    fwrite(bmp_header, sizeof(bmp_header), 1, file);
    // Write pixel data (BMP stores pixels in BGR format and bottom-up)
    for (int y = height - 1; y >= 0; y--) {
        for (int x = 0; x < width; x++) {
            int index = (x + y * width) * 3;
            uint8_t bgr[3] = {
                img->ofile.data[index + 2], // Blue
                img->ofile.data[index + 1], // Green
                img->ofile.data[index + 0]  // Red
            };
            fwrite(bgr, sizeof(bgr), 1, file);
        }
        // Padding for 4-byte alignment
        uint8_t padding[3] = {0, 0, 0};
        fwrite(padding, row_size - width * 3, 1, file);
    }
    fclose(file);
    return 0;
}


size_t tjd_ifunc(JDEC *tjd, uint8_t *buf, size_t len) {
    image_t *img = (image_t *)tjd->device;
    size_t remaining = img->ifile.size - img->ifile.offset;

    if (len > remaining) {
        len = remaining;
    }

    if (buf) {
        memcpy(buf, img->ifile.data + img->ifile.offset, len);
        img->ifile.offset += len;
    } else {
        img->ifile.offset += len;
    }

    return len;
}

int tjd_ofunc(JDEC *tjd, void *bitmap, JRECT *rect) {
    image_t *img = (image_t *)tjd->device;
    uint8_t *pix = (uint8_t *)bitmap;
    int x, y, index;

    for (y = rect->top; y <= rect->bottom; y++) {
        for (x = rect->left; x <= rect->right; x++) {
            index = x + y * tjd->width;
            img->ofile.data[index * 3 + 0] = *pix++;
            img->ofile.data[index * 3 + 1] = *pix++;
            img->ofile.data[index * 3 + 2] = *pix++;

            img->ofile.pixels++;
        }
    }
    return 1;
}

int tjd_test(JDEC *tjd, void *work, size_t worksize, image_t *img)
{
    JRESULT res;

    res = jd_prepare(tjd, tjd_ifunc, work, worksize, img);
    if (res != JDR_OK) {
        printf("Failed to prepare JPEG decoder %u\n", res);
        free(img->ifile.data);
        free(img->ofile.data);
        return 1;
    }
    res = jd_decomp(tjd, tjd_ofunc, 0);
    if (res != JDR_OK) {
        printf("Failed to decode JPEG image %u\n", res);
        free(img->ifile.data);
        free(img->ofile.data);
        return 1;
    }

    return 0;
}

int zjd_ifunc(zjd_t *zjd, uint8_t *buf, uint32_t addr, int len)
{
    image_t *img = (image_t *)zjd->arg;

    size_t remaining = img->ifile.size - addr;

    if (len > remaining) {
        len = remaining;
    }

    if (buf) {
        memcpy(buf, img->ifile.data + addr, len);
    }

    return len;
}

int zjd_ofunc(zjd_t *zjd, zjd_rect_t *rect, void *pixels)
{
    image_t *img = (image_t *)zjd->arg;
    uint8_t *pix = (uint8_t *)pixels;
    int x, y, index;

    printf("Decoded rect: (%d,%d)-(%d,%d)\n", rect->x, rect->y, rect->w, rect->h);
    // for (y = rect->y; y < rect->y + rect->h; y++) {
    //     for (x = rect->x; x < rect->x + rect->w; x++) {
    //         if (x >= zjd->width || y >= zjd->height) {
    //             // Out of bounds, skip
    //             pix += 3;
    //             continue;
    //         }
    //         index = x + y * zjd->width;
    //         img->ofile.data[index * 3 + 0] = *pix++;
    //         img->ofile.data[index * 3 + 1] = *pix++;
    //         img->ofile.data[index * 3 + 2] = *pix++;

    //         img->ofile.pixels++;
    //     }
    // }

    return 1;
}

int zjd_ofunc_snapshot(zjd_t *zjd, zjd_rect_t *rect, void *pixels)
{
    int i, cnt = 0;
    image_t *img = (image_t *)zjd->arg;
    image_roi_t *roi;


    for (i = 0; i < sizeof(img->rois) / sizeof(img->rois[0]); i++) {
        roi = &img->rois[i];
        if (roi->snapshot.offset != 0) {
            cnt++;
            continue;
        }

        if (roi->snapshot.offset == 0) {
            /* roi in x,y,w,h */
            if (roi->x >= rect->x && roi->x < rect->x + rect->w &&
                roi->y >= rect->y && roi->y < rect->y + rect->h) {
                zjd_save(zjd, &roi->snapshot);
            }
        }
    }

    // All snapshots taken, stop further processing
    if (cnt == sizeof(img->rois) / sizeof(img->rois[0])) {
        return 0;
    }

    return 1;
}

int zjd_test(zjd_t *zjd, void *work, size_t worksize, image_t *img)
{
    zjd_res_t res;

    res = zjd_init(
        zjd,
        &(zjd_cfg_t){
            .outfmt = ZJD_RGB888,
            .ifunc = zjd_ifunc,
            .ofunc = zjd_ofunc,
            .buf = work,
            .buflen = worksize,
            .arg = (void *)img
        }
    );
    if (res != ZJD_OK) {
        printf("Failed to initialize zjpgd %d\n", res);
        free(img->ifile.data);
        free(img->ofile.data);
        return 1;
    }
    res = zjd_scan(zjd, NULL, NULL);
    if (res != ZJD_OK) {
        printf("Failed to decode JPEG image %d\n", res);
        free(img->ifile.data);
        free(img->ofile.data);
        return 1;
    }

    return 0;
}

int zjd_roi_test(zjd_t *zjd, void *work, size_t worksize, image_t *img)
{
    zjd_res_t res;
    zjd_ctx_t snapshot;

    snapshot.offset = zjd->imgoft;
    snapshot.dreg = 0;
    snapshot.dbit = 0;
    snapshot.mcu_x = 0;
    snapshot.mcu_y = 0;
    snapshot.dcv[0] = 0;
    snapshot.dcv[1] = 0;
    snapshot.dcv[2] = 0;

    res = zjd_init(
        zjd,
        &(zjd_cfg_t){
            .outfmt = ZJD_RGB888,
            .ifunc = zjd_ifunc,
            .ofunc = zjd_ofunc,
            .buf = work,
            .buflen = worksize,
            .arg = (void *)img
        }
    );
    if (res != ZJD_OK) {
        printf("Failed to initialize zjpgd %d\n", res);
        free(img->ifile.data);
        free(img->ofile.data);
        return 1;
    }
    res = zjd_scan(zjd, &snapshot, NULL);
    if (res != ZJD_OK) {
        printf("Failed to decode JPEG image %d\n", res);
        free(img->ifile.data);
        free(img->ofile.data);
        return 1;
    }

    return 0;
}

int zjd_roi_test_init(zjd_t *zjd, void *work, size_t worksize, image_t *img)
{
    zjd_res_t res;

    res = zjd_init(
        zjd,
        &(zjd_cfg_t){
            .outfmt = ZJD_RGB888,
            .ifunc = zjd_ifunc,
            .ofunc = zjd_ofunc,
            .buf = work,
            .buflen = worksize,
            .arg = (void *)img
        }
    );
    if (res != ZJD_OK) {
        printf("Failed to initialize zjpgd %d\n", res);
        free(img->ifile.data);
        free(img->ofile.data);
        return 1;
    }

    return 0;
}

int zjd_roi_1_4_test(zjd_t *zjd, void *work, size_t worksize, image_t *img, zjd_ctx_t *snapshot, zjd_rect_t *roi)
{
    zjd_res_t res;

    // res = zjd_init(
    //     zjd,
    //     &(zjd_cfg_t){
    //         .outfmt = ZJD_RGB888,
    //         .ifunc = zjd_ifunc,
    //         .ofunc = zjd_ofunc,
    //         .buf = work,
    //         .buflen = worksize,
    //         .arg = (void *)img
    //     }
    // );
    // if (res != ZJD_OK) {
    //     printf("Failed to initialize zjpgd %d\n", res);
    //     free(img->ifile.data);
    //     free(img->ofile.data);
    //     return 1;
    // }
    res = zjd_scan(zjd, snapshot, roi);
    if (res != ZJD_OK) {
        printf("Failed to decode JPEG image %d\n", res);
        free(img->ifile.data);
        free(img->ofile.data);
        return 1;
    }

    return 0;
}

int zjd_get_snapshots(zjd_t *zjd, void *work, size_t worksize, image_t *img)
{
    zjd_res_t res;

    res = zjd_init(
        zjd,
        &(zjd_cfg_t){
            .outfmt = ZJD_RGB888,
            .ifunc = zjd_ifunc,
            .ofunc = zjd_ofunc_snapshot,
            .buf = work,
            .buflen = worksize,
            .arg = (void *)img
        }
    );
    if (res != ZJD_OK) {
        printf("Failed to initialize zjpgd %d\n", res);
        free(img->ifile.data);
        free(img->ofile.data);
        return 1;
    }

    /* scan with zjd_ofunc_snapshot to get all snapshots */
    res = zjd_scan(zjd, NULL, NULL);
    if (res != ZJD_OK) {
        printf("Failed to decode JPEG image %d\n", res);
        free(img->ifile.data);
        free(img->ofile.data);
        return 1;
    }

    return 0;
}

uint32_t micros() {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);  // or CLOCK_MONOTONIC for relative time
    return (uint32_t)((long long)ts.tv_sec * 1000000LL + ts.tv_nsec / 1000);
}

int main(int argc, char **argv)
{
    image_t img;
    int ret, i, j, rounds;
    uint32_t us;

    uint8_t work[4096];

    JDEC tjd;
    JRESULT tjd_res;
    uint32_t tjd_cost_us;

    zjd_t zjd;
    zjd_res_t zjd_res;
    uint32_t zjd_cost_us;
    uint32_t zjd_roi_cost_us;
    uint32_t zjd_roi_1_4_cost_us[3];
    zjd_rect_t roi_rect[3];

    if (argc < 2) {
        printf("Usage: %s <jpg_file>\n", argv[0]);
        return 1;
    }

    ret = load_jpeg(argv[1], &img);
    if (ret != 0) {
        printf("Failed to load JPEG file\n");
        return 1;
    }

    if (argc > 2) {
        rounds = atoi(argv[2]);
        if (rounds < 1) rounds = 1;
    } else {
        rounds = 100;
    }
    printf("\nDecoding %s for %d rounds\n", argv[1], rounds);

    us = micros();
    for (i = 0; i < rounds; i++) {
        img.ifile.offset = 0;
        img.ofile.offset = 0;
        img.ofile.pixels = 0;
        memset(img.ofile.data, 0, img.ofile.size);
        ret = tjd_test(&tjd, work, sizeof(work), &img);
        if (ret != 0) {
            printf("tjd_test failed with error code %d\n", ret);
            free(img.ifile.data);
            free(img.ofile.data);
            return 1;
        }
    }
    tjd_cost_us = (micros() - us) / rounds;
    printf("tjpgd %ux%u decode time: %u us, %u pixels processed\n", tjd.width, tjd.height, tjd_cost_us, (uint32_t)img.ofile.pixels);
    save_bmp("output_tjpgd.bmp", &img, tjd.width, tjd.height);

    us = micros();
    for (i = 0; i < rounds; i++) {
        img.ifile.offset = 0;
        img.ofile.offset = 0;
        img.ofile.pixels = 0;
        memset(img.ofile.data, 0, img.ofile.size);
        ret = zjd_test(&zjd, work, sizeof(work), &img);
        if (ret != 0) {
            printf("zjd_test failed with error code %d\n", ret);
            free(img.ifile.data);
            free(img.ofile.data);
            return 1;
        }
    }
    zjd_cost_us = (micros() - us) / rounds;
    printf("zjpgd %ux%u decode time: %u us, %u pixels processed\n", zjd.width, zjd.height, zjd_cost_us, (uint32_t)img.ofile.pixels);
    save_bmp("output_zjpgd.bmp", &img, zjd.width, zjd.height);

    us = micros();
    for (i = 0; i < rounds; i++) {
        img.ifile.offset = 0;
        img.ofile.offset = 0;
        img.ofile.pixels = 0;
        memset(img.ofile.data, 0, img.ofile.size);
        ret = zjd_roi_test(&zjd, work, sizeof(work), &img);
        if (ret != 0) {
            printf("zjd_roi_test failed with error code %d\n", ret);
            free(img.ifile.data);
            free(img.ofile.data);
            return 1;
        }
    }
    zjd_roi_cost_us = (micros() - us) / rounds;
    printf("zjpgd_roi %ux%u decode time: %u us, %u pixels processed\n", zjd.width, zjd.height, zjd_roi_cost_us, (uint32_t)img.ofile.pixels);
    save_bmp("output_zjpgd_roi.bmp", &img, zjd.width, zjd.height);

    /* central 1/4 */
    roi_rect[0].x = zjd.width / 4;
    roi_rect[0].y = zjd.height / 4;
    roi_rect[0].w = zjd.width / 2;
    roi_rect[0].h = zjd.height / 2;
    img.rois[0].x = roi_rect[0].x;
    img.rois[0].y = roi_rect[0].y;

    /* top left corner */
    roi_rect[1].x = 0;
    roi_rect[1].y = 0;
    roi_rect[1].w = zjd.width / 2;
    roi_rect[1].h = zjd.height / 2;
    img.rois[1].x = roi_rect[1].x;
    img.rois[1].y = roi_rect[1].y;

    /* bottom right corner */
    roi_rect[2].x = zjd.width / 2;
    roi_rect[2].y = zjd.height / 2;
    roi_rect[2].w = zjd.width / 2;
    roi_rect[2].h = zjd.height / 2;
    img.rois[2].x = roi_rect[2].x;
    img.rois[2].y = roi_rect[2].y;

    zjd_get_snapshots(&zjd, work, sizeof(work), &img);

    for (i = 0; i < sizeof(img.rois) / sizeof(img.rois[0]); i++) {
        if (img.rois[i].snapshot.offset == 0) {
            printf("Failed to get snapshot %d\n", i);
            free(img.ifile.data);
            free(img.ofile.data);
            return 1;
        }
        printf("Snapshot %d at (%u,%u) saved at mcu (%u,%u)\n", i, img.rois[i].x, img.rois[i].y, img.rois[i].snapshot.mcu_x, img.rois[i].snapshot.mcu_y);
        printf("Snapshot dimensions: (%u,%u) to (%u,%u)\n", roi_rect[i].x, roi_rect[i].y, roi_rect[i].x + roi_rect[i].w, roi_rect[i].y + roi_rect[i].h);
    }

    /* common roi test init, below roi test function won't call zjd_init again */
    zjd_roi_test_init(&zjd, work, sizeof(work), &img);

    for (j = 0; j < sizeof(img.rois) / sizeof(img.rois[0]); j++) {
        us = micros();
        for (i = 0; i < rounds; i++) {
            img.ifile.offset = 0;
            img.ofile.offset = 0;
            img.ofile.pixels = 0;
            memset(img.ofile.data, 0, img.ofile.size);
            ret = zjd_roi_1_4_test(&zjd, work, sizeof(work), &img, &img.rois[j].snapshot, &roi_rect[j]);
            if (ret != 0) {
                printf("zjd_roi_1_4_test %d failed with error code %d\n", j, ret);
                free(img.ifile.data);
                free(img.ofile.data);
                return 1;
            }
        }
        zjd_roi_1_4_cost_us[j] = (micros() - us) / rounds;
        printf("zjpgd_roi_1_4 %d (%u,%u,%u,%u) decode time: %u us, %u pixels processed\n", j, roi_rect[j].x, roi_rect[j].y, roi_rect[j].w, roi_rect[j].h, zjd_roi_1_4_cost_us[j], (uint32_t)img.ofile.pixels);
        char filename[64];
        snprintf(filename, sizeof(filename), "output_zjpgd_roi_1_4_%d.bmp", j);
        save_bmp(filename, &img, zjd.width, zjd.height);
    }

    printf("file %s,%u,%u,%u,%u,%u,%u,%u\n", argv[1], rounds,
        tjd_cost_us,
        zjd_cost_us,
        zjd_roi_cost_us,
        zjd_roi_1_4_cost_us[0],
        zjd_roi_1_4_cost_us[1],
        zjd_roi_1_4_cost_us[2]
    );

    free(img.ifile.data);
    free(img.ofile.data);

    return 0;
}
