#include <iostream>
#include <SFML/Graphics.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <vector>
#include <array>
#include <fstream>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <string>
#include <stdexcept>


const unsigned char VALORE_PREMUTO = 'a';

int mode = 0;
// Tabella dei tasti aggiornata per SFML 3 (sf::Keyboard::Key::...)
const sf::Keyboard::Key keyMap[16] = {
    sf::Keyboard::Key::X, // 0x0
    sf::Keyboard::Key::Num1, // 0x1
    sf::Keyboard::Key::Num2, // 0x2
    sf::Keyboard::Key::Num3, // 0x3
    sf::Keyboard::Key::Q, // 0x4
    sf::Keyboard::Key::W, // 0x5
    sf::Keyboard::Key::E, // 0x6
    sf::Keyboard::Key::A, // 0x7
    sf::Keyboard::Key::S, // 0x8
    sf::Keyboard::Key::D, // 0x9
    sf::Keyboard::Key::Z, // 0xA
    sf::Keyboard::Key::C, // 0xB
    sf::Keyboard::Key::Num4, // 0xC
    sf::Keyboard::Key::R, // 0xD
    sf::Keyboard::Key::F, // 0xE
    sf::Keyboard::Key::V // 0xF
};

static const sf::Color palette[4] = {
    sf::Color(0, 0, 0),       // 0 = Nero (Sfondo)
    sf::Color(255, 255, 255), // 1 = BIANCO brillante
    sf::Color(255, 255, 255), // 2 = BIANCO brillante
    sf::Color(255, 255, 255)  // 3 = BIANCO brillante
};

// Firma corretta per C++ / SFML
void print(uint8_t plane1[64][128], uint8_t plane2[64][128], bool is_hi_res, float size, sf::RenderWindow &window) {


    // Sfondo nero predefinito (Colore 0)
    window.clear(palette[0]);

    // Limiti dinamici in base alla risoluzione corrente
    int active_width = is_hi_res ? 128 : 64;
    int active_height = is_hi_res ? 64 : 32;

    sf::RectangleShape pixel(sf::Vector2f(size, size));

    for (int y = 0; y < active_height; y++) {
        for (int x = 0; x < active_width; x++) {
            // Combina i due piani per ottenere l'indice di colore da 0 a 3:
            // (plane2 porta il bit più significativo, plane1 quello meno significativo)
            uint8_t color_index = (plane2[y][x] << 1) | plane1[y][x];

            // Disegna il pixel solo se NON è il colore dello sfondo (0)
            if (color_index > 0) {
                pixel.setPosition({x * size, y * size});
                pixel.setFillColor(palette[color_index]);
                window.draw(pixel);
            }
        }
    }

    window.display();
}

class Chip8 {
public:
    uint8_t mode = 0; // 0 = CHIP-8, 1 = SCHIP, 2 = XO-CHIP

    std::array<uint8_t, 65536> memory{};

    // REGISTERS V0 - VF
    std::array<uint8_t, 16> V{};

    // INDEX REGISTER (16 bit)
    uint16_t I = 0;

    // PROGRAM COUNTER & STACK
    uint16_t pc = 0x200;
    std::array<uint16_t, 16> stack{};
    uint8_t sp = 0;

    // TIMERS
    uint8_t delayTimer = 0;
    uint8_t soundTimer = 0;

    // DISPLAY & BITPLANES [Y][X] -> [64 altezza][128 larghezza]
    uint8_t plane1[64][128] = {0};
    uint8_t plane2[64][128] = {0};

    uint8_t bitplane_select = 1; // Bitmask: 1 = Plane 1, 2 = Plane 2, 3 = Entrambi
    bool is_hi_res = false;      // false = 64x32, true = 128x64

    unsigned char key[16]{};

    std::array<uint8_t, 80> chip8_fontset = {
        0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
        0x20, 0x60, 0x20, 0x20, 0x70, // 1
        0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
        0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
        0x90, 0x90, 0xF0, 0x10, 0x10, // 4
        0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
        0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
        0xF0, 0x10, 0x20, 0x40, 0x40, // 7
        0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
        0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
        0xF0, 0x90, 0xF0, 0x90, 0x90, // A
        0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
        0xF0, 0x80, 0x80, 0x80, 0xF0, // C
        0xE0, 0x90, 0x90, 0x90, 0xE0, // D
        0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
        0xF0, 0x80, 0xF0, 0x80, 0x80  // F
    };

    Chip8() {
        // Carica il fontset nei primi indirizzi di memoria (0x000 - 0x050)
        for (int i = 0; i < 80; ++i) {
            memory[i] = chip8_fontset[i];
        }
    }

    size_t get_active_size() const {
        return (mode == 2) ? 65536 : 4096;
    }

    // Ritorna 'true' se c'è stata una collisione (pixel da 1 passa a 0)
    bool xor_pixel(int x, int y) {
        // Protezione Bounds Check
        if (x < 0 || x >= 128 || y < 0 || y >= 64) return false;

        bool collision = false;

        // Plane 1
        if (bitplane_select & 0x1) {
            if (plane1[y][x] == 1) collision = true;
            plane1[y][x] ^= 1;
        }

        // Plane 2
        if (bitplane_select & 0x2) {
            if (plane2[y][x] == 1) collision = true;
            plane2[y][x] ^= 1;
        }

        return collision;
    }

    uint8_t get_pixel(int x, int y) const {
        if (x < 0 || x >= 128 || y < 0 || y >= 64) return 0;
        return (plane2[y][x] << 1) | plane1[y][x];
    }

    void loadROM(const std::string &filename);
    void cycle();

    void push(uint16_t value) {
        if (sp >= 16) throw std::overflow_error("Stack overflow");
        stack[sp++] = value;
    }

    uint16_t pop() {
        if (sp == 0) throw std::underflow_error("Stack underflow");
        return stack[--sp];
    }
};



void Chip8::loadROM(const std::string &filename) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);

    if (!file.is_open()) {
        std::cerr << "Errore: Impossibile aprire la ROM!" << std::endl;
    }

    // Calcoliamo la dimensione del file
    std::streamsize size = file.tellg();


    // Torniamo all'inizio del file
    file.seekg(0, std::ios::beg);

    // Leggiamo i dati direttamente nella memoria a partire dall'offset 0x200
    if (file.read(reinterpret_cast<char *>(&memory[0x200]), size)) {
        std::cout << "ROM caricata con successo! Dimensione: " << size << " byte." << std::endl;
    }
}

void Chip8::cycle() {
    uint16_t opcode = 0;
    if (pc < 4095) {
        opcode = (memory[pc] << 8) | memory[pc + 1];
        pc += 2;
    } else {
        return;
    }

    uint8_t x = (opcode & 0x0F00) >> 8;
    uint8_t y = (opcode & 0x00F0) >> 4;

    uint8_t Vx = V[x];
    uint8_t Vy = V[y];

    //DECODE E EXECUTE
    switch (opcode & 0xF000) {
        case 0x0000: {
    // Gestione dei casi esatti a 16 bit
    if (opcode == 0x00E0) {
        // CLS - Pulisce i piani correntemente selezionati
        for (int yy = 0; yy < 64; yy++) {
            for (int xx = 0; xx < 128; xx++) {
                if (bitplane_select & 0x1) plane1[yy][xx] = 0;
                if (bitplane_select & 0x2) plane2[yy][xx] = 0;
            }
        }
    }
    else if (opcode == 0x00EE) {
        // RET - Ritorna da una subroutine
        pc = pop();
    }
    else if (opcode == 0x00FE) {
        // LOW-RES mode (64x32)
        is_hi_res = false;
        for (int yy = 0; yy < 64; yy++) {
            for (int xx = 0; xx < 128; xx++) {
                if (bitplane_select & 0x1) plane1[yy][xx] = 0;
                if (bitplane_select & 0x2) plane2[yy][xx] = 0;
            }
        }
    }
    else if (opcode == 0x00FF) {
        // HI-RES mode (128x64)
        is_hi_res = true;
        for (int yy = 0; yy < 64; yy++) {
            for (int xx = 0; xx < 128; xx++) {
                if (bitplane_select & 0x1) plane1[yy][xx] = 0;
                if (bitplane_select & 0x2) plane2[yy][xx] = 0;
            }
        }
    }
    else if (opcode == 0x00FD) {
        // EXIT - Ferma l'emulatore (S-CHIP)

    }
    else {
        // Gestione delle istruzioni parametriche (0x00CN, 0x00DN, 0x00FB, 0x00FC)
        int width = is_hi_res ? 128 : 64;
        int height = is_hi_res ? 64 : 32;

        if ((opcode & 0x00F0) == 0x00C0) {
            // 00CN - Scroll Down di N pixel
            int n = opcode & 0x000F;
            for (int yy = height - 1; yy >= 0; yy--) {
                for (int xx = 0; xx < width; xx++) {
                    if (bitplane_select & 0x1) {
                        plane1[yy][xx] = (yy >= n) ? plane1[yy - n][xx] : 0;
                    }
                    if (bitplane_select & 0x2) {
                        plane2[yy][xx] = (yy >= n) ? plane2[yy - n][xx] : 0;
                    }
                }
            }
        }
        else if ((opcode & 0x00F0) == 0x00D0) {
            // 00DN - Scroll Up di N pixel (XO-CHIP)
            int n = opcode & 0x000F;
            for (int yy = 0; yy < height; yy++) {
                for (int xx = 0; xx < width; xx++) {
                    if (bitplane_select & 0x1) {
                        plane1[yy][xx] = (yy + n < height) ? plane1[yy + n][xx] : 0;
                    }
                    if (bitplane_select & 0x2) {
                        plane2[yy][xx] = (yy + n < height) ? plane2[yy + n][xx] : 0;
                    }
                }
            }
        }
        else if (opcode == 0x00FB) {
            // 00FB - Scroll Right di 4 pixel
            for (int yy = 0; yy < height; yy++) {
                for (int xx = width - 1; xx >= 0; xx--) {
                    if (bitplane_select & 0x1) {
                        plane1[yy][xx] = (xx >= 4) ? plane1[yy][xx - 4] : 0;
                    }
                    if (bitplane_select & 0x2) {
                        plane2[yy][xx] = (xx >= 4) ? plane2[yy][xx - 4] : 0;
                    }
                }
            }
        }
        else if (opcode == 0x00FC) {
            // 00FC - Scroll Left di 4 pixel
            for (int yy = 0; yy < height; yy++) {
                for (int xx = 0; xx < width; xx++) {
                    if (bitplane_select & 0x1) {
                        plane1[yy][xx] = (xx + 4 < width) ? plane1[yy][xx + 4] : 0;
                    }
                    if (bitplane_select & 0x2) {
                        plane2[yy][xx] = (xx + 4 < width) ? plane2[yy][xx + 4] : 0;
                    }
                }
            }
        }
    }
    break;
}

        case 0x1000:
            pc = opcode & 0x0FFF;
            break;

        case 0x2000:
            push(pc);
            pc = opcode & 0x0FFF;
            break;

        case 0x3000:
            if (Vx == (opcode & 0x00FF)) {
                pc += 2;
            }
            break;

        case 0x4000:
            if (Vx != (opcode & 0x00FF)) {
                pc += 2;
            }
            break;

        case 0x5000:
            if (Vx == Vy) {
                pc += 2;
            }
            break;

        case 0x9000:
            if (Vx != Vy) {
                pc += 2;
            }
            break;

        case 0x6000:
            V[x] = opcode & 0x00FF;
            break;

        case 0x7000:
            V[x] += opcode & 0x00FF;
            break;

        case 0x8000: {
            switch (opcode & 0x000F) {
                case 0x0000:
                    V[x] = V[y];
                    break;
                case 0x0001:
                    V[x] |= V[y];
                    break;
                case 0x0002:
                    V[x] &= V[y];
                    break;
                case 0x0003:
                    V[x] ^= V[y];
                    break;
                case 0x0004: {
                    uint16_t sum = V[x] + V[y];
                    V[x] = sum & 0xFF;
                    V[0xF] = (sum > 255) ? 1 : 0;
                    break;
                }
                case 0x0005: {
                    // 8XY5
                    uint8_t res = (V[x] >= V[y]) ? 1 : 0;
                    V[x] -= V[y];
                    V[0xF] = res;
                    break;
                }
                case 0x0007: {
                    // 8XY7
                    uint8_t res = (V[y] >= V[x]) ? 1 : 0;
                    V[x] = V[y] - V[x];
                    V[0xF] = res;
                    break;
                }
                case 0x0006: // 8XY6
                    if (mode == 0) {
                        V[x] = V[y];
                        V[0xF] = V[x] & 0x1;
                        V[x] >>= 1;
                    } else {
                        V[0xF] = V[x] & 0x1;
                        V[x] >>= 1;
                    }
                    break;
                case 0x000E: // 8XYE
                    if (mode == 0) {
                        V[x] = V[y];
                        V[0xF] = (V[x] & 0x80) >> 7;
                        V[x] <<= 1;
                    } else {
                        V[0xF] = (V[x] & 0x80) >> 7;
                        V[x] <<= 1;
                    }
                    break;
            } // chiude switch (opcode & 0x000F)
            break; // chiude case 0x8000
        }

        case 0xA000:
            I = opcode & 0x0FFF;
            break;

        case 0xB000: // BNNN
            if (mode == 0) {
                pc = (opcode & 0x0FFF) + V[0];
            } else {
                pc = ((opcode & 0x0FF0) >> 1) + V[opcode & 0x000F];
            }
            break;

        case 0xC000:
            V[x] = (rand() % 256) & (opcode & 0x00FF);
            break;

        case 0xD000: {
            int screen_width  = is_hi_res ? 128 : 64;
            int screen_height = is_hi_res ? 64 : 32;

            int x_start = V[x] % screen_width;
            int y_start = V[y] % screen_height;
            int height  = opcode & 0x000F;

            V[0xF] = 0; // Reset flag collisione

            int rows = height;
            int bytes_per_row = 1;

            // Gestione sprite 16x16 in SCHIP / XO-CHIP
            if ((mode == 1 || mode == 2) && height == 0) {
                rows = 16;
                bytes_per_row = 2;
            }

            uint32_t max_ram = (mode == 2) ? 65536 : 4096;

            for (int row = 0; row < rows; row++) {
                int current_y;

                if (mode == 0) {
                    current_y = (y_start + row) % screen_height;
                } else {
                    current_y = y_start + row;
                    if (current_y >= screen_height) break;
                }

                for (int byte_idx = 0; byte_idx < bytes_per_row; byte_idx++) {
                    uint32_t sprite_addr = I + (row * bytes_per_row) + byte_idx;
                    if (sprite_addr >= max_ram) break;

                    uint8_t spriteByte = memory[sprite_addr];

                    for (int col = 0; col < 8; col++) {
                        if ((spriteByte & (0x80 >> col)) != 0) {
                            int current_x;

                            if (mode == 0) {
                                current_x = (x_start + (byte_idx * 8) + col) % screen_width;
                            } else {
                                current_x = x_start + (byte_idx * 8) + col;
                            }

                            if (current_x < screen_width) {
                                if (xor_pixel(current_x, current_y)) {
                                    V[0xF] = 1;
                                }
                            }
                        }
                    }
                }
            }
            break;
        }

        case 0xE000: {
            uint8_t keyIndex = V[x] & 0x0F;
            switch (opcode & 0x00FF) {
                case 0x009E: // EX9E
                    if (key[keyIndex]) {
                        pc += 2;
                    }
                    break;
                case 0x00A1: // EXA1
                    if (!key[keyIndex]) {
                        pc += 2;
                    }
                    break;
            }
            break;
        }

        case 0xF000: {
            switch (opcode & 0x000F) {
                case 0x0007:
                    V[x] = delayTimer;
                    break;

                case 0x0005:
                    switch (opcode & 0x00F0) {
                        case 0x0010:
                            delayTimer = V[x];
                            break;
                        case 0x0050: {
                            // FX55
                            int max = x;
                            for (int i = 0; i <= max; i++) {
                                memory[I + i] = V[i];
                            }
                            if (mode == 0) {
                                I += x + 1;
                            }
                            break;
                        }
                        case 0x0060: {
                            // FX65
                            int massimo = x;
                            for (int i = 0; i <= massimo; i++) {
                                V[i] = memory[I + i];
                            }
                            if (mode == 0) {
                                I += x + 1;
                            }
                            break;
                        }
                    }
                    break;

                case 0x0008:
                    soundTimer = V[x];
                    break;

                case 0x000E:
                    if (I + V[x] > 0x0FFF) {
                        V[15] = 1;
                    }
                    I += V[x];
                    break;

                case 0x000A: {
                    // FX0A: Wait for key press
                    bool key_pressed = false;
                    for (int i = 0; i < 16; ++i) {
                        if (key[i]) {
                            V[x] = i;
                            key_pressed = true;
                            break;
                        }
                    }
                    if (!key_pressed) {
                        pc -= 2;
                    }
                    break;
                }

                case 0x0009:
                    I = 0x050 + (V[x] * 5);
                    break;

                case 0x0003: {
                    int n = V[x];
                    memory[I] = n / 100;
                    memory[I + 1] = (n / 10) % 10;
                    memory[I + 2] = n % 10;
                    break;
                }

                case 0x0000:
                    I = ((uint16_t) memory[pc] << 8) | memory[pc + 1];
                    pc += 2;
                    break;
            } // chiude switch (opcode & 0x000F)
            break; // chiude case 0xF000
        }
    } // chiude switch (opcode & 0xF000)
}

int main(int argc, char *argv[])
{
    // argv 0 = program, 1 mode, 2 rom

    // 0 for CHIP-8, 1 for SCHIP, 2 for XO-CHIP
    if (argc < 2) {
        std::cout << "No mode specified, using CHIP8 as default.\n";
        mode = 0; // Default: CHIP-8
    } else {
        mode = std::stoi(argv[1]);
        std::cout << "Mode: " << mode << "\n";
    }

    float width = 64;
    float height = 32;
    float size = 10;

    // Cambiato il nome dell'oggetto da 'Chip8' a 'chip8' per evitare lo shadowing di tipo
    Chip8 chip8;

    if (argc < 3) {
        std::cout << "No rom specified, using CHIP8-logo as default.\n";
        chip8.loadROM("roms/1-chip8-logo.ch8");
    } else {
        chip8.loadROM(argv[2]);
        std::cout << "ROM: " << argv[2] << "\n";
    }

    sf::RenderWindow window(
        sf::VideoMode({static_cast<unsigned>(width * size), static_cast<unsigned>(height * size)}),
        "Chip-8-EMULATOR",
        sf::Style::Titlebar | sf::Style::Close
    );
    window.setFramerateLimit(60);

    while (window.isOpen()) {
        while (const auto event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>())
                window.close();
        }

        std::memset(chip8.key, 0, sizeof(chip8.key));

        for (int i = 0; i < 16; ++i) {
            if (sf::Keyboard::isKeyPressed(keyMap[i])) {
                std::cout << std::hex << i << std::endl;
                chip8.key[i] = VALORE_PREMUTO;
            }
        }

        for (int i = 0; i < 11; i++) {
            chip8.cycle();
        }

        int pixel_count = 0;
        for(int y=0; y<32; y++) {
            for(int x=0; x<64; x++) {
                if(chip8.plane1[y][x]) pixel_count++;
            }
        }


        float pixel_size = chip8.is_hi_res ? 5.0f : 10.0f;
        print(chip8.plane1, chip8.plane2, chip8.is_hi_res, pixel_size, window);

        // Update timer
        if (chip8.delayTimer > 0) {
            chip8.delayTimer--;
        }
        if (chip8.soundTimer > 0) {
            chip8.soundTimer--;
        }
    }

    return 0;
}