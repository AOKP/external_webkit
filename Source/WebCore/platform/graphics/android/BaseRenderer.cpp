/*
 * Copyright 2011, The Android Open Source Project
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include "config.h"
#include "BaseRenderer.h"

#if USE(ACCELERATED_COMPOSITING)

#include "GLUtils.h"
#include "SkBitmap.h"
#include "SkBitmapRef.h"
#include "SkCanvas.h"
#include "SkDevice.h"
#include "SkPicture.h"
#include "TilesManager.h"

#include <wtf/text/CString.h>

#ifdef DEBUG

#include <cutils/log.h>
#include <wtf/CurrentTime.h>

#undef XLOG
#define XLOG(...) android_printLog(ANDROID_LOG_DEBUG, "BaseRenderer", __VA_ARGS__)

#else

#undef XLOG
#define XLOG(...)

#endif // DEBUG

namespace WebCore {

void BaseRenderer::drawTileInfo(SkCanvas* canvas,
        const TileRenderInfo& renderInfo, int pictureCount)
{
    SkPaint paint;
    char str[256];
    snprintf(str, 256, "(%d,%d) %.2f, tl%x p%x c%d", renderInfo.x, renderInfo.y,
            renderInfo.scale, this, renderInfo.tiledPage, pictureCount);
    paint.setARGB(255, 0, 0, 0);
    canvas->drawText(str, strlen(str), 0, 10, paint);
    paint.setARGB(255, 255, 0, 0);
    canvas->drawText(str, strlen(str), 0, 11, paint);

    int tagCount = 0;
    const String* tags = getPerformanceTags(tagCount);

    float total = 0;
    for (int i = 0; i < tagCount; i++) {
        float tagDuration = m_perfMon.getAverageDuration(tags[i]);
        total += tagDuration;
        snprintf(str, 256, "%s: %.2f", tags[i].utf8().data(), tagDuration);
        paint.setARGB(255, 0, 0, 0);
        int textY = (i * 12) + 25;
        canvas->drawText(str, strlen(str), 0, textY, paint);
        paint.setARGB(255, 255, 0, 0);
        canvas->drawText(str, strlen(str), 0, textY + 1, paint);
    }
    snprintf(str, 256, "total: %.2f", total);
    paint.setARGB(255, 0, 0, 0);
    int textY = (tagCount * 12) + 30;
    canvas->drawText(str, strlen(str), 0, textY, paint);
    paint.setARGB(255, 255, 0, 0);
    canvas->drawText(str, strlen(str), 0, textY + 1, paint);
}

int BaseRenderer::renderTiledContent(const TileRenderInfo& renderInfo)
{
    const bool visualIndicator = TilesManager::instance()->getShowVisualIndicator();
    const SkSize& tileSize = renderInfo.tileSize;

    SkCanvas canvas;
    setupCanvas(renderInfo, &canvas);

    if (visualIndicator)
        canvas.save();

    setupPartialInval(renderInfo, &canvas);
    canvas.translate(-renderInfo.x * tileSize.width(), -renderInfo.y * tileSize.height());
    canvas.scale(renderInfo.scale, renderInfo.scale);
    int pictureCount = renderInfo.tiledPage->paintBaseLayerContent(&canvas);

    if (visualIndicator) {
        canvas.restore();

        const int color = 20 + (pictureCount % 100);

        // only color the invalidated area
        SkPaint invalPaint;
        invalPaint.setARGB(color, 0, 255, 0);
        canvas.drawIRect(*renderInfo.invalRect, invalPaint);

        // paint the tile boundaries
        SkPaint paint;
        paint.setARGB(128, 255, 0, 0);
        paint.setStrokeWidth(3);
        canvas.drawLine(0, 0, tileSize.width(), tileSize.height(), paint);
        paint.setARGB(128, 0, 255, 0);
        canvas.drawLine(0, tileSize.height(), tileSize.width(), 0, paint);
        paint.setARGB(128, 0, 0, 255);
        canvas.drawLine(0, 0, tileSize.width(), 0, paint);
        canvas.drawLine(tileSize.width(), 0, tileSize.width(), tileSize.height(), paint);

        if (renderInfo.measurePerf)
            drawTileInfo(&canvas, renderInfo, pictureCount);
    }

    renderingComplete(renderInfo, &canvas);
    return pictureCount;
}

} // namespace WebCore

#endif // USE(ACCELERATED_COMPOSITING)
