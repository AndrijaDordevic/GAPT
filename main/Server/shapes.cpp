#include "shapes.hpp"

// Shape definitions
static const std::vector<std::vector<SDL_Point>> SHAPES = {
    {{0, 0}, {1, 0}, {2, 0}, {2, 0}},    // HorizontalLine
    {{0, 0}, {0, 1}, {1, 1}, {1, 1}},    // SmallBottomLeft
    {{0, 0}, {0, 1}, {1, 1}, {2, 1}},    // BottomLeft
    {{0, 0}, {1, 0}, {1, 0}, {1, 1}},    // SmallRightTop
    {{0, 0}, {1, 0}, {2, 0}, {2, 1}},    // RightTop
    {{0, 0}, {1, 0}, {1, 1}, {2, 1}},    // ZShape
    {{0, 0}, {1, 0}, {0, 1}, {1, 1}},    // Square
    {{0, 0}, {1, 0}, {0, 1}, {0, 2}},    // UpsideDownL
    {{0, 0}, {1, 0}, {0, 1}, {0, 1}},    // SmallUpsideDownL
    {{0, 0}, {1, 0}, {1, 1}, {1, 2}},    // MirroredUpsideDownL
    {{0, 0}, {1, 0}, {2, 0}, {0, 1}},    // LeftTop
    {{0, 0}, {1, 0}, {1, 0}, {0, 1}},    // SmallLeftTop
    {{0, 0}, {0, 1}, {0, 2}, {0, 1}},    // ThreeBlocksI
    {{0, 0}, {0, 1}, {0, 1}, {0, 1}},    // TwoBlocksI
    {{0, 0}, {0, 1}, {0, 2}, {1, 2}},    // LShape
    {{0, 0}, {0, 1}, {1, 1}, {1, 2}},    // FourShape
    {{0, 0}, {0, 0}, {0, 0}, {0, 0}},    // DotShape
};

const std::vector<SDL_Point>& GetShape(ShapeType type) {
    return SHAPES[static_cast<size_t>(type)];
}

// (Optional) Helper to get the name as a string
const char* GetShapeName(ShapeType type) {
    static const char* NAMES[] = {
        "HorizontalLine",
        "SmallBottomLeft",
        "BottomLeft",
        "SmallRightTop",
        "RightTop",
        "ZShape",
        "Square",
        "UpsideDownL",
        "SmallUpsideDownL",
        "MirroredUpsideDownL",
        "LeftTop",
        "SmallLeftTop",
        "ThreeBlocksI",
        "TwoBlocksI",
        "LShape",
        "FourShape",
        "DotShape"
    };
    return NAMES[static_cast<size_t>(type)];
}