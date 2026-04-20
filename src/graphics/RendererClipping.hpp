#pragma once
#include <vector>
#include "Line.hpp"
#include "Point.hpp"
#include "Polygon.hpp"
#include "Wireframe.hpp"
#include "BezierCurve.hpp"

// Clip de pontos usual usando os limites da window (-1.0f e 1.0f com NCS)
std::vector<core::Point> ClipPoints(const std::vector<core::Point> &v, const core::Point &wp0, const core::Point &wp1);
// Clipa uma linha somente utilizando liang barsky, utilizado na renderização do background
bool ClipLine(core::Line &line, const core::Point &wp0, const core::Point&wp1);
/* Liang-Barsky ou Cohen-Sutherland clipping, definido pelo parâmetro mode 
    @Liang-Barsky: mode = 0
    @Cohen-Sutherland: mode = 1
*/
std::vector<core::Line> ClipLines(const std::vector<core::Line> &v, const core::Point &wp0, const core::Point &wp1, int mode);
/* Quebra o wireframe em linhas e faz o clipping de linha. */
std::vector<core::Line> ClipWireframes(const std::vector<core::Wireframe> &v, const core::Point &wp0, const core::Point &wp1);
/* Sutherland–Hodgman clipping*/
std::vector<core::Polygon> ClipPolygons(const std::vector<core::Polygon> &v, const core::Point &wp0, const core::Point &wp1);
/*Redundante. Igual o clipping de wireframes*/
std::vector<core::Line> ClipBezierCurves(const std::vector<core::BezierCurve> &v, const core::Point &wp0, const core::Point &wp1);
