#include "bmp_image.h"
#include "pack_defines.h"

#include <fstream>

using namespace std;

namespace img_lib {

PACKED_STRUCT_BEGIN BitmapFileHeader {
    uint16_t bf_type;      // Тип файла, должен быть 'BM'
    uint32_t bf_size;      // Размер файла в байтах
    uint16_t bf_reserved1; // Зарезервировано, должно быть 0
    uint16_t bf_reserved2; // Зарезервировано, должно быть 0
    uint32_t bf_off_bits;   // Смещение до начала данных изображения, обычно 54 байта
}
PACKED_STRUCT_END

PACKED_STRUCT_BEGIN BitmapInfoHeader {
    uint32_t bi_size;          // Размер этого заголовка, 4 байта
    int32_t bi_width;          // Ширина изображения в пикселях, 4 байта
    int32_t bi_height;         // Высота изображения в пикселях, 4 байта
    uint16_t bi_planes;        // Число цветовых плоскостей, должно быть 1
    uint16_t bi_bit_count;      // Число бит на пиксель, в нашем случае 24
    uint32_t bi_сompression;   // Тип компрессии, в нашешем случае 0 (без сжатия)
    uint32_t bi_size_image;     // Количество байт в данных — 4 байта, беззнаковое целое. Произведение отступа на высоту
    int32_t bi_x_pxls_per_meter;  // Горизонтальное разрешение, пикселей на метр, 4 байта (должно быть 11811)
    int32_t bi_y_pxls_per_meter;  // Вертикальное разрешение, пикселей на метр, 4 байта (должно быть 11811)
    uint32_t bi_clr_used;       // Число используемых цветов, 4 байта, должно быть 0
    uint32_t bi_clr_important;  // Число значимых цветов, 4 байта, должно быть 0x1000000
}
PACKED_STRUCT_END

// функция вычисления отступа по ширине
static int GetBMPStride(int w) {
    return 4 * ((w * 3 + 3) / 4);
}

// напишите эту функцию
bool SaveBMP(const Path& file, const Image& image) {
    ofstream ofs(file, ios::binary);
    if (!ofs) {
        return false;
    }

    int width = image.GetWidth();
    int height = image.GetHeight();
    int stride = GetBMPStride(width);

    BitmapFileHeader file_header;
    file_header.bf_type = 0x4D42; // 'BM'
    file_header.bf_size = sizeof(BitmapFileHeader) + sizeof(BitmapInfoHeader) + stride * height;
    file_header.bf_reserved1 = 0;
    file_header.bf_reserved2 = 0;
    file_header.bf_off_bits = sizeof(BitmapFileHeader) + sizeof(BitmapInfoHeader);

    BitmapInfoHeader info_header;
    info_header.bi_size = sizeof(BitmapInfoHeader);
    info_header.bi_width = width;
    info_header.bi_height = height;
    info_header.bi_planes = 1;
    info_header.bi_bit_count = 24;
    info_header.bi_сompression = 0;
    info_header.bi_size_image = stride * height;
    info_header.bi_x_pxls_per_meter = 11811;
    info_header.bi_y_pxls_per_meter = 11811;
    info_header.bi_clr_used = 0;
    info_header.bi_clr_important = 0x1000000;

    ofs.write(reinterpret_cast<char*>(&file_header), sizeof(file_header));
    ofs.write(reinterpret_cast<char*>(&info_header), sizeof(info_header));


    std::vector<char> buff(stride);

    for (int y = height - 1; y >= 0; --y) {
        const Color* line = image.GetLine(y);
        for (int x = 0; x < width; ++x) {
            buff[x * 3 + 0] = static_cast<char>(line[x].b);
            buff[x * 3 + 1] = static_cast<char>(line[x].g);
            buff[x * 3 + 2] = static_cast<char>(line[x].r);
        }
        ofs.write(buff.data(), stride);
    }

    return true;
}

Image LoadBMP(const Path& file) {
    // открываем поток с флагом ios::binary
    // поскольку будем читать данные в двоичном формате
    ifstream ifs(file, ios::binary);
    BitmapFileHeader file_header;
    BitmapInfoHeader info_header;

    // читаем заголовки
    ifs.read(reinterpret_cast<char*>(&file_header), sizeof(file_header));
    ifs.read(reinterpret_cast<char*>(&info_header), sizeof(info_header));

    if (file_header.bf_type != 0x4D42 /* 'BM' */ || info_header.bi_bit_count != 24 || info_header.bi_сompression != 0) {
        return {};
    }

    int width = info_header.bi_width;
    int height = info_header.bi_height;
    int stride = GetBMPStride(width);
    std::vector<char> buff(stride);

    Image result(width, height, Color::Black());
    for (int y = height - 1; y >= 0; --y) {
        Color* line = result.GetLine(y);
        ifs.read(buff.data(), stride);

        for (int x = 0; x < width; ++x) {
            line[x].b = static_cast<byte>(buff[x * 3 + 0]);
            line[x].g = static_cast<byte>(buff[x * 3 + 1]);
            line[x].r = static_cast<byte>(buff[x * 3 + 2]);
        }
    }

    return result;
}

}  // namespace img_lib