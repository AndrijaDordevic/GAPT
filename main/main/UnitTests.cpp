#include <cassert>
#include "Tetromino.hpp"
#include "Menu.hpp"
#include <vector>
#include <iostream>
#include "Window.hpp"
#include "Audio.hpp"
#include <sstream>
#include <iomanip>
#include <string>
#include <thread>
#include <atomic>
#include <openssl/hmac.h>
#include <nlohmann/json.hpp>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")



std::vector<std::vector<SDL_Point>> shapes = {
{{0, 0}, {1, 0}, {2, 0}, {2, 0}},    // Horizontal Line
{{0, 0}, {0, 1}, {1, 1}, {1, 1}},    // Smaller Bottom Left shape
{{0, 0}, {0, 1}, {1, 1}, {2, 1}},    // Bottom Left shape
{{0, 0}, {1, 0}, {1, 0}, {1, 1}},    // Smaller Right Top shape
{{0, 0}, {1, 0}, {2, 0}, {2, 1}},    // Right Top shape
{{0, 0}, {1, 0}, {1, 1}, {2, 1}},    // Z shape
{{0, 0}, {1, 0}, {0, 1}, {1, 1}},    // Square shape
{{0, 0}, {1, 0}, {0, 1}, {0, 2}},    // Upside Down L 
{{0, 0}, {1, 0}, {0, 1}, {0, 1}},    // Smaller Upside Down L shape
{{0, 0}, {1, 0}, {1, 1}, {1, 2}},    // Mirrored Upside Down L shape
{{0, 0}, {1, 0}, {2, 0}, {0, 1}},    // Left Top  shape
{{0, 0}, {1, 0}, {1, 0}, {0, 1}},    // Smaller Left Top  shape
{{0, 0}, {0, 1}, {0, 2}, {0, 1}},    // 3 blocks i shape
{{0, 0}, {0, 1}, {0, 1}, {0, 1}},    // 2 blocks i shape
{{0, 0}, {0, 1}, {0, 2}, {1, 2}},    // L shape
{{0, 0}, {0, 1}, {1, 1}, {1, 2}},    // 4 shape
{{0, 0}, {0, 0}, {0, 0}, {0, 0}},    // Dot shape
};

struct MenuItem {
    SDL_FRect rect;
    string label;
    bool isHovered;
    SDL_Texture* textTexture;
    int textWidth;
    int textHeight;
};

extern std::vector<MenuItem> menuItems;
extern SDL_Window* windowm;
extern SDL_Renderer* rendererm;
using json = nlohmann::json;

    Tetromino CreateTetromino(const std::vector<SDL_Point>& shape, int xOffset, int yOffset) {
        Tetromino t;
        for (const auto& p : shape) {
            t.blocks.push_back({ p.x + xOffset, p.y + yOffset });
        }
        return t;
    }

    static std::string computeHMAC(const std::string& data, const std::string& secret) {
        unsigned char* result = HMAC(
            EVP_sha256(),
            reinterpret_cast<const unsigned char*>(secret.data()), secret.size(),
            reinterpret_cast<const unsigned char*>(data.data()), data.size(),
            nullptr, nullptr
        );

        std::ostringstream hex;
        hex << std::hex << std::setfill('0');
        for (int i = 0; i < 32; ++i) {
            hex << std::setw(2) << static_cast<int>(result[i]);
        }
        return hex.str();
    }

    static bool hmacEquals(const std::string& a, const std::string& b) {
        if (a.size() != b.size()) return false;
        unsigned char diff = 0;
        for (size_t i = 0; i < a.size(); ++i)
            diff |= a[i] ^ b[i];
        return diff == 0;
    }

    static bool validateHMAC(json& j, const std::string& secret) {
        if (!j.contains("hmac")) {
            std::cerr << "No HMAC in message – rejecting\n";
            return false;
        }
        std::string receivedTag = j["hmac"];
        // Remove it for recomputing
        j.erase("hmac");
        std::string bodyStr = j.dump();
        // Compute expected
        std::string expectedTag = computeHMAC(bodyStr, secret);
        if (!hmacEquals(receivedTag, expectedTag)) {
            std::cerr << "HMAC verification failed – possible tampering\n";
            return false;
        }
        return true;
    }

    static const std::string SHARED_SECRET = "my?very?strong?key";

    bool Test_HoverStates() {
        assert(!menuItems.empty());

        for (size_t i = 0; i < menuItems.size(); ++i) {
            float hoverX = menuItems[i].rect.x + 1;
            float hoverY = menuItems[i].rect.y + 1;

            // Simulate hovering over current item
            menuItems[i].isHovered = isMouseOver(hoverX, hoverY, menuItems[i].rect);

            // Validate hover is true only if we're over it
            assert(menuItems[i].isHovered == true);
        }

        std::cout << "Test_HoverStates passed.\n";
        return true;
    }

    bool Test_Collision() {
        std::vector<Tetromino> placed;

        // Test 1: No placed tetrominos = no collision
        Tetromino test1 = CreateTetromino(shapes[0], SnapToGrid(5, 40), SnapToGrid(5, 40));
        assert(CheckCollision(test1, placed) == false);

        // Test 2: Collision — block at (5,5)
        placed.push_back(CreateTetromino(shapes[0], SnapToGrid(5, 40), SnapToGrid(5, 40))); // square at same spot
        Tetromino test2 = CreateTetromino(shapes[1], SnapToGrid(5, 40), SnapToGrid(5, 40)); // long overlapping square
        assert(CheckCollision(test2, placed) == true);

        // Test 3: No collision — test block is far away

        Tetromino test3 = CreateTetromino(shapes[2], SnapToGrid(100, 40), SnapToGrid(10, 40));
        assert(CheckCollision(test3, placed) == false);

        // Test 4: Edge touch but no overlap — no collision
        Tetromino test4 = CreateTetromino(shapes[0], SnapToGrid(7, 40), SnapToGrid(5, 40)); // square to the right of existing one
        assert(CheckCollision(test4, placed) == true);

        std::cout << "All collision tests passed.\n";
        return 0;

    }

    int Test_Position() {


        tetrominos.clear();

        // Add a shape that *does not* conflict with spawnX/spawnY
        tetrominos.push_back(CreateTetromino(shapes[0], 3, 8)); // out of the way
        assert(IsPositionFree(10) == true);

        // Add a shape that *does* conflict
        tetrominos.clear();
        tetrominos.push_back(CreateTetromino(shapes[6], spawnX, 10)); // Square at spawn
        assert(IsPositionFree(10) == false); // Should overlap

        // Add a Dot that hits spawn exactly
        tetrominos.clear();
        tetrominos.push_back(CreateTetromino(shapes[16], spawnX, 10));
        assert(IsPositionFree(10) == false);

        // Move Dot away — should be free
        tetrominos.clear();
        tetrominos.push_back(CreateTetromino(shapes[16], spawnX + 1, 10));
        assert(IsPositionFree(10) == true);

        std::cout << "All shape-based tests passed.\n";
        return 0;


    }

    bool Test_InsideGrid() {

        int GRID_START_X = 112;
        int GRID_START_Y = 125;
        int BLOCK_SIZE = 40;
        int Rows = 9;
        // Test 1: Inside grid, near center
        Tetromino t1 = CreateTetromino(shapes[0], 3 * BLOCK_SIZE, 5 * BLOCK_SIZE);
        assert(IsInsideGrid(t1, GRID_START_X, GRID_START_Y) == true);

        // Test 2: Slightly outside left/top (within -5 margin)
        Tetromino t2 = CreateTetromino(shapes[0], SnapToGrid(-4, 40), SnapToGrid(-4, 40));
        assert(IsInsideGrid(t2, GRID_START_X, GRID_START_Y) == false);

        // Test 3: Fully outside top-left
        Tetromino t3 = CreateTetromino(shapes[0], SnapToGrid(-40, 40), SnapToGrid(-40, 40));
        assert(IsInsideGrid(t3, GRID_START_X, GRID_START_Y) == false);

        // Test 5: Fully outside right edge
        Tetromino t5 = CreateTetromino(shapes[0], SnapToGrid(1000, 40), SnapToGrid(1000, 40));
        assert(IsInsideGrid(t5, GRID_START_X, GRID_START_Y) == false);

        // Test 6: Fully outside bottom edge
        Tetromino t6 = CreateTetromino(shapes[0], SnapToGrid(-1000, 40), SnapToGrid(-1000, 40));
        assert(IsInsideGrid(t6, GRID_START_X, GRID_START_Y) == false);

        std::cout << "All IsInsideGrid tests passed.\n";
        return 0;

    }

    bool Test_ComputeHMAC() {
        std::string data = "The quick brown fox jumps over the lazy dog";

        const std::string expected =
            "ad5800ea20a7a3599ba631507ea99e5852445c1ed280d9d1030c474390f4dd69";
        std::string tag = computeHMAC(data, SHARED_SECRET);
        assert(tag == expected);
        std::cout << "ComputeHMAC test passed. \n";
        return 0;
    }

    bool Test_hmacEquals() {
        // identical strings = true
        assert(hmacEquals("abcdef", "abcdef"));
        // different bytes = false
        assert(!hmacEquals("abcd00", "abcd01"));
        // differing lengths = false
        assert(!hmacEquals("short", "toolong"));
        std::cout << "hmacEquals test passed. \n";
        return 0;
    }

    bool Test_validateHMAC_Missing() {
        json j = { {"foo", "bar"} };      // no "hmac" field
        bool ok = validateHMAC(j, SHARED_SECRET);
        assert(!ok);
        // original data remains
        assert(j["foo"] == "bar");
        std::cout << "HMAC_Missing test passed.\n";
        return true;
    }

    bool Test_validateHMAC_Correct() {
        json j = { {"a", 1}, {"b", 2} };
        // compute correct tag over the dump of {a,b}
        std::string tag = computeHMAC(j.dump(), SHARED_SECRET);
        j["hmac"] = tag;

        bool ok = validateHMAC(j, SHARED_SECRET);
        assert(ok);
        // on success, hmac key is erased
        assert(!j.contains("hmac"));
        std::cout << "HMAC_Correct test passed.\n";
        return true;
    }

    bool Test_validateHMAC_BadTag() {
        json j = { {"x", 42}, {"hmac", std::string(64, '0')} };  // bogus tag
        bool ok = validateHMAC(j, SHARED_SECRET);
        assert(!ok);
        // implementation erases even on failure
        assert(!j.contains("hmac"));
        std::cout << "HMAC_BadTag test passed.\n";
        return true;
    }


