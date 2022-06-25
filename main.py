from typing import List
from xmlrpc.client import Boolean
import serial
import re
import sys
import array
import os
import matplotlib.pyplot as plt
from enum import IntEnum
import copy

file_path = ""


class HardwareConnector:
    MARKER = 0xAA

    def __init__(self, pixels) -> None:
        self.pixels = pixels
        self.histogram = [0] * 256
        self.connector = serial.Serial(
            port="COM5",
            baudrate=115200,
            parity=serial.PARITY_NONE,
            stopbits=serial.STOPBITS_ONE,
            bytesize=serial.EIGHTBITS,
            timeout=1,
        )

    def write_pixels(self) -> Boolean:
        self.connector.write(bytearray([self.MARKER]))
        value = self.connector.read_until().decode("utf-8")
        if "OK" not in value:
            return False
        value = str()

        self.connector.write(len(self.pixels).to_bytes(2, byteorder="big"))
        value = self.connector.read_until().decode("utf-8")
        if len(self.pixels) != int(value):
            return False
        value = str()

        counter = 0
        for pixel in self.pixels:
            # print(pixel)
            self.connector.write(bytearray([pixel]))
            value = self.connector.read_until().decode("utf-8")
            if "OK" in value:
                pass
            else:
                return False
            value = str()
            counter += 1

        is_ready = False
        while is_ready is False:
            value = self.connector.read_until().decode("utf-8")
            if "Done" in value:
                is_ready = True
            else:
                result = re.findall(r"\d+", value)
                if result:
                    self.histogram[int(result[0])] = int(result[1])
                else:
                    return False

        return True

    def get_histogram(self) -> List:
        return self.histogram


class BmpFile:
    def __init__(self, raw_data):
        self.raw_data = copy.deepcopy(raw_data)
        self.file_size = 0
        self.data_offset = 0
        self.data_width = 0
        self.data_height = 0
        self.bits_per_pixel = 0
        self.raw_data_size = 0
        self.pixels = []
        self.histogram = [0] * 256
        self.is_header_parsed = False

    def parse_header(self):
        if (
            self.raw_data[self.Offsets.ID] != 0x42
            and self.raw_data[self.Offsets.ID + 1] != 0x4D
        ):
            return False

        for i in range(self.fieldSize[self.Offsets.FILE_SIZE]):
            self.file_size += self.raw_data[self.Offsets.FILE_SIZE + i] << (8 * i)

        for i in range(self.fieldSize[self.Offsets.DATA_OFFSET]):
            self.data_offset += self.raw_data[self.Offsets.DATA_OFFSET + i] << (8 * i)

        for i in range(self.fieldSize[self.Offsets.DATA_WIDTH]):
            self.data_width += self.raw_data[self.Offsets.DATA_WIDTH + i] << (8 * i)

        for i in range(self.fieldSize[self.Offsets.DATA_HEIGHT]):
            self.data_height += self.raw_data[self.Offsets.DATA_HEIGHT + i] << (8 * i)

        for i in range(self.fieldSize[self.Offsets.BITS_PER_PIXEL]):
            self.bits_per_pixel += self.raw_data[self.Offsets.BITS_PER_PIXEL + i] << (
                8 * i
            )

        for i in range(self.fieldSize[self.Offsets.RAW_DATA_SIZE]):
            self.raw_data_size += self.raw_data[self.Offsets.RAW_DATA_SIZE + i] << (
                8 * i
            )

    def generate_histogram(self):
        bin_size = 1
        row_data_length = self.data_width * (int(self.bits_per_pixel / 8))
        row_padding_length = (4 - (row_data_length % 4)) % 4

        data_index = self.data_offset

        for _ in range(self.data_height):
            for __ in range(self.data_width):
                pixel_value = 0
                for i in range(int(self.bits_per_pixel / 8)):
                    pixel_value += self.raw_data[data_index] << (8 * i)
                    self.pixels.append(pixel_value)
                    data_index += 1
                for i in range(len(self.histogram)):
                    if (i * bin_size) <= pixel_value < ((i + 1) * bin_size):
                        self.histogram[i] += 1
            data_index += row_padding_length

    def plot_histograms(self, hardware_histogram):
        errors = 0

        for i in range(256):
            if self.histogram[i] != hardware_histogram[i]:
                errors += 1

        print("Number of wrongly calculated bins:", errors)

        plt.figure()
        plt.subplot(211)
        plt.bar(range(256), self.histogram)
        plt.title("Histogram referencyjny")
        plt.xlabel("Poziom szarości")
        plt.ylabel("Liczba pixeli")

        plt.subplot(212)
        plt.bar(range(256), hardware_histogram)
        plt.title("Histogram odczytany z FPGA")
        plt.xlabel("Poziom szarości")
        plt.ylabel("Liczba pixeli")
        plt.show()

    def get_pixels(self):
        return self.pixels

    class Offsets(IntEnum):
        ID = 0x00
        FILE_SIZE = 0x02
        DATA_OFFSET = 0x0A
        DATA_WIDTH = 0x12
        DATA_HEIGHT = 0x16
        BITS_PER_PIXEL = 0x1C
        RAW_DATA_SIZE = 0x22

    fieldSize = {
        Offsets.ID: 2,
        Offsets.FILE_SIZE: 4,
        Offsets.DATA_OFFSET: 4,
        Offsets.DATA_WIDTH: 4,
        Offsets.DATA_HEIGHT: 4,
        Offsets.BITS_PER_PIXEL: 2,
        Offsets.RAW_DATA_SIZE: 4,
    }


def main(file_path):
    raw_bmp = array.array("B")
    raw_bmp.fromfile(open(file_path, "rb"), os.path.getsize(file_path))

    bmp_file = BmpFile(raw_bmp)
    bmp_file.parse_header()
    bmp_file.generate_histogram()

    hardware_connector = HardwareConnector(bmp_file.get_pixels())
    hardware_connector.write_pixels()

    bmp_file.plot_histograms(hardware_connector.get_histogram())


if __name__ == "__main__":
    file_path = sys.argv[1]
    main(sys.argv[1])
