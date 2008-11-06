/*  GSsoft
 *  Copyright (C) 2002-2005  GSsoft Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __SOFT_H__
#define __SOFT_H__

void drawLineF(Vertex *v);
void drawLineF_Z(Vertex *v);
void drawLineF_F(Vertex *v);
void drawLineF_ZF(Vertex *v);

void drawLineG(Vertex *v);
void drawLineG_Z(Vertex *v);
void drawLineG_F(Vertex *v);
void drawLineG_ZF(Vertex *v);

void drawTriangleF(Vertex *v);
void drawTriangleF_Z(Vertex *v);
void drawTriangleF_F(Vertex *v);
void drawTriangleF_ZF(Vertex *v);

void drawTriangleG(Vertex *v);
void drawTriangleG_Z(Vertex *v);
void drawTriangleG_F(Vertex *v);
void drawTriangleG_ZF(Vertex *v);

void drawTriangleFTDecal(Vertex *v);
void drawTriangleFTDecal_Z(Vertex * v);
void drawTriangleFTDecal_F(Vertex * v);
void drawTriangleFTDecal_ZF(Vertex * v);

void drawTriangleFTModulate(Vertex *v);
void drawTriangleFTModulate_Z(Vertex * v);
void drawTriangleFTModulate_F(Vertex * v);
void drawTriangleFTModulate_ZF(Vertex * v);

void drawTriangleFTHighlight(Vertex *v);
void drawTriangleFTHighlight_Z(Vertex * v);
void drawTriangleFTHighlight_F(Vertex * v);
void drawTriangleFTHighlight_ZF(Vertex * v);

void drawTriangleFTHighlight2(Vertex *v);
void drawTriangleFTHighlight2_Z(Vertex * v);
void drawTriangleFTHighlight2_F(Vertex * v);
void drawTriangleFTHighlight2_ZF(Vertex * v);

void drawTriangleGTDecal(Vertex *v);
void drawTriangleGTDecal_Z(Vertex * v);
void drawTriangleGTDecal_F(Vertex * v);
void drawTriangleGTDecal_ZF(Vertex * v);

void drawTriangleGTModulate(Vertex *v);
void drawTriangleGTModulate_Z(Vertex * v);
void drawTriangleGTModulate_F(Vertex * v);
void drawTriangleGTModulate_ZF(Vertex * v);

void drawTriangleGTHighlight(Vertex *v);
void drawTriangleGTHighlight_Z(Vertex * v);
void drawTriangleGTHighlight_F(Vertex * v);
void drawTriangleGTHighlight_ZF(Vertex * v);

void drawTriangleGTHighlight2(Vertex *v);
void drawTriangleGTHighlight2_Z(Vertex * v);
void drawTriangleGTHighlight2_F(Vertex * v);
void drawTriangleGTHighlight2_ZF(Vertex * v);

void drawSprite(Vertex *v);
void drawSprite_Z(Vertex *v);
void drawSprite_F(Vertex *v);
void drawSprite_ZF(Vertex *v);

void drawSpriteTDecal(Vertex *v);
void drawSpriteTDecal_Z(Vertex * v);
void drawSpriteTDecal_F(Vertex * v);
void drawSpriteTDecal_ZF(Vertex * v);

void drawSpriteTModulate(Vertex *v);
void drawSpriteTModulate_Z(Vertex * v);
void drawSpriteTModulate_F(Vertex * v);
void drawSpriteTModulate_ZF(Vertex * v);

void drawSpriteTHighlight(Vertex *v);
void drawSpriteTHighlight_Z(Vertex * v);
void drawSpriteTHighlight_F(Vertex * v);
void drawSpriteTHighlight_ZF(Vertex * v);

void drawSpriteTHighlight2(Vertex *v);
void drawSpriteTHighlight2_Z(Vertex * v);
void drawSpriteTHighlight2_F(Vertex * v);
void drawSpriteTHighlight2_ZF(Vertex * v);

#endif /* __SOFT_H__ */
