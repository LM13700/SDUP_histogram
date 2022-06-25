/*
 * main.c: simple test application
 *
 * This application configures UART 16550 to baud rate 9600.
 * PS7 UART (Zynq) is not initialized by this application, since
 * bootrom/bsp configures it to baud rate 115200
 *
 * ------------------------------------------------
 * | UART TYPE   BAUD RATE                        |
 * ------------------------------------------------
 *   uartns550   9600
 *   uartlite    Configurable only in HW design
 *   ps7_uart    115200 (configured by bootrom/bsp)
 */

#include <stdio.h>
#include <stdbool.h>
#include "platform.h"
#include "xil_printf.h"

#define ARRAY_LENGTH(_ARR_)     (sizeof(_ARR_) / sizeof((_ARR_)[0]))
#define HISTOGRAM_LENGTH        (256u)

#define COMMUNICATION_MARKER    (0xAA)

void Histogram_Reset(void);
bool Histogram_WritePixels(u8* pixels, size_t pixelsLength);
bool Histogram_ReadHistogram(u16* histogram, size_t histogramLength);

void waitForMarker(void)
{
    bool isReceived;
    char8 c;

    isReceived = false;

    while (!isReceived)
    {
        c = inbyte();

        if (c == COMMUNICATION_MARKER)
        {
            isReceived = true;
            xil_printf("OK\n\r");
        }
    }
}

u16 readPixelNum(void)
{
    char8 c1;
    char8 c2;

    c1 = inbyte();
    c2 = inbyte();

    return (u16)((c1 << 8) | (c2));
}

/**
 *  printDecimalFXPVal - print fixed-point value in decimal format
 *  val - value to print out in radix-2 fixed-point
 *  scale - Fixed-point scaling factor
 *	nbr_of_decimal_digit - number precision. The number of digits after decimal point
 */


int main()
{
    u8 pixel;
    u16 pixelNumber;
    u16 histogram[HISTOGRAM_LENGTH];
    size_t index;

    memset(histogram, 0u, sizeof(histogram));
    init_platform();

    while(1)
    {
        Histogram_Reset();
        waitForMarker();
        pixelNumber = readPixelNum();
        xil_printf("%d\n\r", pixelNumber);

        for (index = 0u; index < pixelNumber; index++)
        {
            pixel = inbyte();
            Histogram_WritePixels(&pixel, sizeof(pixel));
            xil_printf("OK\n\r");
        }

        Histogram_ReadHistogram(histogram, ARRAY_LENGTH(histogram));

        for (index = 0u; index < ARRAY_LENGTH(histogram); index++)
        {
            xil_printf("Bin%d: %u\n\r", index, histogram[index]);
        }

        xil_printf("Done\n\r");
    }
}
