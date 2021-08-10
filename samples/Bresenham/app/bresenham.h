/********************************************************************
*                                                                   *
*                    Curve Rasterizing Algorithm                    *
*                                                                   *
********************************************************************/

/**
 * @author Zingl Alois
 * @date 22.08.2016
 * @version 1.2
*/

#pragma once

#include <cstdint>

/* Provided by application */

extern void setPixel(int x0, int y0);
extern void setPixel3d(int x0, int y0, int z0);
extern void setPixelAA(int x0, int y0, uint8_t alpha);

/* Function declarations */

void plotLine(int x0, int y0, int x1, int y1);
void plotLine3d(int x0, int y0, int z0, int x1, int y1, int z1);
void plotEllipse(int xm, int ym, int a, int b);
void plotOptimizedEllipse(int xm, int ym, int a, int b);
void plotCircle(int xm, int ym, int r);

/* rectangular parameter enclosing the ellipse */
void plotEllipseRect(int x0, int y0, int x1, int y1);

/* plot a limited quadratic Bezier segment */
void plotQuadBezierSeg(int x0, int y0, int x1, int y1, int x2, int y2);

/* plot any quadratic Bezier curve */
void plotQuadBezier(int x0, int y0, int x1, int y1, int x2, int y2);

/* plot a limited rational Bezier segment, squared weight */
void plotQuadRationalBezierSeg(int x0, int y0, int x1, int y1, int x2, int y2, float w);

/* plot any quadratic rational Bezier curve */
void plotQuadRationalBezier(int x0, int y0, int x1, int y1, int x2, int y2, float w);

/* plot ellipse rotated by angle (radian) */
void plotRotatedEllipse(int x, int y, int a, int b, float angle);

/* rectangle enclosing the ellipse, integer rotation angle */
void plotRotatedEllipseRect(int x0, int y0, int x1, int y1, long zd);

/* plot limited cubic Bezier segment */
void plotCubicBezierSeg(int x0, int y0, float x1, float y1, float x2, float y2, int x3, int y3);

/* plot any cubic Bezier curve */
void plotCubicBezier(int x0, int y0, int x1, int y1, int x2, int y2, int x3, int y3);

/* draw a black (0) anti-aliased line on white (255) background */
void plotLineAA(int x0, int y0, int x1, int y1);

/* draw a black anti-aliased circle on white background */
void plotCircleAA(int xm, int ym, int r);

/* draw a black anti-aliased rectangular ellipse on white background */
void plotEllipseRectAA(int x0, int y0, int x1, int y1);

/* draw an limited anti-aliased quadratic Bezier segment */
void plotQuadBezierSegAA(int x0, int y0, int x1, int y1, int x2, int y2);

/* draw an anti-aliased rational quadratic Bezier segment, squared weight */
void plotQuadRationalBezierSegAA(int x0, int y0, int x1, int y1, int x2, int y2, float w);

/* plot limited anti-aliased cubic Bezier segment */
void plotCubicBezierSegAA(int x0, int y0, float x1, float y1, float x2, float y2, int x3, int y3);

/* plot an anti-aliased line of width wd */
void plotLineWidth(int x0, int y0, int x1, int y1, float wd);

/* plot quadratic spline, destroys input arrays x,y */
void plotQuadSpline(int n, int x[], int y[]);

/* plot cubic spline, destroys input arrays x,y */
void plotCubicSpline(int n, int x[], int y[]);
