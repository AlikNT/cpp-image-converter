#include "ppm_image.h"

#include <array>
#include <fstream>
#include <iostream>
#include <stdio.h>
#include <setjmp.h>

#include <jpeglib.h>

using namespace std;

namespace img_lib {

// структура из примера LibJPEG
struct my_error_mgr {
    struct jpeg_error_mgr pub;
    jmp_buf setjmp_buffer;
};

typedef struct my_error_mgr* my_error_ptr;

// функция из примера LibJPEG
METHODDEF(void)
my_error_exit (j_common_ptr cinfo) {
    my_error_ptr myerr = (my_error_ptr) cinfo->err;
    (*cinfo->err->output_message) (cinfo);
    longjmp(myerr->setjmp_buffer, 1);
}

// тип JSAMPLE фактически псевдоним для unsigned char
void SaveScanlineToImage(const JSAMPLE* row, int y, Image& out_image) {
    Color* line = out_image.GetLine(y);
    for (int x = 0; x < out_image.GetWidth(); ++x) {
        const JSAMPLE* pixel = row + x * 3;
        line[x] = Color{byte{pixel[0]}, byte{pixel[1]}, byte{pixel[2]}, byte{255}};
    }
}

bool SaveJPEG(const Path& file, const Image& image) {
    jpeg_compress_struct cinfo;
    jpeg_error_mgr jerr;

    FILE * outfile;		/* целевой файл */
    JSAMPROW row_pointer[1];	/* указатель на JSAMPLE row[s] */
    int row_stride;		/* физическая ширина строки в буфере изображения */

    /* Шаг 1: выделение и инициализация объекта сжатия JPEG */

    /* Сначала нам нужно настроить обработчик ошибок на случай, если шаг инициализации
    * не удастся. (Маловероятно, но это может произойти, если у вас не хватит памяти.)
    * Эта процедура заполняет содержимое структуры jerr и возвращает
    * адрес jerr, который мы помещаем в поле ссылки в cinfo.
    */

    cinfo.err = jpeg_std_error(&jerr);
    /* Теперь мы можем инициализировать объект сжатия JPEG. */
    jpeg_create_compress(&cinfo);

    /* Шаг 2: указать место назначения данных (например, файл) */
    /* Примечание: шаги 2 и 3 можно выполнять в любом порядке. */

    /* Здесь мы используем предоставленный библиотекой код для отправки сжатых данных в
    * поток stdio. Вы также можете написать свой собственный код, чтобы сделать что-то еще.
    * ОЧЕНЬ ВАЖНО: используйте опцию "b" для fopen(), если вы работаете на машине, которая
    * требует ее для записи двоичных файлов.
    */
#ifdef _MSC_VER
    if ((outfile = _wfopen(file.string().c_str(), "wb")) == NULL) {
        cerr << "can't open %s\n"s << file.string().c_str();
#else
    if ((outfile = fopen(file.string().c_str(), "wb")) == NULL) {
        cerr << "can't open %s\n"s << file.string().c_str();
#endif
        return false;
    }
    jpeg_stdio_dest(&cinfo, outfile);

    /* Шаг 3: установка параметров для сжатия */

    /* Сначала мы предоставляем описание входного изображения.
    * Необходимо заполнить четыре поля структуры cinfo:
    */
    cinfo.image_width = image.GetWidth(); 	/* ширина и высота изображения в пикселях */
    cinfo.image_height = image.GetHeight();
    cinfo.input_components = 3;		/* Количество цветовых компонентов на пиксель */
    cinfo.in_color_space = JCS_RGB; /* цветовое пространство входного изображения */
    /* Теперь используйте процедуру библиотеки для установки параметров сжатия по умолчанию.
    * (Вы должны установить как минимум cinfo.in_color_space перед вызовом этого,
    * поскольку значения по умолчанию зависят от исходного цветового пространства.)
    */
    jpeg_set_defaults(&cinfo);
    /* Теперь вы можете задать любые параметры, отличные от параметров по умолчанию, которые вы хотите.
    * Здесь мы просто иллюстрируем использование масштабирования качества (таблицы квантования):
    */

    /* Шаг 4: Запустить компрессор */

    /* TRUE гарантирует, что мы запишем полный файл interchange-JPEG.
    * Передайте TRUE, если вы не совсем уверены в том, что делаете.
    */
    jpeg_start_compress(&cinfo, TRUE);

    /* Шаг 5: while (осталось записать строки сканирования) */
    /* jpeg_write_scanlines(...); */

    /* Здесь мы используем переменную состояния библиотеки cinfo.next_scanline как

    * счетчик цикла, чтобы нам не приходилось отслеживать самим.
    * Для простоты мы передаем одну строку сканирования за вызов; вы можете передать
    * больше, если хотите.
    */
    row_stride = image.GetWidth() * 3;	/* JSAMPLE на строку в image_buffer */

    while (cinfo.next_scanline < cinfo.image_height) {
        /* jpeg_write_scanlines ожидает массив указателей на строки развертки.
       * Здесь массив состоит только из одного элемента, но вы можете передать
       * более одной строки развертки за раз, если это удобнее.
       */
        std::vector<JSAMPLE> row(row_stride);
        const Color *line = image.GetLine(static_cast<int>(cinfo.next_scanline));
        for (int x = 0; x < image.GetWidth(); ++x) {
            row[x * 3] = static_cast<unsigned char>(line[x].r);
            row[x * 3 + 1] = static_cast<unsigned char>(line[x].g);
            row[x * 3 + 2] = static_cast<unsigned char>(line[x].b);
        }
        row_pointer[0] = row.data();
        jpeg_write_scanlines(&cinfo, row_pointer, 1);
    }

    /* Шаг 6: Завершение сжатия */

    jpeg_finish_compress(&cinfo);
    /* После finish_compress мы можем закрыть выходной файл. */
    fclose(outfile);

    /* Шаг 7: освобождение объекта сжатия JPEG */

    /* Это важный шаг, поскольку он освободит большой объем памяти. */
    jpeg_destroy_compress(&cinfo);

    /* И мы закончили! */
    return true;
}

Image LoadJPEG(const Path& file) {
    jpeg_decompress_struct cinfo;
    my_error_mgr jerr;
    
    FILE* infile;
    JSAMPARRAY buffer;
    int row_stride;

    // Тут не избежать функции открытия файла из языка C,
    // поэтому приходится использовать конвертацию пути к string.
    // Под Visual Studio это может быть опасно, и нужно применить
    // нестандартную функцию _wfopen
#ifdef _MSC_VER
    if ((infile = _wfopen(file.wstring().c_str(), "rb")) == NULL) {
#else
    if ((infile = fopen(file.string().c_str(), "rb")) == NULL) {
#endif
        return {};
    }

    /* Шаг 1: выделяем память и инициализируем объект декодирования JPEG */

    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = my_error_exit;

    if (setjmp(jerr.setjmp_buffer)) {
        jpeg_destroy_decompress(&cinfo);
        fclose(infile);
        return {};
    }

    jpeg_create_decompress(&cinfo);

    /* Шаг 2: устанавливаем источник данных */

    jpeg_stdio_src(&cinfo, infile);

    /* Шаг 3: читаем параметры изображения через jpeg_read_header() */

    (void) jpeg_read_header(&cinfo, TRUE);

    /* Шаг 4: устанавливаем параметры декодирования */

    // установим желаемый формат изображения
    cinfo.out_color_space = JCS_RGB;
    cinfo.output_components = 3;

    /* Шаг 5: начинаем декодирование */

    (void) jpeg_start_decompress(&cinfo);
    
    row_stride = cinfo.output_width * cinfo.output_components;
    
    buffer = (*cinfo.mem->alloc_sarray)
                ((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);

    /* Шаг 5a: выделим изображение ImgLib */
    Image result(cinfo.output_width, cinfo.output_height, Color::Black());

    /* Шаг 6: while (остаются строки изображения) */
    /*                     jpeg_read_scanlines(...); */

    while (cinfo.output_scanline < cinfo.output_height) {
        int y = cinfo.output_scanline;
        (void) jpeg_read_scanlines(&cinfo, buffer, 1);

        SaveScanlineToImage(buffer[0], y, result);
    }

    /* Шаг 7: Останавливаем декодирование */

    (void) jpeg_finish_decompress(&cinfo);

    /* Шаг 8: Освобождаем объект декодирования */

    jpeg_destroy_decompress(&cinfo);
    fclose(infile);

    return result;
}

} // of namespace img_lib