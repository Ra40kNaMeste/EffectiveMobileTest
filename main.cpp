
#include <cstring>
#include <exception>
#include <fstream>
#include <iostream>
#include <istream>
#include <map>
#include <ostream>
#include <string>
#include <vector>

// Парсинг аргуметов из командной строки
std::map<std::string, std::string> parseArgs(const int &argc, char *argv[]) {
  std::map<std::string, std::string> args;

  for (int i = 1; i < argc; ++i) {
    if (argv[i][0] == '-') {
      if (i + 1 < argc && argv[i + 1][0] != '-') {
        args.insert(std::make_pair(argv[i], argv[i + 1]));
        ++i;
      } else {
        args.insert(std::make_pair(argv[i], ""));
      }
    }
  }

  return args;
}

// Функция получения пути к исходному изображению
std::string get_path(const std::map<std::string, std::string> &params) {
  if (params.count("--file"))
    return params.at("--file");
  if (params.count("-f"))
    return params.at("-f");
  std::cout << "Enter input BMP file name:";
  std::string res;
  std::cin >> res;
  return res;
}

// Функция получения пути для сохранения
std::string get_out_path(const std::map<std::string, std::string> &params) {
  if (params.count("--out"))
    return params.at("--out");
  if (params.count("-o"))
    return params.at("-o");
  std::cout << "Enter output BMP file name:";
  std::string res;
  std::cin >> res;
  return res;
}

struct pixel {
  pixel() {}
  pixel(char r, char g, char b) {
    this->b = b;
    this->g = g;
    this->r = r;
  }
  unsigned char r, g, b; // red, green, blue values

  bool operator==(const pixel &other) {
    return r == other.r && g == other.g && b == other.b;
  }
};

// Шаблонная функция получения различных данных из строма
// Написал т.к. лень каждый раз это прописывать было
template <typename T> T read_from_stream(std::istream *stream) {
  T res;
  stream->read(reinterpret_cast<char *>(&res), sizeof(T));
  return res;
}

// Фукнция загрузки bmp изображения
// Разрабатывал по линукс, поэтому писал всё с нуля. Жрёт не все
// bmp-изображения, а только цветные 24-bit Colors изображения Также берутся
// хедеры читаемого изображения, чтобы не париться с сохранением. Размер и тип
// не меняется - оставляем старые Хедеры не забыть удалить!
std::vector<std::vector<pixel>>
read_bmp(const std::string &filename,
         std::vector<unsigned char> **out_headers) {
  std::ifstream input(filename);

  if (!input.is_open()) {
    throw std::exception();
  }

  // С какого байта начинается изображение
  unsigned int pixelOffset;
  input.seekg(0x000A, input.beg);
  pixelOffset = read_from_stream<unsigned int>(&input);

  // Сохранение старых заголовков
  input.seekg(0x0, input.beg);
  char *buff = new char[pixelOffset];
  input.read(buff, pixelOffset);

  *out_headers = new std::vector<unsigned char>(buff, buff + pixelOffset);
  delete[] buff;

  unsigned int width, height;
  unsigned short planes; // Просто для проверки на битость. Она должна = 1

  input.seekg(0x0012, input.beg);
  width = read_from_stream<unsigned int>(&input);

  input.seekg(0x0016, input.beg);
  height = read_from_stream<unsigned int>(&input);

  input.seekg(0x001A, input.beg);
  planes = read_from_stream<unsigned short>(&input);

  if (planes != 1) {
    throw std::exception();
  }

  // Чтение самого изображения
  std::vector<std::vector<pixel>> image(height, std::vector<pixel>(width));
  unsigned int padding = (4 - (width * 3) % 4) % 4;
  input.seekg(pixelOffset, std::ios::beg);

  for (unsigned int y = 0; y < height; ++y) {
    for (unsigned int x = 0; x < width; ++x) {
      unsigned char r, g, b;

      input.read(reinterpret_cast<char *>(&b), 1);
      input.read(reinterpret_cast<char *>(&g), 1);
      input.read(reinterpret_cast<char *>(&r), 1);

      image[y][x].r = r;
      image[y][x].g = g;
      image[y][x].b = b;
    }

    input.ignore(padding);
  }

  return image;
}

// Функция записи изображения. Старые хедеры передать
void write_image(std::vector<std::vector<pixel>> &image, std::string path,
                 const std::vector<unsigned char> *headers) {
  std::ofstream output(path);
  if (!output.is_open()) {
    throw std::exception();
  }
  for (auto value : *headers) {
    output << value;
  }

  unsigned int width, height;
  height = image.size();
  width = height == 0 ? 0 : image[0].size();

  unsigned int padding = (4 - (width * 3) % 4) % 4;

  char zero{0x00};
  for (unsigned int y = 0; y < height; ++y) {
    for (auto pxl : image[y]) {
      output.write(reinterpret_cast<char *>(&pxl.b), 1);
      output.write(reinterpret_cast<char *>(&pxl.g), 1);
      output.write(reinterpret_cast<char *>(&pxl.r), 1);
    }

    output.write(&zero, padding);
  }
}

// Рисование пикселя в консоли. Поддержитвается 2 типа. Если будет больше -
// некорректная работа
void print_pixel(const pixel &pxl) {
  static pixel white_pxl(255, 255, 255);
  static pixel black_pxl(0, 0, 0);
  if (pxl == white_pxl) {
    printf("\x1B[47m \033[0m");
  } else if (pxl == black_pxl) {
    printf("\x1B[40m \033[0m");
  }
}

// Отображение изобраения
void show_image(const std::vector<std::vector<pixel>> &image) {
  int rows = image.size();
  for (int row = rows - 1; row >= 0; --row) {
    for (auto pixel : image[row]) {
      print_pixel(pixel);
    }
    std::cout << std::endl;
  }
}

// Функция рисования креста на всю картинку
void print_X(std::vector<std::vector<pixel>> &image, pixel color) {
  int rows = image.size();
  if (rows == 0)
    return;
  int cols = image[0].size();

  if (rows > cols) {
    double a = cols / double(rows);
    for (int r = 0; r < rows; ++r) {
      image[r][int(a * r)] = color;
      image[r][cols - int(a * r) - 1] = color;
    }
  } else {
    double a = rows / double(cols);
    for (int c = 0; c < cols; ++c) {
      image[int(a * c)][c] = color;
      image[rows - int(a * c) - 1][c] = color;
    }
  }
}

int main(int argc, char *argv[]) {
  std::map<std::string, std::string> args = parseArgs(argc, argv);
  // Обрабатываем help
  if (args.count("--help") || args.count("-h")) {
    std::cout << "This program draws a cross on a bmp image" << std::endl;
    std::cout << "Usage:" << std::endl;
    std::cout << "-f | --file\tpath to bmp-image";
    std::cout << "-o | --out\tpath for saving";
    return 0;
  }
  // Парсим параметры ком строки
  std::vector<std::vector<pixel>> image;
  std::string path = get_path(args);
  std::vector<unsigned char> *headers;
  try {
    // Загружаем изображение
    image = read_bmp(path, &headers);
  } catch (std::exception ex) {
    std::cout << "Error opening file. Check the path" << std::endl;
    return -1;
  }
  show_image(image);
  // Рисуем крест
  print_X(image, pixel{0, 0, 0});
  show_image(image);
  std::string out_path = get_out_path(args);
  try {
    // Сохраняем
    write_image(image, out_path, headers);
  } catch (std::exception ex) {
    std::cout << "Error writing file. Check the path" << std::endl;
  }
  delete headers;
  // Радуемся
  return 0;
}
