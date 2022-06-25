/***************************** Include Files *********************************/
#include "xil_io.h"
#include "xparameters.h"
#include "histogram_ip.h"
#include <stdbool.h>

/**************************** User definitions ********************************/
#define HISTOGRAM_BASE_ADDR                         XPAR_HISTOGRAM_IP_0_S00_AXI_BASEADDR

#define INPUT_CONTROL_REG_OFFSET                    HISTOGRAM_IP_S00_AXI_SLV_REG0_OFFSET
#define INPUT_DATA_REG_OFFSET                       HISTOGRAM_IP_S00_AXI_SLV_REG1_OFFSET
#define OUT_STATUS_REG_OFFSET                       HISTOGRAM_IP_S00_AXI_SLV_REG2_OFFSET
#define OUT_RESULT_REG_OFFSET                       HISTOGRAM_IP_S00_AXI_SLV_REG3_OFFSET

/* Macros for setting flags in INPUT CONTROL register */
#define PIXEL_VALID_ON                              (1u)
#define PIXEL_VALID_OFF                             (0u)
#define HIST_RESET_ON                               ((1u) << 1)
#define HIST_RESET_OFF                              ((0u) << 1)
#define HIST_ADDRESS_VALID_ON                       ((1u) << 2)
#define HIST_ADDRESS_VALID_OFF                      ((0u) << 2)

/* Macros for getting flags from OUT STATUS register */
#define PIXEL_READ_MASK                             (1u)
#define HIST_OUT_VALID_MASK                         ((1u) << 1)

/* Macros for writing data to INPUT DATA register */
#define INPUT_DATA_WRITE_PIXEL(_VAL_)               (_VAL_)
#define INPUT_DATA_WRITE_ADDRESS(_VAL_)             ((_VAL_) << 8)

#define HISTOGRAM_WRITE_REGISTER(_REG_, _VAL_)      (HISTOGRAM_IP_mWriteReg(HISTOGRAM_BASE_ADDR, (_REG_), (_VAL_)))
#define HISTOGRAM_READ_REGISTER(_REG_)              (HISTOGRAM_IP_mReadReg(HISTOGRAM_BASE_ADDR, (_REG_)))

#define HISTOGRAM_FLAG_TIMEOUT                      (10u)
#define HISTOGRAM_LENGTH                            (256u)

/***************************** Local functions declarations *************************/
static bool Histogram_WaitForOutStatusFlag(u32 statusRegMask);

/***************************** Exported functions definitions ***********************/
void Histogram_Reset(void)
{
    HISTOGRAM_WRITE_REGISTER(INPUT_CONTROL_REG_OFFSET, 0u);

    HISTOGRAM_WRITE_REGISTER(INPUT_CONTROL_REG_OFFSET, HIST_RESET_ON);
    HISTOGRAM_WRITE_REGISTER(INPUT_CONTROL_REG_OFFSET, HIST_RESET_OFF);
}

bool Histogram_WritePixels(u8* pixels, size_t pixelsLength)
{
    size_t index;
    bool result;

    result = false;

    if (pixelsLength > HISTOGRAM_LENGTH)
    {
        goto histogram_write_pixels_exit;
    }

    for (index = 0u; index < pixelsLength; index++)
    {
        HISTOGRAM_WRITE_REGISTER(INPUT_DATA_REG_OFFSET, INPUT_DATA_WRITE_PIXEL(pixels[index]));

        HISTOGRAM_WRITE_REGISTER(INPUT_CONTROL_REG_OFFSET, PIXEL_VALID_ON);
        result = Histogram_WaitForOutStatusFlag(PIXEL_READ_MASK);
        HISTOGRAM_WRITE_REGISTER(INPUT_CONTROL_REG_OFFSET, PIXEL_VALID_OFF);

        if (result == false)
        {
            goto histogram_write_pixels_exit;
        }
    }

histogram_write_pixels_exit:

    return result;
}

bool Histogram_ReadHistogram(u16* histogram, size_t histogramLength)
{
    size_t index;
    bool result;

    result = false;

    if (histogramLength > HISTOGRAM_LENGTH)
    {
        goto histogram_read_histogram_exit;
    }

    for (index = 0u; index < histogramLength; index++)
    {
        HISTOGRAM_WRITE_REGISTER(INPUT_DATA_REG_OFFSET, INPUT_DATA_WRITE_ADDRESS(index));

        HISTOGRAM_WRITE_REGISTER(INPUT_CONTROL_REG_OFFSET, HIST_ADDRESS_VALID_ON);
        result = Histogram_WaitForOutStatusFlag(HIST_OUT_VALID_MASK);
        HISTOGRAM_WRITE_REGISTER(INPUT_CONTROL_REG_OFFSET, HIST_ADDRESS_VALID_OFF);

        if (result == false)
        {
            goto histogram_read_histogram_exit;
        }

        histogram[index] = HISTOGRAM_READ_REGISTER(OUT_RESULT_REG_OFFSET);
    }

histogram_read_histogram_exit:

    return result;
}

/***************************** Local functions definitions *************************/
static bool Histogram_WaitForOutStatusFlag(u32 statusRegMask)
{
    bool result;
    u8 timeout;

    result = true;
    timeout = 0u;
    while (((HISTOGRAM_READ_REGISTER(OUT_STATUS_REG_OFFSET) & statusRegMask) == 0u) && (timeout < HISTOGRAM_FLAG_TIMEOUT))
    {
        timeout++;
    }

    if (timeout >= HISTOGRAM_FLAG_TIMEOUT)
    {
        result = false;
    }

    return result;
}
