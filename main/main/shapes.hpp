#pragma once
#include <vector>
#include <SDL3/SDL.h>

enum class ShapeType : uint8_t {  // uint8_t ensures 1-byte transmission
    HorizontalLine = 0,
    SmallBottomLeft = 1,
    BottomLeft = 2,
    SmallRightTop = 3,
    RightTop = 4,
    ZShape = 5,
    Square = 6,
    UpsideDownL = 7,
    SmallUpsideDownL = 8,
    MirroredUpsideDownL = 9,
    LeftTop = 10,
    SmallLeftTop = 11,
    ThreeBlocksI = 12,
    TwoBlocksI = 13,
    LShape = 14,
    FourShape = 15,
    DotShape = 16,
    Count = 17
};
// Returns the shape points for a given type
const std::vector<SDL_Point>& GetShape(ShapeType type);

// (Optional) Returns the name as a string
const char* GetShapeName(ShapeType type);