#ifndef OVERLAY_CURSOR_H
#define OVERLAY_CURSOR_H

namespace overlay {
namespace helper {

enum class Cursor {
  None = 0,
  Arrow = 1,
  ArrowProgress = 2,
  Wait = 3,
  Text = 4,
  Pointer = 5,
  Help = 6,
  Crosshair = 7,
  Move = 8,
  ResizeNorthEastSouthWest = 9,
  ResizeNorthSouth = 10,
  ResizeNorthWestSouthEast = 11,
  ResizeWestEast = 12,
  No = 13,
  Alias = 14,
  Cell = 15,
  ColumnResize = 16,
  Grab = 17,
  Grabbing = 18,
  PanningEast = 19,
  PanningMiddle = 20,
  PanningMiddleHorizontal = 21,
  PanningMiddleVertical = 22,
  PanningNorth = 23,
  PanningNorthEast = 24,
  PanningNorthWest = 25,
  PanningSouth = 26,
  PanningSouthEast = 27,
  PanningSouthWest = 28,
  PanningWest = 29,
  RowResize = 30,
  VerticalText = 31,
  ZoomIn = 32,
  ZoomOut = 33,
  Copy = 34
};

}  // namespace helper
}  // namespace overlay

#endif