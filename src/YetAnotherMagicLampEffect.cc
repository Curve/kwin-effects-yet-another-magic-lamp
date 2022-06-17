/*
 * Copyright (C) 2018 Vlad Zagorodniy <vladzzag@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

// Own
#include "YetAnotherMagicLampEffect.h"
#include "Model.h"

// Auto-generated
#include "YetAnotherMagicLampConfig.h"

// std
#include <cmath>

enum ShapeCurve {
    Linear = 0,
    Quad = 1,
    Cubic = 2,
    Quart = 3,
    Quint = 4,
    Sine = 5,
    Circ = 6,
    Bounce = 7,
    Bezier = 8
};

YetAnotherMagicLampEffect::YetAnotherMagicLampEffect()
{
    reconfigure(ReconfigureAll);

    connect(KWin::effects, &KWin::EffectsHandler::windowMinimized,
        this, &YetAnotherMagicLampEffect::slotWindowMinimized);
    connect(KWin::effects, &KWin::EffectsHandler::windowUnminimized,
        this, &YetAnotherMagicLampEffect::slotWindowUnminimized);
    connect(KWin::effects, &KWin::EffectsHandler::windowDeleted,
        this, &YetAnotherMagicLampEffect::slotWindowDeleted);
    connect(KWin::effects, &KWin::EffectsHandler::activeFullScreenEffectChanged,
        this, &YetAnotherMagicLampEffect::slotActiveFullScreenEffectChanged);
}

YetAnotherMagicLampEffect::~YetAnotherMagicLampEffect()
{
}

void YetAnotherMagicLampEffect::reconfigure(ReconfigureFlags flags)
{
    Q_UNUSED(flags)

    YetAnotherMagicLampConfig::self()->read();

    QEasingCurve curve;
    const auto shapeCurve = static_cast<ShapeCurve>(YetAnotherMagicLampConfig::shapeCurve());
    switch (shapeCurve) {
    case ShapeCurve::Linear:
        curve.setType(QEasingCurve::Linear);
        break;

    case ShapeCurve::Quad:
        curve.setType(QEasingCurve::InOutQuad);
        break;

    case ShapeCurve::Cubic:
        curve.setType(QEasingCurve::InOutCubic);
        break;

    case ShapeCurve::Quart:
        curve.setType(QEasingCurve::InOutQuart);
        break;

    case ShapeCurve::Quint:
        curve.setType(QEasingCurve::InOutQuint);
        break;

    case ShapeCurve::Sine:
        curve.setType(QEasingCurve::InOutSine);
        break;

    case ShapeCurve::Circ:
        curve.setType(QEasingCurve::InOutCirc);
        break;

    case ShapeCurve::Bounce:
        curve.setType(QEasingCurve::InOutBounce);
        break;

    case ShapeCurve::Bezier:
        // With the cubic bezier curve, "0" corresponds to the furtherst edge
        // of a window, "1" corresponds to the closest edge.
        curve.setType(QEasingCurve::BezierSpline);
        curve.addCubicBezierSegment(
            QPointF(0.3, 0.0),
            QPointF(0.7, 1.0),
            QPointF(1.0, 1.0));
        break;
    default:
        // Fallback to the sine curve.
        curve.setType(QEasingCurve::InOutSine);
        break;
    }
    m_modelParameters.shapeCurve = curve;

    const int baseDuration = animationTime<YetAnotherMagicLampConfig>(300);
    m_modelParameters.squashDuration = std::chrono::milliseconds(baseDuration);
    m_modelParameters.stretchDuration = std::chrono::milliseconds(qMax(qRound(baseDuration * 0.7), 1));
    m_modelParameters.bumpDuration = std::chrono::milliseconds(baseDuration);
    m_modelParameters.shapeFactor = YetAnotherMagicLampConfig::initialShapeFactor();
    m_modelParameters.bumpDistance = YetAnotherMagicLampConfig::maxBumpDistance();

    m_gridResolution = YetAnotherMagicLampConfig::gridResolution();
}

void YetAnotherMagicLampEffect::prePaintScreen(KWin::ScreenPrePaintData& data, std::chrono::milliseconds presentTime)
{
    for (AnimationData& data : m_animationData) {
        data.model.advance(presentTime);
    }

    data.mask |= PAINT_SCREEN_WITH_TRANSFORMED_WINDOWS;

    KWin::effects->prePaintScreen(data, presentTime);
}

void YetAnotherMagicLampEffect::postPaintScreen()
{
    auto modelIt = m_animationData.begin();
    while (modelIt != m_animationData.end()) {
        if ((*modelIt).model.done()) {
            unredirect(modelIt.key());
            modelIt = m_animationData.erase(modelIt);
        } else {
            ++modelIt;
        }
    }

    KWin::effects->addRepaintFull();
    KWin::effects->postPaintScreen();
}

void YetAnotherMagicLampEffect::paintWindow(KWin::EffectWindow* w, int mask, QRegion region, KWin::WindowPaintData& data)
{
    QRegion clip = region;

    auto modelIt = m_animationData.constFind(w);
    if (modelIt != m_animationData.constEnd()) {
        if ((*modelIt).model.needsClip()) {
            clip = (*modelIt).model.clipRegion();
        }
    }

    KWin::effects->paintWindow(w, mask, clip, data);
}

void YetAnotherMagicLampEffect::deform(KWin::EffectWindow* window, int mask, KWin::WindowPaintData& data, KWin::WindowQuadList& quads)
{
    Q_UNUSED(mask)
    Q_UNUSED(data)

    auto modelIt = m_animationData.constFind(window);
    if (modelIt == m_animationData.constEnd()) {
        return;
    }

    quads = quads.makeGrid(m_gridResolution);
    (*modelIt).model.apply(quads);
}

bool YetAnotherMagicLampEffect::isActive() const
{
    return !m_animationData.isEmpty();
}

bool YetAnotherMagicLampEffect::supported()
{
    if (!KWin::effects->animationsSupported()) {
        return false;
    }
    if (KWin::effects->isOpenGLCompositing()) {
        return true;
    }
    return false;
}

void YetAnotherMagicLampEffect::slotWindowMinimized(KWin::EffectWindow* w)
{
    if (KWin::effects->activeFullScreenEffect()) {
        return;
    }

    const QRect iconRect = w->iconGeometry();
    if (!iconRect.isValid()) {
        return;
    }

    AnimationData& data = m_animationData[w];
    data.model.setWindow(w);
    data.model.setParameters(m_modelParameters);
    data.model.start(Model::AnimationKind::Minimize);
    data.visibleRef = KWin::EffectWindowVisibleRef(w, KWin::EffectWindow::PAINT_DISABLED_BY_MINIMIZE);

    redirect(w);

    KWin::effects->addRepaintFull();
}

void YetAnotherMagicLampEffect::slotWindowUnminimized(KWin::EffectWindow* w)
{
    if (KWin::effects->activeFullScreenEffect()) {
        return;
    }

    const QRect iconRect = w->iconGeometry();
    if (!iconRect.isValid()) {
        return;
    }

    AnimationData& data = m_animationData[w];
    data.model.setWindow(w);
    data.model.setParameters(m_modelParameters);
    data.model.start(Model::AnimationKind::Unminimize);

    redirect(w);

    KWin::effects->addRepaintFull();
}

void YetAnotherMagicLampEffect::slotWindowDeleted(KWin::EffectWindow* w)
{
    m_animationData.remove(w);
}

void YetAnotherMagicLampEffect::slotActiveFullScreenEffectChanged()
{
    if (KWin::effects->activeFullScreenEffect() != nullptr) {
        for (auto it = m_animationData.constBegin(); it != m_animationData.constEnd(); ++it) {
            unredirect(it.key());
        }
        m_animationData.clear();
    }
}
