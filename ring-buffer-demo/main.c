#include <stdio.h>
#include "ring_buffer.h"

// 测试环形 buffer。
void test_ring_buffer();

int main() {

    test_ring_buffer();

    return 0;
}

void test_ring_buffer() {
    printf("\n");

    const int x = 8, y = 9;
    int array[x][y] = {0};
    for (int xi = 0; xi < x; ++xi) {
        for (int yi = 0; yi < y; ++yi) {
            array[xi][yi] = (xi + 1) * 10 + yi;
            printf("%d ", array[xi][yi]);
        }
        printf("\n");
    }

    printf("\n");

    int out[x] = {0};

    RingBuffer *ringBuffer = WebRtc_CreateBuffer(y * 2, sizeof(int));
    WebRtc_InitBuffer(ringBuffer);

    for (int i = 0; i < x; ++i) {
        WebRtc_WriteBuffer(ringBuffer, array[i], y);
        while (x <= WebRtc_available_read(ringBuffer)) {
            WebRtc_ReadBuffer(ringBuffer, NULL, out, x);
            for (int j = 0; j < x; ++j) {
                printf("%d ", out[j]);
            }
            printf("\n");
        }
    }

    WebRtc_FreeBuffer(ringBuffer);
}