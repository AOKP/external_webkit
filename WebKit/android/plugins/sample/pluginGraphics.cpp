/*
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
 
#include "pluginGraphics.h"

#include "android_npapi.h"
#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <math.h>
#include <string.h>

extern NPNetscapeFuncs*        browser;
extern ANPLogInterfaceV0       gLogI;
extern ANPCanvasInterfaceV0    gCanvasI;
extern ANPPaintInterfaceV0     gPaintI;
extern ANPTypefaceInterfaceV0  gTypefaceI;

static void inval(NPP instance) {
    browser->invalidaterect(instance, NULL);
}

static uint16 rnd16(float x, int inset) {
    int ix = (int)roundf(x) + inset;
    if (ix < 0) {
        ix = 0;
    }
    return static_cast<uint16>(ix);
}

static void inval(NPP instance, const ANPRectF& r, bool doAA) {
    const int inset = doAA ? -1 : 0;

    PluginObject *obj = reinterpret_cast<PluginObject*>(instance->pdata);
    NPRect inval;
    inval.left = rnd16(r.left, inset);
    inval.top = rnd16(r.top, inset);
    inval.right = rnd16(r.right, -inset);
    inval.bottom = rnd16(r.bottom, -inset);
    browser->invalidaterect(instance, &inval);
}

uint32_t getMSecs() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint32_t) (tv.tv_sec * 1000 + tv.tv_usec / 1000 ); // microseconds to milliseconds
}

///////////////////////////////////////////////////////////////////////////////

class BallAnimation : public Animation {
public:
    BallAnimation(NPP inst);
    virtual ~BallAnimation();
    virtual void draw(ANPCanvas*);
private:
    float m_x;
    float m_y;
    float m_dx;
    float m_dy;
    
    ANPRectF    m_oval;
    ANPPaint*   m_paint;
    
    static const float SCALE = 0.1;
};

BallAnimation::BallAnimation(NPP inst) : Animation(inst) {
    m_x = m_y = 0;
    m_dx = 7 * SCALE;
    m_dy = 5 * SCALE;
    
    memset(&m_oval, 0, sizeof(m_oval));

    m_paint = gPaintI.newPaint();
    gPaintI.setFlags(m_paint, gPaintI.getFlags(m_paint) | kAntiAlias_ANPPaintFlag);
    gPaintI.setColor(m_paint, 0xFFFF0000);
    gPaintI.setTextSize(m_paint, 24);
    
    ANPTypeface* tf = gTypefaceI.createFromName("serif", kItalic_ANPTypefaceStyle);
    gPaintI.setTypeface(m_paint, tf);
    gTypefaceI.unref(tf);
}

BallAnimation::~BallAnimation() {
    gPaintI.deletePaint(m_paint);
}

static void bounce(float* x, float* dx, const float max) {
    *x += *dx;
    if (*x < 0) {
        *x = 0;
        if (*dx < 0) {
            *dx = -*dx;
        }
    } else if (*x > max) {
        *x = max;
        if (*dx > 0) {
            *dx = -*dx;
        }
    }
}

void BallAnimation::draw(ANPCanvas* canvas) {
    NPP instance = this->inst();
    PluginObject *obj = (PluginObject*) instance->pdata;
    const float OW = 20;
    const float OH = 20;

    inval(instance, m_oval, true);  // inval the old
    m_oval.left = m_x;
    m_oval.top = m_y;
    m_oval.right = m_x + OW;
    m_oval.bottom = m_y + OH;
    inval(instance, m_oval, true);  // inval the new
    
    gCanvasI.drawColor(canvas, 0xFFFFFFFF);

    gPaintI.setColor(m_paint, 0xFFFF0000);
    gCanvasI.drawOval(canvas, &m_oval, m_paint);
    
    bounce(&m_x, &m_dx, obj->window->width - OW);
    bounce(&m_y, &m_dy, obj->window->height - OH);
    
    if (obj->mUnichar) {
        gPaintI.setColor(m_paint, 0xFF0000FF);
        char c = static_cast<char>(obj->mUnichar);
        gCanvasI.drawText(canvas, &c, 1, 10, 30, m_paint);
    }
}

///////////////////////////////////////////////////////////////////////////////

void drawPlugin(NPP instance, const ANPBitmap& bitmap, const ANPRectI& clip) {
    ANPCanvas* canvas = gCanvasI.newCanvas(&bitmap);
    
    ANPRectF clipR;
    clipR.left = clip.left;
    clipR.top = clip.top;
    clipR.right = clip.right;
    clipR.bottom = clip.bottom;
    gCanvasI.clipRect(canvas, &clipR);
    
    drawPlugin(instance, canvas);
    
    gCanvasI.deleteCanvas(canvas);
}

void drawPlugin(NPP instance, ANPCanvas* canvas) {
    PluginObject *obj = (PluginObject*) instance->pdata;    
    if (obj->anim == NULL) {
        obj->anim = new BallAnimation(instance);
    }
    obj->anim->draw(canvas);
}

