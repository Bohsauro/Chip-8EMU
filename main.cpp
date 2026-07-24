#include <iostream>
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <vector>
#include <array>
#include <fstream>
#include <cstdint>
#include <cstdlib>
#include <iomanip>


class Chip8 {
    private:


        // MEMORY
        std::array<uint8_t, 4096> memory{};


        // REGISTERS V0 - VF
        std::array<uint8_t, 16> V{};

        // INDEX REGISTER (16 bit)
        uint16_t I = 0;

        // PROGRAM COUNTER
         // i programmi partono da 0x200

        // STACK (16 livelli)
        std::array<uint16_t, 16> stack{};
        uint8_t sp = 0; // stack pointer



        std::array<uint8_t, 80> chip8_fontset =
        {
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
    public:

        uint16_t pc = 0x200;
        // TIMERS
        uint8_t delayTimer = 0;
        uint8_t soundTimer = 0;
        uint8_t screen[32][64] = {0};
        Chip8();

        void loadROM(const std::string& filename);
        void cycle();   // esegue fetch-decode-execute

        // PUSH (usato per CALL)
        void push(uint16_t value) {
            if (sp >= 16) {
                throw std::overflow_error("Stack overflow");
            }
            stack[sp] = value;
            sp++;
        }

        // POP (usato per RET)
        uint16_t pop() {
            if (sp == 0) {
                throw std::underflow_error("Stack underflow");
            }
            sp--;
            return stack[sp];
        }
};

Chip8::Chip8() {
    pc = 0x200;

    for (size_t i = 0; i < chip8_fontset.size(); i++)
    {
        memory[0x050 + i] = chip8_fontset[i]; // font
    }

}

void Chip8::loadROM(const std::string &filename) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);

    if (!file.is_open()) {
        std::cerr << "Errore: Impossibile aprire la ROM!" << std::endl;

    }

    // Calcoliamo la dimensione del file
    std::streamsize size = file.tellg();

    // Controllo che la ROM non sia più grande dello spazio disponibile (3584 byte)
    if (size > (4096 - 0x200)) {
        std::cerr << "Errore: ROM troppo grande per la memoria!" << std::endl;

    }

    // Torniamo all'inizio del file
    file.seekg(0, std::ios::beg);

    // Leggiamo i dati direttamente nella memoria a partire dall'offset 0x200
    if (file.read(reinterpret_cast<char*>(&memory[0x200]), size)) {
        std::cout << "ROM caricata con successo! Dimensione: " << size << " byte." << std::endl;

    }


}



int main()
{


    // Create the main window
    sf::RenderWindow window(sf::VideoMode({800, 600}), "SFML window");







    // Start the game loop
    while (window.isOpen())
    {
        // Process events
        while (const std::optional event = window.pollEvent())
        {
            // Close window: exit
            if (event->is<sf::Event::Closed>())
                window.close();
        }



        window.display();
    }
}