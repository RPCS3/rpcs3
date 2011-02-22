/**
 * (c) Copyright 1993, 1994, Silicon Graphics, Inc.
 * ALL RIGHTS RESERVED
 * Permission to use, copy, modify, and distribute this software for
 * any purpose and without fee is hereby granted, provided that the above
 * copyright notice appear in all copies and that both the copyright notice
 * and this permission notice appear in supporting documentation, and that
 * the name of Silicon Graphics, Inc. not be used in advertising
 * or publicity pertaining to distribution of the software without specific,
 * written prior permission.
 *
 * THE MATERIAL EMBODIED ON THIS SOFTWARE IS PROVIDED TO YOU "AS-IS"
 * AND WITHOUT WARRANTY OF ANY KIND, EXPRESS, IMPLIED OR OTHERWISE,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY OR
 * FITNESS FOR A PARTICULAR PURPOSE.  IN NO EVENT SHALL SILICON
 * GRAPHICS, INC.  BE LIABLE TO YOU OR ANYONE ELSE FOR ANY DIRECT,
 * SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY
 * KIND, OR ANY DAMAGES WHATSOEVER, INCLUDING WITHOUT LIMITATION,
 * LOSS OF PROFIT, LOSS OF USE, SAVINGS OR REVENUE, OR THE CLAIMS OF
 * THIRD PARTIES, WHETHER OR NOT SILICON GRAPHICS, INC.  HAS BEEN
 * ADVISED OF THE POSSIBILITY OF SUCH LOSS, HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE
 * POSSESSION, USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * US Government Users Restricted Rights
 * Use, duplication, or disclosure by the Government is subject to
 * restrictions set forth in FAR 52.227.19(c)(2) or subparagraph
 * (c)(1)(ii) of the Rights in Technical Data and Computer Software
 * clause at DFARS 252.227-7013 and/or in similar or successor
 * clauses in the FAR or the DOD or NASA FAR Supplement.
 * Unpublished-- rights reserved under the copyright laws of the
 * United States.  Contractor/manufacturer is Silicon Graphics,
 * Inc., 2011 N.  Shoreline Blvd., Mountain View, CA 94039-7311.
 *
 * OpenGL(TM) is a trademark of Silicon Graphics, Inc.
 */
#include <math.h>
#include <stdlib.h>  /* For rand(). */
#include <GLUT/glut.h>
#include "atlantis.h"

void
FishTransform(fishRec * fish)
{

    glTranslatef(fish->y, fish->z, -fish->x);
    glRotatef(-fish->psi, 0.0, 1.0, 0.0);
    glRotatef(fish->theta, 1.0, 0.0, 0.0);
    glRotatef(-fish->phi, 0.0, 0.0, 1.0);
}

void
WhalePilot(fishRec * fish)
{

    fish->phi = -20.0;
    fish->theta = 0.0;
    fish->psi -= 0.5;

    fish->x += WHALESPEED * fish->v * cos(fish->psi / RAD) * cos(fish->theta / RAD);
    fish->y += WHALESPEED * fish->v * sin(fish->psi / RAD) * cos(fish->theta / RAD);
    fish->z += WHALESPEED * fish->v * sin(fish->theta / RAD);
}

void
SharkPilot(fishRec * fish)
{
    static int sign = 1;
    float X, Y, Z, tpsi, ttheta, thetal;

    fish->xt = 60000.0;
    fish->yt = 0.0;
    fish->zt = 0.0;

    X = fish->xt - fish->x;
    Y = fish->yt - fish->y;
    Z = fish->zt - fish->z;

    thetal = fish->theta;

    ttheta = RAD * atan(Z / (sqrt(X * X + Y * Y)));

    if (ttheta > fish->theta + 0.25) {
        fish->theta += 0.5;
    } else if (ttheta < fish->theta - 0.25) {
        fish->theta -= 0.5;
    }
    if (fish->theta > 90.0) {
        fish->theta = 90.0;
    }
    if (fish->theta < -90.0) {
        fish->theta = -90.0;
    }
    fish->dtheta = fish->theta - thetal;

    tpsi = RAD * atan2(Y, X);

    fish->attack = 0;

    if (fabs(tpsi - fish->psi) < 10.0) {
        fish->attack = 1;
    } else if (fabs(tpsi - fish->psi) < 45.0) {
        if (fish->psi > tpsi) {
            fish->psi -= 0.5;
            if (fish->psi < -180.0) {
                fish->psi += 360.0;
            }
        } else if (fish->psi < tpsi) {
            fish->psi += 0.5;
            if (fish->psi > 180.0) {
                fish->psi -= 360.0;
            }
        }
    } else {
        if (rand() % 100 > 98) {
            sign = 1 - sign;
        }
        fish->psi += sign;
        if (fish->psi > 180.0) {
            fish->psi -= 360.0;
        }
        if (fish->psi < -180.0) {
            fish->psi += 360.0;
        }
    }

    if (fish->attack) {
        if (fish->v < 1.1) {
            fish->spurt = 1;
        }
        if (fish->spurt) {
            fish->v += 0.2;
        }
        if (fish->v > 5.0) {
            fish->spurt = 0;
        }
        if ((fish->v > 1.0) && (!fish->spurt)) {
            fish->v -= 0.2;
        }
    } else {
        if (!(rand() % 400) && (!fish->spurt)) {
            fish->spurt = 1;
        }
        if (fish->spurt) {
            fish->v += 0.05;
        }
        if (fish->v > 3.0) {
            fish->spurt = 0;
        }
        if ((fish->v > 1.0) && (!fish->spurt)) {
            fish->v -= 0.05;
        }
    }

    fish->x += SHARKSPEED * fish->v * cos(fish->psi / RAD) * cos(fish->theta / RAD);
    fish->y += SHARKSPEED * fish->v * sin(fish->psi / RAD) * cos(fish->theta / RAD);
    fish->z += SHARKSPEED * fish->v * sin(fish->theta / RAD);
}

void
SharkMiss(int i)
{
    int j;
    float avoid, thetal;
    float X, Y, Z, R;

    for (j = 0; j < NUM_SHARKS; j++) {
        if (j != i) {
            X = sharks[j].x - sharks[i].x;
            Y = sharks[j].y - sharks[i].y;
            Z = sharks[j].z - sharks[i].z;

            R = sqrt(X * X + Y * Y + Z * Z);

            avoid = 1.0;
            thetal = sharks[i].theta;

            if (R < SHARKSIZE) {
                if (Z > 0.0) {
                    sharks[i].theta -= avoid;
                } else {
                    sharks[i].theta += avoid;
                }
            }
            sharks[i].dtheta += (sharks[i].theta - thetal);
        }
    }
}
