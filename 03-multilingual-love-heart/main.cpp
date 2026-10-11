#define _XOPEN_SOURCE 700

#include <algorithm>
#include <array>
#include <chrono>
#include <clocale>
#include <cmath>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cerrno>
#include <fcntl.h>
#include <random>
#include <string>
#include <string_view>
#include <sys/ioctl.h>
#include <termios.h>
#include <thread>
#include <unistd.h>
#include <vector>
#include <wchar.h>

using Clock = std::chrono::steady_clock;
constexpr double PI = 3.14159265358979323846;
volatile std::sig_atomic_t stop_requested = 0;

void stop_on_signal(int) { stop_requested = 1; }

struct Terminal {
    termios previous{};
    int previous_flags = -1;
    bool active = false;

    Terminal() {
        if (!isatty(STDIN_FILENO) || !isatty(STDOUT_FILENO)) return;
        if (tcgetattr(STDIN_FILENO, &previous) != 0) return;
        termios raw = previous;
        raw.c_lflag &= static_cast<tcflag_t>(~(ICANON | ECHO));
        raw.c_iflag &= static_cast<tcflag_t>(~(IXON | ICRNL));
        raw.c_cc[VMIN] = 0;
        raw.c_cc[VTIME] = 0;
        if (tcsetattr(STDIN_FILENO, TCSANOW, &raw) != 0) return;
        previous_flags = fcntl(STDIN_FILENO, F_GETFL);
        if (previous_flags != -1)
            fcntl(STDIN_FILENO, F_SETFL, previous_flags | O_NONBLOCK);
        active = true;
        std::fputs("\033[?1049h\033[?25l\033[2J\033[H", stdout);
        std::fflush(stdout);
    }

    ~Terminal() {
        if (!active) return;
        std::fputs("\033[0m\033[?25h\033[?1049l", stdout);
        std::fflush(stdout);
        tcsetattr(STDIN_FILENO, TCSANOW, &previous);
        if (previous_flags != -1)
            fcntl(STDIN_FILENO, F_SETFL, previous_flags);
    }

    int key() const {
        unsigned char ch = 0;
        return read(STDIN_FILENO, &ch, 1) == 1 ? ch : -1;
    }
};

struct Rune {
    std::string bytes;
    int columns;
};

std::vector<Rune> decode(std::string_view input) {
    std::vector<Rune> out;
    std::mbstate_t state{};
    for (std::size_t i = 0; i < input.size();) {
        wchar_t cp = L'\0';
        std::size_t n = std::mbrtowc(&cp, input.data() + i, input.size() - i, &state);
        if (n == static_cast<std::size_t>(-1) ||
            n == static_cast<std::size_t>(-2)) {
            out.push_back({"?", 1});
            state = {};
            ++i;
            continue;
        }
        if (n == 0) { ++i; state = {}; continue; }
        const int columns = ::wcwidth(cp);
        if (columns == 0 && !out.empty()) {
            out.back().bytes.append(input.substr(i, n));
        } else if (columns > 0) {
            out.push_back({std::string(input.substr(i, n)), columns});
        } else {
            out.push_back({"?", 1});
        }
        i += n;
    }
    return out;
}

struct Phrase {
    std::vector<Rune> runes;
    int columns = 0;
};

std::vector<Phrase> make_phrases() {
    constexpr std::array<std::string_view, 20> words = {
        "Seni seviyorum", "I love you", "Je t'aime", "Te amo", "Ich liebe dich",
        "Ti amo", "Eu te amo", "Я тебя люблю", "愛してる", "사랑해",
        "我爱你", "Σ' αγαπώ", "Ik hou van jou", "Jag älskar dig", "Kocham cię",
        "Te iubesc", "Miluji tě", "Aku cinta kamu", "Jeg elsker deg", "Minä rakastan sinua"
    };
    std::vector<Phrase> result;
    for (auto text : words) {
        Phrase phrase;
        phrase.runes = decode(text);
        for (const auto& ch : phrase.runes) phrase.columns += ch.columns;
        result.push_back(std::move(phrase));
    }
    return result;
}

struct Color {
    int r = 0, g = 0, b = 0;
    bool operator==(const Color&) const = default;
};

struct Cell {
    std::string glyph = " ";
    Color fg{};
    bool continuation = false;
};

struct Canvas {
    int w, h;
    std::vector<Cell> cells;

    Canvas(int columns, int rows) : w(columns), h(rows), cells(w * h) {}

    void put(int x, int y, const std::string& glyph, int width, Color color) {
        if (y < 0 || y >= h || x < 0 || x + width > w || width < 1) return;
        Cell& c = cells[y * w + x];
        if (c.continuation) return;
        c.glyph = glyph;
        c.fg = color;
        if (width == 2) {
            Cell& following = cells[y * w + x + 1];
            following.glyph.clear();
            following.continuation = true;
        }
    }

    void text(int x, int y, std::string_view text, Color color) {
        for (const auto& ch : decode(text)) {
            put(x, y, ch.bytes, ch.columns, color);
            x += ch.columns;
        }
    }
};

Color glow(int x, int y, double t, int palette = 0, double strength = 1) {
    const double wave = 0.5 + 0.5 * std::sin(t * 3.2 - x * 0.16 + y * 0.33);
    const int a = static_cast<int>((125 + 130 * wave) * strength);
    auto sat = [](int n) { return std::clamp(n, 0, 255); };
    switch (palette % 4) {
        case 1: return {sat(a), sat(30 + a / 3), sat(75 + a / 3)};
        case 2: return {sat(80 + a / 2), sat(18 + a / 9), sat(a)};
        case 3: return {sat(a), sat(55 + a / 2), sat(8 + a / 15)};
        default: return {sat(a), sat(8 + a / 8), sat(22 + a / 4)};
    }
}

void frame_to_terminal(const Canvas& screen) {
    std::string output = "\033[H";
    output.reserve(screen.w * screen.h * 8);
    Color previous{};
    for (int y = 0; y < screen.h; ++y) {
        for (int x = 0; x < screen.w; ++x) {
            const Cell& cell = screen.cells[y * screen.w + x];
            if (cell.continuation) continue;
            if (!(cell.fg == previous)) {
                if (cell.fg == Color{}) {
                    output += "\033[39m";
                } else {
                    output += "\033[38;2;" + std::to_string(cell.fg.r) + ";" +
                        std::to_string(cell.fg.g) + ";" +
                        std::to_string(cell.fg.b) + "m";
                }
                previous = cell.fg;
            }
            output += cell.glyph;
        }
        output += "\033[39m\033[K\r\n";
        previous = {};
    }
    std::size_t sent = 0;
    while (sent < output.size()) {
        const ssize_t n = write(STDOUT_FILENO, output.data() + sent, output.size() - sent);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) break;
        sent += static_cast<std::size_t>(n);
    }
}

std::pair<int, int> terminal_size() {
    winsize size{};
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &size) != 0)
        return {80, 28};
    return {std::max(1, static_cast<int>(size.ws_col)),
            std::max(1, static_cast<int>(size.ws_row))};
}

struct Spark {
    double x = 0, y = 0, vx = 0, vy = 0, age = 0, life = 1;
};

struct Engine {
    std::mt19937 rng{std::random_device{}()};
    std::vector<Spark> sparks;
    const std::vector<Phrase> phrases = make_phrases();
    double time = 0;
    double speed = 1;
    bool paused = false;
    int mode = 0;
    int palette = 0;
    int old_w = 0, old_h = 0;

    double random(double lo, double hi) {
        return std::uniform_real_distribution<double>(lo, hi)(rng);
    }

    Spark spawn(int w, int h, int heart_w, int heart_h) {
        const double angle = random(0, 2 * PI);
        const double s = std::sin(angle);
        const double px = 16 * s * s * s / 17;
        const double py = (13 * std::cos(angle) - 5 * std::cos(2 * angle)
            - 2 * std::cos(3 * angle) - std::cos(4 * angle)) / 17;
        const double cx = w / 2.0;
        const double cy = (h - 1) / 2.0;
        Spark p;
        p.x = cx + px * heart_w * 0.44;
        p.y = cy - py * heart_h * 0.43;
        p.vx = random(-4, 4) + px * 1.5;
        p.vy = random(-3.8, 1.6);
        p.life = random(0.6, 2.7);
        return p;
    }

    void resize_if_needed(int w, int h, int heart_w, int heart_h) {
        if (w == old_w && h == old_h) return;
        old_w = w; old_h = h;
        sparks.clear();
        for (int i = 0; i < 125; ++i) {
            Spark p = spawn(w, h, heart_w, heart_h);
            p.age = random(0, p.life);
            p.x += p.vx * p.age;
            p.y += p.vy * p.age;
            sparks.push_back(p);
        }
        std::fputs("\033[2J", stdout);
        std::fflush(stdout);
    }

    void update_sparks(double dt, int w, int h, int heart_w, int heart_h) {
        for (auto& p : sparks) {
            p.age += dt;
            p.x += (p.vx + std::sin(time + p.y) * 1.4) * dt;
            p.y += p.vy * dt;
            if (p.age > p.life || p.x < 0 || p.x >= w || p.y < 0 || p.y >= h)
                p = spawn(w, h, heart_w, heart_h);
        }
    }

    static double heart_value(int x, int y, int w, int h, int heart_w, int heart_h, double beat) {
        const double xx = (x - w / 2.0) * 3.1 / heart_w / beat;
        const double yy = ((h - 1) / 2.0 - y) * 2.9 / heart_h / beat;
        const double a = xx * xx + yy * yy - 1.0;
        return a * a * a - xx * xx * yy * yy * yy;
    }

    void words(Canvas& c, int y, int left, int right) {
        int x = left;
        const int cycle = static_cast<int>(time * 0.65);
        while (x < right) {
            const bool first = x == left;
            const int spacer = first ? 0 : 1;
            const int start = (y * 7 + cycle * 3 + x / 9) % static_cast<int>(phrases.size());
            const Phrase* picked = nullptr;
            for (int tries = 0; tries < static_cast<int>(phrases.size()); ++tries) {
                const Phrase& p = phrases[(start + tries) % phrases.size()];
                if (p.columns + spacer <= right - x) {
                    picked = &p;
                    break;
                }
            }
            if (!picked) {
                c.put(x, y, ((x + y) % 4) ? "·" : "♥", 1,
                    glow(x, y, time, palette, 0.52));
                ++x;
                continue;
            }
            if (spacer) {
                c.put(x, y, " ", 1, {});
                ++x;
            }
            for (const Rune& r : picked->runes) {
                c.put(x, y, r.bytes, r.columns, glow(x, y, time, palette));
                x += r.columns;
            }
        }
    }

    void render(int columns, int rows) {
        const int w = std::min(columns - 1, 180);
        const int h = std::min(rows - 1, 55);
        if (w <= 0 || h <= 0) return;
        Canvas c(w, h);
        if (w < 60 || h < 20) {
            c.text(0, 1, "Terminali buyut: en az 61 sutun x 21 satir.", {255, 90, 120});
            frame_to_terminal(c);
            return;
        }

        const int heart_h = std::min(h - 7, 39);
        const int heart_w = std::min(w - 4, static_cast<int>(heart_h * 2.55));
        resize_if_needed(w, h, heart_w, heart_h);
        const double pulse = std::pow(std::max(0.0, std::sin(time * 2.0 * PI * 1.1)), 12);
        const double beat = 1.0 + 0.08 * pulse;

        const std::string headline = "LOVE HEART // TERMINAL CINEMATIC";
        c.text(std::max(0, (w - static_cast<int>(headline.size())) / 2), 0,
               headline, {255, 80, 110});

        for (const auto& p : sparks) {
            const int x = static_cast<int>(std::lround(p.x));
            const int y = static_cast<int>(std::lround(p.y));
            const double fade = std::max(0.0, 1.0 - p.age / p.life);
            const std::string mark = fade > 0.78 ? "✦" : (fade > 0.35 ? "*" : "·");
            c.put(x, y, mark, 1, glow(x, y, time, palette, 0.75 * fade));
        }

        for (int y = 2; y < h - 3; ++y) {
            int x = 0;
            while (x < w) {
                const double f = heart_value(x, y, w, h, heart_w, heart_h, beat);
                if (f > 0) {
                    if (f < 0.075 && (x + y) % 3 == 0)
                        c.put(x, y, "·", 1, glow(x, y, time, palette, 0.23));
                    ++x;
                    continue;
                }
                const int left = x;
                while (x < w && heart_value(x, y, w, h, heart_w, heart_h, beat) <= 0) ++x;
                if (mode == 1) {
                    for (int px = left; px < x; ++px) {
                        const int noise = (px * 41 + y * 29 + static_cast<int>(time * 15)) % 9;
                        const std::string mark = noise == 0 ? "✦" : (noise < 4 ? "♥" : "·");
                        c.put(px, y, mark, 1, glow(px, y, time, palette, noise == 0 ? 1.0 : 0.72));
                    }
                } else {
                    words(c, y, left, x);
                }
            }
        }

        const std::string subtitle = "20 LANGUAGES  •  ONE HEART";
        c.text(std::max(0, (w - 28) / 2), h - 3, subtitle, {224, 75, 110});
        const std::string help = "Q exit | SPACE pause | +/- tempo | M mode | C color | R restart";
        c.text(std::max(0, (w - static_cast<int>(help.size())) / 2), h - 1,
               help, paused ? Color{245, 190, 80} : Color{115, 85, 100});
        frame_to_terminal(c);
    }

    void on_key(int key) {
        switch (key) {
            case ' ': paused = !paused; break;
            case '+': case '=': speed = std::min(3.0, speed + 0.2); break;
            case '-': case '_': speed = std::max(0.2, speed - 0.2); break;
            case 'm': case 'M': mode = (mode + 1) % 2; break;
            case 'c': case 'C': palette = (palette + 1) % 4; break;
            case 'r': case 'R': time = 0; speed = 1; break;
            case 'q': case 'Q': stop_requested = 1; break;
            default: break;
        }
    }
};

int main() {
    if (!std::setlocale(LC_ALL, "")) std::setlocale(LC_ALL, "C.UTF-8");
    std::signal(SIGINT, stop_on_signal);
    std::signal(SIGTERM, stop_on_signal);
    Terminal terminal;
    if (!terminal.active) {
        std::fputs("Bu uygulamayi dogrudan bir terminalde calistir.\n", stderr);
        return 1;
    }

    Engine engine;
    auto before = Clock::now();
    auto next = before;
    while (!stop_requested) {
        auto now = Clock::now();
        const double dt = std::clamp(std::chrono::duration<double>(now - before).count(), 0.0, 0.1);
        before = now;
        int key = -1;
        while ((key = terminal.key()) != -1) engine.on_key(key);
        auto [cols, rows] = terminal_size();
        if (!engine.paused && cols >= 61 && rows >= 21) {
            engine.time += dt * engine.speed;
            const int w = std::min(cols - 1, 180);
            const int h = std::min(rows - 1, 55);
            const int hh = std::min(h - 7, 39);
            const int hw = std::min(w - 4, static_cast<int>(hh * 2.55));
            engine.resize_if_needed(w, h, hw, hh);
            engine.update_sparks(dt * engine.speed, w, h, hw, hh);
        }
        engine.render(cols, rows);
        next += std::chrono::milliseconds(33);
        std::this_thread::sleep_until(next);
        if (Clock::now() - next > std::chrono::milliseconds(250)) next = Clock::now();
    }
    return 0;
}
