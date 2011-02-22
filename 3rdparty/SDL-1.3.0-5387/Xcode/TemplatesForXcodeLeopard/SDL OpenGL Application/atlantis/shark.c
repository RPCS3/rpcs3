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
#include <GLUT/glut.h>
#include <math.h>
#include "atlantis.h"
/* *INDENT-OFF* */
static float N002[3] = {0.000077 ,-0.020611 ,0.999788};
static float N003[3] = {0.961425 ,0.258729 ,-0.093390};
static float N004[3] = {0.510811 ,-0.769633 ,-0.383063};
static float N005[3] = {0.400123 ,0.855734 ,-0.328055};
static float N006[3] = {-0.770715 ,0.610204 ,-0.183440};
static float N007[3] = {-0.915597 ,-0.373345 ,-0.149316};
static float N008[3] = {-0.972788 ,0.208921 ,-0.100179};
static float N009[3] = {-0.939713 ,-0.312268 ,-0.139383};
static float N010[3] = {-0.624138 ,-0.741047 ,-0.247589};
static float N011[3] = {0.591434 ,-0.768401 ,-0.244471};
static float N012[3] = {0.935152 ,-0.328495 ,-0.132598};
static float N013[3] = {0.997102 ,0.074243 ,-0.016593};
static float N014[3] = {0.969995 ,0.241712 ,-0.026186};
static float N015[3] = {0.844539 ,0.502628 ,-0.184714};
static float N016[3] = {-0.906608 ,0.386308 ,-0.169787};
static float N017[3] = {-0.970016 ,0.241698 ,-0.025516};
static float N018[3] = {-0.998652 ,0.050493 ,-0.012045};
static float N019[3] = {-0.942685 ,-0.333051 ,-0.020556};
static float N020[3] = {-0.660944 ,-0.750276 ,0.015480};
static float N021[3] = {0.503549 ,-0.862908 ,-0.042749};
static float N022[3] = {0.953202 ,-0.302092 ,-0.012089};
static float N023[3] = {0.998738 ,0.023574 ,0.044344};
static float N024[3] = {0.979297 ,0.193272 ,0.060202};
static float N025[3] = {0.798300 ,0.464885 ,0.382883};
static float N026[3] = {-0.756590 ,0.452403 ,0.472126};
static float N027[3] = {-0.953855 ,0.293003 ,0.065651};
static float N028[3] = {-0.998033 ,0.040292 ,0.048028};
static float N029[3] = {-0.977079 ,-0.204288 ,0.059858};
static float N030[3] = {-0.729117 ,-0.675304 ,0.111140};
static float N031[3] = {0.598361 ,-0.792753 ,0.116221};
static float N032[3] = {0.965192 ,-0.252991 ,0.066332};
static float N033[3] = {0.998201 ,-0.002790 ,0.059892};
static float N034[3] = {0.978657 ,0.193135 ,0.070207};
static float N035[3] = {0.718815 ,0.680392 ,0.142733};
static float N036[3] = {-0.383096 ,0.906212 ,0.178936};
static float N037[3] = {-0.952831 ,0.292590 ,0.080647};
static float N038[3] = {-0.997680 ,0.032417 ,0.059861};
static float N039[3] = {-0.982629 ,-0.169881 ,0.074700};
static float N040[3] = {-0.695424 ,-0.703466 ,0.146700};
static float N041[3] = {0.359323 ,-0.915531 ,0.180805};
static float N042[3] = {0.943356 ,-0.319387 ,0.089842};
static float N043[3] = {0.998272 ,-0.032435 ,0.048993};
static float N044[3] = {0.978997 ,0.193205 ,0.065084};
static float N045[3] = {0.872144 ,0.470094 ,-0.135565};
static float N046[3] = {-0.664282 ,0.737945 ,-0.119027};
static float N047[3] = {-0.954508 ,0.288570 ,0.075107};
static float N048[3] = {-0.998273 ,0.032406 ,0.048993};
static float N049[3] = {-0.979908 ,-0.193579 ,0.048038};
static float N050[3] = {-0.858736 ,-0.507202 ,-0.072938};
static float N051[3] = {0.643545 ,-0.763887 ,-0.048237};
static float N052[3] = {0.955580 ,-0.288954 ,0.058068};
static float N058[3] = {0.000050 ,0.793007 ,-0.609213};
static float N059[3] = {0.913510 ,0.235418 ,-0.331779};
static float N060[3] = {-0.807970 ,0.495000 ,-0.319625};
static float N061[3] = {0.000000 ,0.784687 ,-0.619892};
static float N062[3] = {0.000000 ,-1.000000 ,0.000000};
static float N063[3] = {0.000000 ,1.000000 ,0.000000};
static float N064[3] = {0.000000 ,1.000000 ,0.000000};
static float N065[3] = {0.000000 ,1.000000 ,0.000000};
static float N066[3] = {-0.055784 ,0.257059 ,0.964784};
static float N069[3] = {-0.000505 ,-0.929775 ,-0.368127};
static float N070[3] = {0.000000 ,1.000000 ,0.000000};
static float P002[3] = {0.00, -36.59, 5687.72};
static float P003[3] = {90.00, 114.73, 724.38};
static float P004[3] = {58.24, -146.84, 262.35};
static float P005[3] = {27.81, 231.52, 510.43};
static float P006[3] = {-27.81, 230.43, 509.76};
static float P007[3] = {-46.09, -146.83, 265.84};
static float P008[3] = {-90.00, 103.84, 718.53};
static float P009[3] = {-131.10, -165.92, 834.85};
static float P010[3] = {-27.81, -285.31, 500.00};
static float P011[3] = {27.81, -285.32, 500.00};
static float P012[3] = {147.96, -170.89, 845.50};
static float P013[3] = {180.00, 0.00, 2000.00};
static float P014[3] = {145.62, 352.67, 2000.00};
static float P015[3] = {55.62, 570.63, 2000.00};
static float P016[3] = {-55.62, 570.64, 2000.00};
static float P017[3] = {-145.62, 352.68, 2000.00};
static float P018[3] = {-180.00, 0.01, 2000.00};
static float P019[3] = {-178.20, -352.66, 2001.61};
static float P020[3] = {-55.63, -570.63, 2000.00};
static float P021[3] = {55.62, -570.64, 2000.00};
static float P022[3] = {179.91, -352.69, 1998.39};
static float P023[3] = {150.00, 0.00, 3000.00};
static float P024[3] = {121.35, 293.89, 3000.00};
static float P025[3] = {46.35, 502.93, 2883.09};
static float P026[3] = {-46.35, 497.45, 2877.24};
static float P027[3] = {-121.35, 293.90, 3000.00};
static float P028[3] = {-150.00, 0.00, 3000.00};
static float P029[3] = {-152.21, -304.84, 2858.68};
static float P030[3] = {-46.36, -475.52, 3000.00};
static float P031[3] = {46.35, -475.53, 3000.00};
static float P032[3] = {155.64, -304.87, 2863.50};
static float P033[3] = {90.00, 0.00, 4000.00};
static float P034[3] = {72.81, 176.33, 4000.00};
static float P035[3] = {27.81, 285.32, 4000.00};
static float P036[3] = {-27.81, 285.32, 4000.00};
static float P037[3] = {-72.81, 176.34, 4000.00};
static float P038[3] = {-90.00, 0.00, 4000.00};
static float P039[3] = {-72.81, -176.33, 4000.00};
static float P040[3] = {-27.81, -285.31, 4000.00};
static float P041[3] = {27.81, -285.32, 4000.00};
static float P042[3] = {72.81, -176.34, 4000.00};
static float P043[3] = {30.00, 0.00, 5000.00};
static float P044[3] = {24.27, 58.78, 5000.00};
static float P045[3] = {9.27, 95.11, 5000.00};
static float P046[3] = {-9.27, 95.11, 5000.00};
static float P047[3] = {-24.27, 58.78, 5000.00};
static float P048[3] = {-30.00, 0.00, 5000.00};
static float P049[3] = {-24.27, -58.78, 5000.00};
static float P050[3] = {-9.27, -95.10, 5000.00};
static float P051[3] = {9.27, -95.11, 5000.00};
static float P052[3] = {24.27, -58.78, 5000.00};
static float P058[3] = {0.00, 1212.72, 2703.08};
static float P059[3] = {50.36, 0.00, 108.14};
static float P060[3] = {-22.18, 0.00, 108.14};
static float P061[3] = {0.00, 1181.61, 6344.65};
static float P062[3] = {516.45, -887.08, 2535.45};
static float P063[3] = {-545.69, -879.31, 2555.63};
static float P064[3] = {618.89, -1005.64, 2988.32};
static float P065[3] = {-635.37, -1014.79, 2938.68};
static float P066[3] = {0.00, 1374.43, 3064.18};
static float P069[3] = {0.00, -418.25, 5765.04};
static float P070[3] = {0.00, 1266.91, 6629.60};
static float P071[3] = {-139.12, -124.96, 997.98};
static float P072[3] = {-139.24, -110.18, 1020.68};
static float P073[3] = {-137.33, -94.52, 1022.63};
static float P074[3] = {-137.03, -79.91, 996.89};
static float P075[3] = {-135.21, -91.48, 969.14};
static float P076[3] = {-135.39, -110.87, 968.76};
static float P077[3] = {150.23, -78.44, 995.53};
static float P078[3] = {152.79, -92.76, 1018.46};
static float P079[3] = {154.19, -110.20, 1020.55};
static float P080[3] = {151.33, -124.15, 993.77};
static float P081[3] = {150.49, -111.19, 969.86};
static float P082[3] = {150.79, -92.41, 969.70};
static float iP002[3] = {0.00, -36.59, 5687.72};
static float iP004[3] = {58.24, -146.84, 262.35};
static float iP007[3] = {-46.09, -146.83, 265.84};
static float iP010[3] = {-27.81, -285.31, 500.00};
static float iP011[3] = {27.81, -285.32, 500.00};
static float iP023[3] = {150.00, 0.00, 3000.00};
static float iP024[3] = {121.35, 293.89, 3000.00};
static float iP025[3] = {46.35, 502.93, 2883.09};
static float iP026[3] = {-46.35, 497.45, 2877.24};
static float iP027[3] = {-121.35, 293.90, 3000.00};
static float iP028[3] = {-150.00, 0.00, 3000.00};
static float iP029[3] = {-121.35, -304.84, 2853.86};
static float iP030[3] = {-46.36, -475.52, 3000.00};
static float iP031[3] = {46.35, -475.53, 3000.00};
static float iP032[3] = {121.35, -304.87, 2853.86};
static float iP033[3] = {90.00, 0.00, 4000.00};
static float iP034[3] = {72.81, 176.33, 4000.00};
static float iP035[3] = {27.81, 285.32, 4000.00};
static float iP036[3] = {-27.81, 285.32, 4000.00};
static float iP037[3] = {-72.81, 176.34, 4000.00};
static float iP038[3] = {-90.00, 0.00, 4000.00};
static float iP039[3] = {-72.81, -176.33, 4000.00};
static float iP040[3] = {-27.81, -285.31, 4000.00};
static float iP041[3] = {27.81, -285.32, 4000.00};
static float iP042[3] = {72.81, -176.34, 4000.00};
static float iP043[3] = {30.00, 0.00, 5000.00};
static float iP044[3] = {24.27, 58.78, 5000.00};
static float iP045[3] = {9.27, 95.11, 5000.00};
static float iP046[3] = {-9.27, 95.11, 5000.00};
static float iP047[3] = {-24.27, 58.78, 5000.00};
static float iP048[3] = {-30.00, 0.00, 5000.00};
static float iP049[3] = {-24.27, -58.78, 5000.00};
static float iP050[3] = {-9.27, -95.10, 5000.00};
static float iP051[3] = {9.27, -95.11, 5000.00};
static float iP052[3] = {24.27, -58.78, 5000.00};
static float iP061[3] = {0.00, 1181.61, 6344.65};
static float iP069[3] = {0.00, -418.25, 5765.04};
static float iP070[3] = {0.00, 1266.91, 6629.60};
/* *INDENT-ON* */

void
Fish001(void)
{
    glBegin(GL_POLYGON);
    glNormal3fv(N005);
    glVertex3fv(P005);
    glNormal3fv(N059);
    glVertex3fv(P059);
    glNormal3fv(N060);
    glVertex3fv(P060);
    glNormal3fv(N006);
    glVertex3fv(P006);
    glEnd();
    glBegin(GL_POLYGON);
    glNormal3fv(N015);
    glVertex3fv(P015);
    glNormal3fv(N005);
    glVertex3fv(P005);
    glNormal3fv(N006);
    glVertex3fv(P006);
    glNormal3fv(N016);
    glVertex3fv(P016);
    glEnd();
    glBegin(GL_POLYGON);
    glNormal3fv(N006);
    glVertex3fv(P006);
    glNormal3fv(N060);
    glVertex3fv(P060);
    glNormal3fv(N008);
    glVertex3fv(P008);
    glEnd();
    glBegin(GL_POLYGON);
    glNormal3fv(N016);
    glVertex3fv(P016);
    glNormal3fv(N006);
    glVertex3fv(P006);
    glNormal3fv(N008);
    glVertex3fv(P008);
    glEnd();
    glBegin(GL_POLYGON);
    glNormal3fv(N016);
    glVertex3fv(P016);
    glNormal3fv(N008);
    glVertex3fv(P008);
    glNormal3fv(N017);
    glVertex3fv(P017);
    glEnd();
    glBegin(GL_POLYGON);
    glNormal3fv(N017);
    glVertex3fv(P017);
    glNormal3fv(N008);
    glVertex3fv(P008);
    glNormal3fv(N018);
    glVertex3fv(P018);
    glEnd();
    glBegin(GL_POLYGON);
    glNormal3fv(N008);
    glVertex3fv(P008);
    glNormal3fv(N009);
    glVertex3fv(P009);
    glNormal3fv(N018);
    glVertex3fv(P018);
    glEnd();
    glBegin(GL_POLYGON);
    glNormal3fv(N008);
    glVertex3fv(P008);
    glNormal3fv(N060);
    glVertex3fv(P060);
    glNormal3fv(N009);
    glVertex3fv(P009);
    glEnd();
    glBegin(GL_POLYGON);
    glNormal3fv(N007);
    glVertex3fv(P007);
    glNormal3fv(N010);
    glVertex3fv(P010);
    glNormal3fv(N009);
    glVertex3fv(P009);
    glEnd();
    glBegin(GL_POLYGON);
    glNormal3fv(N009);
    glVertex3fv(P009);
    glNormal3fv(N019);
    glVertex3fv(P019);
    glNormal3fv(N018);
    glVertex3fv(P018);
    glEnd();
    glBegin(GL_POLYGON);
    glNormal3fv(N009);
    glVertex3fv(P009);
    glNormal3fv(N010);
    glVertex3fv(P010);
    glNormal3fv(N019);
    glVertex3fv(P019);
    glEnd();
    glBegin(GL_POLYGON);
    glNormal3fv(N010);
    glVertex3fv(P010);
    glNormal3fv(N020);
    glVertex3fv(P020);
    glNormal3fv(N019);
    glVertex3fv(P019);
    glEnd();
    glBegin(GL_POLYGON);
    glNormal3fv(N010);
    glVertex3fv(P010);
    glNormal3fv(N011);
    glVertex3fv(P011);
    glNormal3fv(N021);
    glVertex3fv(P021);
    glNormal3fv(N020);
    glVertex3fv(P020);
    glEnd();
    glBegin(GL_POLYGON);
    glNormal3fv(N004);
    glVertex3fv(P004);
    glNormal3fv(N011);
    glVertex3fv(P011);
    glNormal3fv(N010);
    glVertex3fv(P010);
    glNormal3fv(N007);
    glVertex3fv(P007);
    glEnd();
    glBegin(GL_POLYGON);
    glNormal3fv(N004);
    glVertex3fv(P004);
    glNormal3fv(N012);
    glVertex3fv(P012);
    glNormal3fv(N011);
    glVertex3fv(P011);
    glEnd();
    glBegin(GL_POLYGON);
    glNormal3fv(N012);
    glVertex3fv(P012);
    glNormal3fv(N022);
    glVertex3fv(P022);
    glNormal3fv(N011);
    glVertex3fv(P011);
    glEnd();
    glBegin(GL_POLYGON);
    glNormal3fv(N011);
    glVertex3fv(P011);
    glNormal3fv(N022);
    glVertex3fv(P022);
    glNormal3fv(N021);
    glVertex3fv(P021);
    glEnd();
    glBegin(GL_POLYGON);
    glNormal3fv(N059);
    glVertex3fv(P059);
    glNormal3fv(N005);
    glVertex3fv(P005);
    glNormal3fv(N015);
    glVertex3fv(P015);
    glEnd();
    glBegin(GL_POLYGON);
    glNormal3fv(N015);
    glVertex3fv(P015);
    glNormal3fv(N014);
    glVertex3fv(P014);
    glNormal3fv(N003);
    glVertex3fv(P003);
    glEnd();
    glBegin(GL_POLYGON);
    glNormal3fv(N015);
    glVertex3fv(P015);
    glNormal3fv(N003);
    glVertex3fv(P003);
    glNormal3fv(N059);
    glVertex3fv(P059);
    glEnd();
    glBegin(GL_POLYGON);
    glNormal3fv(N014);
    glVertex3fv(P014);
    glNormal3fv(N013);
    glVertex3fv(P013);
    glNormal3fv(N003);
    glVertex3fv(P003);
    glEnd();
    glBegin(GL_POLYGON);
    glNormal3fv(N003);
    glVertex3fv(P003);
    glNormal3fv(N012);
    glVertex3fv(P012);
    glNormal3fv(N059);
    glVertex3fv(P059);
    glEnd();
    glBegin(GL_POLYGON);
    glNormal3fv(N013);
    glVertex3fv(P013);
    glNormal3fv(N012);
    glVertex3fv(P012);
    glNormal3fv(N003);
    glVertex3fv(P003);
    glEnd();
    glBegin(GL_POLYGON);
    glNormal3fv(N013);
    glVertex3fv(P013);
    glNormal3fv(N022);
    glVertex3fv(P022);
    glNormal3fv(N012);
    glVertex3fv(P012);
    glEnd();
    glBegin(GL_POLYGON);
    glVertex3fv(P071);
    glVertex3fv(P072);
    glVertex3fv(P073);
    glVertex3fv(P074);
    glVertex3fv(P075);
    glVertex3fv(P076);
    glEnd();
    glBegin(GL_POLYGON);
    glVertex3fv(P077);
    glVertex3fv(P078);
    glVertex3fv(P079);
    glVertex3fv(P080);
    glVertex3fv(P081);
    glVertex3fv(P082);
    glEnd();
}

void
Fish002(void)
{
    glBegin(GL_POLYGON);
    glNormal3fv(N013);
    glVertex3fv(P013);
    glNormal3fv(N014);
    glVertex3fv(P014);
    glNormal3fv(N024);
    glVertex3fv(P024);
    glNormal3fv(N023);
    glVertex3fv(P023);
    glEnd();
    glBegin(GL_POLYGON);
    glNormal3fv(N014);
    glVertex3fv(P014);
    glNormal3fv(N015);
    glVertex3fv(P015);
    glNormal3fv(N025);
    glVertex3fv(P025);
    glNormal3fv(N024);
    glVertex3fv(P024);
    glEnd();
    glBegin(GL_POLYGON);
    glNormal3fv(N016);
    glVertex3fv(P016);
    glNormal3fv(N017);
    glVertex3fv(P017);
    glNormal3fv(N027);
    glVertex3fv(P027);
    glNormal3fv(N026);
    glVertex3fv(P026);
    glEnd();
    glBegin(GL_POLYGON);
    glNormal3fv(N017);
    glVertex3fv(P017);
    glNormal3fv(N018);
    glVertex3fv(P018);
    glNormal3fv(N028);
    glVertex3fv(P028);
    glNormal3fv(N027);
    glVertex3fv(P027);
    glEnd();
    glBegin(GL_POLYGON);
    glNormal3fv(N020);
    glVertex3fv(P020);
    glNormal3fv(N021);
    glVertex3fv(P021);
    glNormal3fv(N031);
    glVertex3fv(P031);
    glNormal3fv(N030);
    glVertex3fv(P030);
    glEnd();
    glBegin(GL_POLYGON);
    glNormal3fv(N013);
    glVertex3fv(P013);
    glNormal3fv(N023);
    glVertex3fv(P023);
    glNormal3fv(N022);
    glVertex3fv(P022);
    glEnd();
    glBegin(GL_POLYGON);
    glNormal3fv(N022);
    glVertex3fv(P022);
    glNormal3fv(N023);
    glVertex3fv(P023);
    glNormal3fv(N032);
    glVertex3fv(P032);
    glEnd();
    glBegin(GL_POLYGON);
    glNormal3fv(N022);
    glVertex3fv(P022);
    glNormal3fv(N032);
    glVertex3fv(P032);
    glNormal3fv(N031);
    glVertex3fv(P031);
    glEnd();
    glBegin(GL_POLYGON);
    glNormal3fv(N022);
    glVertex3fv(P022);
    glNormal3fv(N031);
    glVertex3fv(P031);
    glNormal3fv(N021);
    glVertex3fv(P021);
    glEnd();
    glBegin(GL_POLYGON);
    glNormal3fv(N018);
    glVertex3fv(P018);
    glNormal3fv(N019);
    glVertex3fv(P019);
    glNormal3fv(N029);
    glVertex3fv(P029);
    glEnd();
    glBegin(GL_POLYGON);
    glNormal3fv(N018);
    glVertex3fv(P018);
    glNormal3fv(N029);
    glVertex3fv(P029);
    glNormal3fv(N028);
    glVertex3fv(P028);
    glEnd();
    glBegin(GL_POLYGON);
    glNormal3fv(N019);
    glVertex3fv(P019);
    glNormal3fv(N020);
    glVertex3fv(P020);
    glNormal3fv(N030);
    glVertex3fv(P030);
    glEnd();
    glBegin(GL_POLYGON);
    glNormal3fv(N019);
    glVertex3fv(P019);
    glNormal3fv(N030);
    glVertex3fv(P030);
    glNormal3fv(N029);
    glVertex3fv(P029);
    glEnd();
}

void
Fish003(void)
{
    glBegin(GL_POLYGON);
    glNormal3fv(N032);
    glVertex3fv(P032);
    glNormal3fv(N023);
    glVertex3fv(P023);
    glNormal3fv(N033);
    glVertex3fv(P033);
    glNormal3fv(N042);
    glVertex3fv(P042);
    glEnd();
    glBegin(GL_POLYGON);
    glNormal3fv(N031);
    glVertex3fv(P031);
    glNormal3fv(N032);
    glVertex3fv(P032);
    glNormal3fv(N042);
    glVertex3fv(P042);
    glNormal3fv(N041);
    glVertex3fv(P041);
    glEnd();
    glBegin(GL_POLYGON);
    glNormal3fv(N023);
    glVertex3fv(P023);
    glNormal3fv(N024);
    glVertex3fv(P024);
    glNormal3fv(N034);
    glVertex3fv(P034);
    glNormal3fv(N033);
    glVertex3fv(P033);
    glEnd();
    glBegin(GL_POLYGON);
    glNormal3fv(N024);
    glVertex3fv(P024);
    glNormal3fv(N025);
    glVertex3fv(P025);
    glNormal3fv(N035);
    glVertex3fv(P035);
    glNormal3fv(N034);
    glVertex3fv(P034);
    glEnd();
    glBegin(GL_POLYGON);
    glNormal3fv(N030);
    glVertex3fv(P030);
    glNormal3fv(N031);
    glVertex3fv(P031);
    glNormal3fv(N041);
    glVertex3fv(P041);
    glNormal3fv(N040);
    glVertex3fv(P040);
    glEnd();
    glBegin(GL_POLYGON);
    glNormal3fv(N025);
    glVertex3fv(P025);
    glNormal3fv(N026);
    glVertex3fv(P026);
    glNormal3fv(N036);
    glVertex3fv(P036);
    glNormal3fv(N035);
    glVertex3fv(P035);
    glEnd();
    glBegin(GL_POLYGON);
    glNormal3fv(N026);
    glVertex3fv(P026);
    glNormal3fv(N027);
    glVertex3fv(P027);
    glNormal3fv(N037);
    glVertex3fv(P037);
    glNormal3fv(N036);
    glVertex3fv(P036);
    glEnd();
    glBegin(GL_POLYGON);
    glNormal3fv(N027);
    glVertex3fv(P027);
    glNormal3fv(N028);
    glVertex3fv(P028);
    glNormal3fv(N038);
    glVertex3fv(P038);
    glNormal3fv(N037);
    glVertex3fv(P037);
    glEnd();
    glBegin(GL_POLYGON);
    glNormal3fv(N028);
    glVertex3fv(P028);
    glNormal3fv(N029);
    glVertex3fv(P029);
    glNormal3fv(N039);
    glVertex3fv(P039);
    glNormal3fv(N038);
    glVertex3fv(P038);
    glEnd();
    glBegin(GL_POLYGON);
    glNormal3fv(N029);
    glVertex3fv(P029);
    glNormal3fv(N030);
    glVertex3fv(P030);
    glNormal3fv(N040);
    glVertex3fv(P040);
    glNormal3fv(N039);
    glVertex3fv(P039);
    glEnd();
}

void
Fish004(void)
{
    glBegin(GL_POLYGON);
    glNormal3fv(N040);
    glVertex3fv(P040);
    glNormal3fv(N041);
    glVertex3fv(P041);
    glNormal3fv(N051);
    glVertex3fv(P051);
    glNormal3fv(N050);
    glVertex3fv(P050);
    glEnd();
    glBegin(GL_POLYGON);
    glNormal3fv(N041);
    glVertex3fv(P041);
    glNormal3fv(N042);
    glVertex3fv(P042);
    glNormal3fv(N052);
    glVertex3fv(P052);
    glNormal3fv(N051);
    glVertex3fv(P051);
    glEnd();
    glBegin(GL_POLYGON);
    glNormal3fv(N042);
    glVertex3fv(P042);
    glNormal3fv(N033);
    glVertex3fv(P033);
    glNormal3fv(N043);
    glVertex3fv(P043);
    glNormal3fv(N052);
    glVertex3fv(P052);
    glEnd();
    glBegin(GL_POLYGON);
    glNormal3fv(N033);
    glVertex3fv(P033);
    glNormal3fv(N034);
    glVertex3fv(P034);
    glNormal3fv(N044);
    glVertex3fv(P044);
    glNormal3fv(N043);
    glVertex3fv(P043);
    glEnd();
    glBegin(GL_POLYGON);
    glNormal3fv(N034);
    glVertex3fv(P034);
    glNormal3fv(N035);
    glVertex3fv(P035);
    glNormal3fv(N045);
    glVertex3fv(P045);
    glNormal3fv(N044);
    glVertex3fv(P044);
    glEnd();
    glBegin(GL_POLYGON);
    glNormal3fv(N035);
    glVertex3fv(P035);
    glNormal3fv(N036);
    glVertex3fv(P036);
    glNormal3fv(N046);
    glVertex3fv(P046);
    glNormal3fv(N045);
    glVertex3fv(P045);
    glEnd();
    glBegin(GL_POLYGON);
    glNormal3fv(N036);
    glVertex3fv(P036);
    glNormal3fv(N037);
    glVertex3fv(P037);
    glNormal3fv(N047);
    glVertex3fv(P047);
    glNormal3fv(N046);
    glVertex3fv(P046);
    glEnd();
    glBegin(GL_POLYGON);
    glNormal3fv(N037);
    glVertex3fv(P037);
    glNormal3fv(N038);
    glVertex3fv(P038);
    glNormal3fv(N048);
    glVertex3fv(P048);
    glNormal3fv(N047);
    glVertex3fv(P047);
    glEnd();
    glBegin(GL_POLYGON);
    glNormal3fv(N038);
    glVertex3fv(P038);
    glNormal3fv(N039);
    glVertex3fv(P039);
    glNormal3fv(N049);
    glVertex3fv(P049);
    glNormal3fv(N048);
    glVertex3fv(P048);
    glEnd();
    glBegin(GL_POLYGON);
    glNormal3fv(N039);
    glVertex3fv(P039);
    glNormal3fv(N040);
    glVertex3fv(P040);
    glNormal3fv(N050);
    glVertex3fv(P050);
    glNormal3fv(N049);
    glVertex3fv(P049);
    glEnd();
    glBegin(GL_POLYGON);
    glNormal3fv(N070);
    glVertex3fv(P070);
    glNormal3fv(N061);
    glVertex3fv(P061);
    glNormal3fv(N002);
    glVertex3fv(P002);
    glEnd();
    glBegin(GL_POLYGON);
    glNormal3fv(N061);
    glVertex3fv(P061);
    glNormal3fv(N046);
    glVertex3fv(P046);
    glNormal3fv(N002);
    glVertex3fv(P002);
    glEnd();
    glBegin(GL_POLYGON);
    glNormal3fv(N045);
    glVertex3fv(P045);
    glNormal3fv(N046);
    glVertex3fv(P046);
    glNormal3fv(N061);
    glVertex3fv(P061);
    glEnd();
    glBegin(GL_POLYGON);
    glNormal3fv(N002);
    glVertex3fv(P002);
    glNormal3fv(N061);
    glVertex3fv(P061);
    glNormal3fv(N070);
    glVertex3fv(P070);
    glEnd();
    glBegin(GL_POLYGON);
    glNormal3fv(N002);
    glVertex3fv(P002);
    glNormal3fv(N045);
    glVertex3fv(P045);
    glNormal3fv(N061);
    glVertex3fv(P061);
    glEnd();
}

void
Fish005(void)
{
    glBegin(GL_POLYGON);
    glNormal3fv(N002);
    glVertex3fv(P002);
    glNormal3fv(N044);
    glVertex3fv(P044);
    glNormal3fv(N045);
    glVertex3fv(P045);
    glEnd();
    glBegin(GL_POLYGON);
    glNormal3fv(N002);
    glVertex3fv(P002);
    glNormal3fv(N043);
    glVertex3fv(P043);
    glNormal3fv(N044);
    glVertex3fv(P044);
    glEnd();
    glBegin(GL_POLYGON);
    glNormal3fv(N002);
    glVertex3fv(P002);
    glNormal3fv(N052);
    glVertex3fv(P052);
    glNormal3fv(N043);
    glVertex3fv(P043);
    glEnd();
    glBegin(GL_POLYGON);
    glNormal3fv(N002);
    glVertex3fv(P002);
    glNormal3fv(N051);
    glVertex3fv(P051);
    glNormal3fv(N052);
    glVertex3fv(P052);
    glEnd();
    glBegin(GL_POLYGON);
    glNormal3fv(N002);
    glVertex3fv(P002);
    glNormal3fv(N046);
    glVertex3fv(P046);
    glNormal3fv(N047);
    glVertex3fv(P047);
    glEnd();
    glBegin(GL_POLYGON);
    glNormal3fv(N002);
    glVertex3fv(P002);
    glNormal3fv(N047);
    glVertex3fv(P047);
    glNormal3fv(N048);
    glVertex3fv(P048);
    glEnd();
    glBegin(GL_POLYGON);
    glNormal3fv(N002);
    glVertex3fv(P002);
    glNormal3fv(N048);
    glVertex3fv(P048);
    glNormal3fv(N049);
    glVertex3fv(P049);
    glEnd();
    glBegin(GL_POLYGON);
    glNormal3fv(N002);
    glVertex3fv(P002);
    glNormal3fv(N049);
    glVertex3fv(P049);
    glNormal3fv(N050);
    glVertex3fv(P050);
    glEnd();
    glBegin(GL_POLYGON);
    glNormal3fv(N050);
    glVertex3fv(P050);
    glNormal3fv(N051);
    glVertex3fv(P051);
    glNormal3fv(N069);
    glVertex3fv(P069);
    glEnd();
    glBegin(GL_POLYGON);
    glNormal3fv(N051);
    glVertex3fv(P051);
    glNormal3fv(N002);
    glVertex3fv(P002);
    glNormal3fv(N069);
    glVertex3fv(P069);
    glEnd();
    glBegin(GL_POLYGON);
    glNormal3fv(N050);
    glVertex3fv(P050);
    glNormal3fv(N069);
    glVertex3fv(P069);
    glNormal3fv(N002);
    glVertex3fv(P002);
    glEnd();
}

void
Fish006(void)
{
    glBegin(GL_POLYGON);
    glNormal3fv(N066);
    glVertex3fv(P066);
    glNormal3fv(N016);
    glVertex3fv(P016);
    glNormal3fv(N026);
    glVertex3fv(P026);
    glEnd();
    glBegin(GL_POLYGON);
    glNormal3fv(N015);
    glVertex3fv(P015);
    glNormal3fv(N066);
    glVertex3fv(P066);
    glNormal3fv(N025);
    glVertex3fv(P025);
    glEnd();
    glBegin(GL_POLYGON);
    glNormal3fv(N025);
    glVertex3fv(P025);
    glNormal3fv(N066);
    glVertex3fv(P066);
    glNormal3fv(N026);
    glVertex3fv(P026);
    glEnd();
    glBegin(GL_POLYGON);
    glNormal3fv(N066);
    glVertex3fv(P066);
    glNormal3fv(N058);
    glVertex3fv(P058);
    glNormal3fv(N016);
    glVertex3fv(P016);
    glEnd();
    glBegin(GL_POLYGON);
    glNormal3fv(N015);
    glVertex3fv(P015);
    glNormal3fv(N058);
    glVertex3fv(P058);
    glNormal3fv(N066);
    glVertex3fv(P066);
    glEnd();
    glBegin(GL_POLYGON);
    glNormal3fv(N058);
    glVertex3fv(P058);
    glNormal3fv(N015);
    glVertex3fv(P015);
    glNormal3fv(N016);
    glVertex3fv(P016);
    glEnd();
}

void
Fish007(void)
{
    glBegin(GL_POLYGON);
    glNormal3fv(N062);
    glVertex3fv(P062);
    glNormal3fv(N022);
    glVertex3fv(P022);
    glNormal3fv(N032);
    glVertex3fv(P032);
    glEnd();
    glBegin(GL_POLYGON);
    glNormal3fv(N062);
    glVertex3fv(P062);
    glNormal3fv(N032);
    glVertex3fv(P032);
    glNormal3fv(N064);
    glVertex3fv(P064);
    glEnd();
    glBegin(GL_POLYGON);
    glNormal3fv(N022);
    glVertex3fv(P022);
    glNormal3fv(N062);
    glVertex3fv(P062);
    glNormal3fv(N032);
    glVertex3fv(P032);
    glEnd();
    glBegin(GL_POLYGON);
    glNormal3fv(N062);
    glVertex3fv(P062);
    glNormal3fv(N064);
    glVertex3fv(P064);
    glNormal3fv(N032);
    glVertex3fv(P032);
    glEnd();
}

void
Fish008(void)
{
    glBegin(GL_POLYGON);
    glNormal3fv(N063);
    glVertex3fv(P063);
    glNormal3fv(N019);
    glVertex3fv(P019);
    glNormal3fv(N029);
    glVertex3fv(P029);
    glEnd();
    glBegin(GL_POLYGON);
    glNormal3fv(N019);
    glVertex3fv(P019);
    glNormal3fv(N063);
    glVertex3fv(P063);
    glNormal3fv(N029);
    glVertex3fv(P029);
    glEnd();
    glBegin(GL_POLYGON);
    glNormal3fv(N063);
    glVertex3fv(P063);
    glNormal3fv(N029);
    glVertex3fv(P029);
    glNormal3fv(N065);
    glVertex3fv(P065);
    glEnd();
    glBegin(GL_POLYGON);
    glNormal3fv(N063);
    glVertex3fv(P063);
    glNormal3fv(N065);
    glVertex3fv(P065);
    glNormal3fv(N029);
    glVertex3fv(P029);
    glEnd();
}

void
Fish009(void)
{
    glBegin(GL_POLYGON);
    glVertex3fv(P059);
    glVertex3fv(P012);
    glVertex3fv(P009);
    glVertex3fv(P060);
    glEnd();
    glBegin(GL_POLYGON);
    glVertex3fv(P012);
    glVertex3fv(P004);
    glVertex3fv(P007);
    glVertex3fv(P009);
    glEnd();
}

void
Fish_1(void)
{
    Fish004();
    Fish005();
    Fish003();
    Fish007();
    Fish006();
    Fish002();
    Fish008();
    Fish009();
    Fish001();
}

void
Fish_2(void)
{
    Fish005();
    Fish004();
    Fish003();
    Fish008();
    Fish006();
    Fish002();
    Fish007();
    Fish009();
    Fish001();
}

void
Fish_3(void)
{
    Fish005();
    Fish004();
    Fish007();
    Fish003();
    Fish002();
    Fish008();
    Fish009();
    Fish001();
    Fish006();
}

void
Fish_4(void)
{
    Fish005();
    Fish004();
    Fish008();
    Fish003();
    Fish002();
    Fish007();
    Fish009();
    Fish001();
    Fish006();
}

void
Fish_5(void)
{
    Fish009();
    Fish006();
    Fish007();
    Fish001();
    Fish002();
    Fish003();
    Fish008();
    Fish004();
    Fish005();
}

void
Fish_6(void)
{
    Fish009();
    Fish006();
    Fish008();
    Fish001();
    Fish002();
    Fish007();
    Fish003();
    Fish004();
    Fish005();
}

void
Fish_7(void)
{
    Fish009();
    Fish001();
    Fish007();
    Fish005();
    Fish002();
    Fish008();
    Fish003();
    Fish004();
    Fish006();
}

void
Fish_8(void)
{
    Fish009();
    Fish008();
    Fish001();
    Fish002();
    Fish007();
    Fish003();
    Fish005();
    Fish004();
    Fish006();
}

void
DrawShark(fishRec * fish)
{
    float mat[4][4];
    int n;
    float seg1, seg2, seg3, seg4, segup;
    float thrash, chomp;

    fish->htail = (int) (fish->htail - (int) (5.0 * fish->v)) % 360;

    thrash = 50.0 * fish->v;

    seg1 = 0.6 * thrash * sin(fish->htail * RRAD);
    seg2 = 1.8 * thrash * sin((fish->htail + 45.0) * RRAD);
    seg3 = 3.0 * thrash * sin((fish->htail + 90.0) * RRAD);
    seg4 = 4.0 * thrash * sin((fish->htail + 110.0) * RRAD);

    chomp = 0.0;
    if (fish->v > 2.0) {
        chomp = -(fish->v - 2.0) * 200.0;
    }
    P004[1] = iP004[1] + chomp;
    P007[1] = iP007[1] + chomp;
    P010[1] = iP010[1] + chomp;
    P011[1] = iP011[1] + chomp;

    P023[0] = iP023[0] + seg1;
    P024[0] = iP024[0] + seg1;
    P025[0] = iP025[0] + seg1;
    P026[0] = iP026[0] + seg1;
    P027[0] = iP027[0] + seg1;
    P028[0] = iP028[0] + seg1;
    P029[0] = iP029[0] + seg1;
    P030[0] = iP030[0] + seg1;
    P031[0] = iP031[0] + seg1;
    P032[0] = iP032[0] + seg1;
    P033[0] = iP033[0] + seg2;
    P034[0] = iP034[0] + seg2;
    P035[0] = iP035[0] + seg2;
    P036[0] = iP036[0] + seg2;
    P037[0] = iP037[0] + seg2;
    P038[0] = iP038[0] + seg2;
    P039[0] = iP039[0] + seg2;
    P040[0] = iP040[0] + seg2;
    P041[0] = iP041[0] + seg2;
    P042[0] = iP042[0] + seg2;
    P043[0] = iP043[0] + seg3;
    P044[0] = iP044[0] + seg3;
    P045[0] = iP045[0] + seg3;
    P046[0] = iP046[0] + seg3;
    P047[0] = iP047[0] + seg3;
    P048[0] = iP048[0] + seg3;
    P049[0] = iP049[0] + seg3;
    P050[0] = iP050[0] + seg3;
    P051[0] = iP051[0] + seg3;
    P052[0] = iP052[0] + seg3;
    P002[0] = iP002[0] + seg4;
    P061[0] = iP061[0] + seg4;
    P069[0] = iP069[0] + seg4;
    P070[0] = iP070[0] + seg4;

    fish->vtail += ((fish->dtheta - fish->vtail) * 0.1);

    if (fish->vtail > 0.5) {
        fish->vtail = 0.5;
    } else if (fish->vtail < -0.5) {
        fish->vtail = -0.5;
    }
    segup = thrash * fish->vtail;

    P023[1] = iP023[1] + segup;
    P024[1] = iP024[1] + segup;
    P025[1] = iP025[1] + segup;
    P026[1] = iP026[1] + segup;
    P027[1] = iP027[1] + segup;
    P028[1] = iP028[1] + segup;
    P029[1] = iP029[1] + segup;
    P030[1] = iP030[1] + segup;
    P031[1] = iP031[1] + segup;
    P032[1] = iP032[1] + segup;
    P033[1] = iP033[1] + segup * 5.0;
    P034[1] = iP034[1] + segup * 5.0;
    P035[1] = iP035[1] + segup * 5.0;
    P036[1] = iP036[1] + segup * 5.0;
    P037[1] = iP037[1] + segup * 5.0;
    P038[1] = iP038[1] + segup * 5.0;
    P039[1] = iP039[1] + segup * 5.0;
    P040[1] = iP040[1] + segup * 5.0;
    P041[1] = iP041[1] + segup * 5.0;
    P042[1] = iP042[1] + segup * 5.0;
    P043[1] = iP043[1] + segup * 12.0;
    P044[1] = iP044[1] + segup * 12.0;
    P045[1] = iP045[1] + segup * 12.0;
    P046[1] = iP046[1] + segup * 12.0;
    P047[1] = iP047[1] + segup * 12.0;
    P048[1] = iP048[1] + segup * 12.0;
    P049[1] = iP049[1] + segup * 12.0;
    P050[1] = iP050[1] + segup * 12.0;
    P051[1] = iP051[1] + segup * 12.0;
    P052[1] = iP052[1] + segup * 12.0;
    P002[1] = iP002[1] + segup * 17.0;
    P061[1] = iP061[1] + segup * 17.0;
    P069[1] = iP069[1] + segup * 17.0;
    P070[1] = iP070[1] + segup * 17.0;

    glPushMatrix();

    glTranslatef(0.0, 0.0, -3000.0);

    glGetFloatv(GL_MODELVIEW_MATRIX, &mat[0][0]);
    n = 0;
    if (mat[0][2] >= 0.0) {
        n += 1;
    }
    if (mat[1][2] >= 0.0) {
        n += 2;
    }
    if (mat[2][2] >= 0.0) {
        n += 4;
    }
    glScalef(2.0, 1.0, 1.0);

    glEnable(GL_CULL_FACE);
    switch (n) {
    case 0:
        Fish_1();
        break;
    case 1:
        Fish_2();
        break;
    case 2:
        Fish_3();
        break;
    case 3:
        Fish_4();
        break;
    case 4:
        Fish_5();
        break;
    case 5:
        Fish_6();
        break;
    case 6:
        Fish_7();
        break;
    case 7:
        Fish_8();
        break;
    }
    glDisable(GL_CULL_FACE);

    glPopMatrix();
}
