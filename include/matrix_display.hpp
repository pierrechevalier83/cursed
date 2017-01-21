#pragma once

#include <curses.h>

#include <boost/range/algorithm/for_each.hpp>
#include <boost/range/algorithm/transform.hpp>

#include <cstdlib>
#include <memory>
#include <vector>

namespace ncurses {

class ColorScheme {
   public:
    ColorScheme(const std::vector<int>& scheme_) : scheme(scheme_) {
        start_color();
        int index = 0;
        for (const auto& color : scheme) {
            init_pair(index++, COLOR_BLACK, color);
        }
    }
    std::vector<int> scheme;
};

class Color {
   public:
    Color(int n) {
        attron(COLOR_PAIR(n));
        attron(A_BOLD);
    }
    ~Color() {
        attroff(COLOR_PAIR(n));
        attroff(A_BOLD);
    }

   private:
    int n;
};

struct Cell {
    Cell(const auto& s, int c = 0) : content(s), color_code(c) {}
    std::wstring content;
    int color_code;
};

void n_chars(int n, auto c) {
    std::wstring s(n, c);
    addwstr(s.c_str());
}
void addwch(const auto character) { n_chars(1, character); }
void n_strings(int n, auto s) {
    while (n-- > 0) {
        addwstr(s.c_str());
    }
}

void end_line() { addch('\n'); }
auto positioned(const auto& content, auto width, int offset) {
    if (content.length() >= width) {
        return content;
    } else {
        auto left = std::wstring(offset, L' ');
        auto right = std::wstring(width - (content.length() + offset), L' ');
        auto line = left + content + right;
        return line;
    }
}
auto centered(const auto& content, auto width) {
    auto offset = (width - content.length()) / 2;
    return positioned(content, width, offset);
}
auto aligned_right(const auto& content, auto width) {
    auto offset = width - content.length();
    return positioned(content, width, offset);
}
auto aligned_left(const auto& content, auto width) {
    return positioned(content, width, 0);
}
enum class Aligned { right, left, center };

void printline(const auto& content, int width = 0,
               Aligned alignment = Aligned::left) {
    std::wstring s;
    switch (alignment) {
        case Aligned::right: {
            s = aligned_right(content, width);
        }
        case Aligned::left: {
            s = aligned_left(content, width);
        }
        case Aligned::center: {
            s = centered(content, width);
        }
    }
    addwstr(s.c_str());
}

struct Environment {
    Environment() {
        initscr();
        setlocale(LC_ALL, "");
        cbreak();
        noecho();
        keypad(stdscr, true);
    }
    ~Environment() { endwin(); }
};

struct BoxStyle {
    struct Corners {
        wchar_t top_left = L'┏';
        wchar_t top_right = L'┓';
        wchar_t bottom_left = L'┗';
        wchar_t bottom_right = L'┛';
    };
    struct Intersections {
        wchar_t top = L'┳';
        wchar_t bottom = L'┻';
        wchar_t left = L'┣';
        wchar_t right = L'┫';
        wchar_t center = L'╋';
    };
    struct Borders {
        wchar_t horizontal = L'━';
        wchar_t vertical = L'┃';
    };
    Corners corners;
    Intersections intersections;
    Borders borders;
};

struct MatrixStyle {
    MatrixStyle(int cell_width_, int cell_height_)
        : cell_width(cell_width_), cell_height(cell_height_) {}
    int cell_width;
    int cell_height;
    BoxStyle box_style;
};

class MatrixDisplay {
   public:
    MatrixDisplay(const MatrixStyle& style_) : style(style_) {}
    int width_in_chars(const std::vector<std::vector<Cell>>& data) {
        return (style.cell_width + 1) * n_rows(data);
    }
    void print(const std::vector<std::vector<Cell>>& data) {
        int n = n_rows(data);
        top_row(n);
        auto first = true;
        for (const auto& row : data) {
            if (first) {
                first = false;
            } else {
                middle_row(n);
            }
            values(row);
        }
        bottom_row(n);
    }

   private:
    int n_rows(const std::vector<std::vector<Cell>>& data) {
        assert(!data.empty());
        return data[0].size();
    }
    void value_row(std::vector<Cell> row, auto left, auto intersection,
                   auto right) {
        addwch(left);
        auto first = true;
        for (const auto& cell : row) {
            if (first) {
                first = false;
            } else {
                addwch(intersection);
            }
            Color scoped(cell.color_code);
            printline(cell.content, style.cell_width, Aligned::center);
        }
        addwch(right);
        end_line();
    }
    void sep_row(int n, auto left, auto plain, auto intersection, auto right) {
        std::vector<Cell> sep(n, Cell(std::wstring(style.cell_width, plain)));
        value_row(sep, left, intersection, right);
    }

    void top_row(int n) {
        const auto& box = style.box_style;
        sep_row(n, box.corners.top_left, box.borders.horizontal,
                box.intersections.top, box.corners.top_right);
    }
    void bottom_row(int n) {
        const auto& box = style.box_style;
        sep_row(n, box.corners.bottom_left, box.borders.horizontal,
                box.intersections.bottom, box.corners.bottom_right);
    }
    void middle_row(int n) {
        const auto& box = style.box_style;
        sep_row(n, box.intersections.left, box.borders.horizontal,
                box.intersections.center, box.intersections.right);
    }
    void values(const std::vector<Cell>& row) {
        const auto& box = style.box_style;
        auto padding_row = row;
        for (auto& cell : padding_row) {
            cell.content = std::wstring(style.cell_width, L' ');
        }
        const auto line_height = 1;
        const auto top_pad = (style.cell_height - line_height) / 2;
        for (auto i = 0; i < top_pad; ++i) {
            value_row(padding_row, box.borders.vertical, box.borders.vertical,
                      box.borders.vertical);
        }
        value_row(row, box.borders.vertical, box.borders.vertical,
                  box.borders.vertical);
        auto bottom_pad = style.cell_height - (top_pad + line_height);
        for (auto i = 0; i < bottom_pad; ++i) {
            value_row(padding_row, box.borders.vertical, box.borders.vertical,
                      box.borders.vertical);
        }
    }
    void sep_col() { addwch(style.box_style.borders.vertical); }

   private:
    const MatrixStyle style;
};
}  // namespace ncurses
