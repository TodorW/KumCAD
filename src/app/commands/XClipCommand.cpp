#include "commands/XClipCommand.h"

#include "core/document/Commands.h"
#include "core/geometry/Insert.h"

XClipCommand::XClipCommand(lcad::Document& document, lcad::EntityId targetId)
    : m_document(document), m_targetId(targetId) {}

QString XClipCommand::start() {
    return QStringLiteral("XCLIP  1 found. Enter clipping option [ON/OFF/Delete/New boundary] <New>:");
}

std::optional<QString> XClipCommand::onPoint(const lcad::Point2D& pt) {
    if (m_stage == Stage::RectFirst) {
        m_rectFirst = pt;
        m_stage = Stage::RectSecond;
        return QStringLiteral("Specify opposite corner:");
    }
    if (m_stage == Stage::RectSecond) {
        const lcad::Point2D a = m_rectFirst;
        const lcad::Point2D b(pt.x, m_rectFirst.y);
        const lcad::Point2D c = pt;
        const lcad::Point2D d(m_rectFirst.x, pt.y);
        applyClip({a, b, c, d}, true, QStringLiteral("*Clip boundary set*"));
        return m_result;
    }
    if (m_stage == Stage::Polygon) {
        m_polyPoints.push_back(pt);
        return QStringLiteral("Specify next point or Enter to close (%1 so far):").arg(m_polyPoints.size());
    }
    return std::nullopt;
}

std::optional<QString> XClipCommand::onOption(const QString& option) {
    const QString upper = option.trimmed().toUpper();
    if (m_stage == Stage::Option) {
        const lcad::Entity* e = m_document.findEntity(m_targetId);
        std::vector<lcad::Point2D> existing;
        if (e && e->type() == lcad::EntityType::Insert) {
            existing = static_cast<const lcad::InsertEntity&>(*e).clipBoundary();
        }
        if (upper == QLatin1String("ON")) {
            applyClip(std::move(existing), true, QStringLiteral("*Clip turned on*"));
            return m_result;
        }
        if (upper == QLatin1String("OFF")) {
            applyClip(std::move(existing), false, QStringLiteral("*Clip turned off*"));
            return m_result;
        }
        if (upper == QLatin1String("DELETE") || upper == QLatin1String("D")) {
            applyClip({}, true, QStringLiteral("*Clip boundary deleted*"));
            return m_result;
        }
        if (upper == QLatin1String("NEW") || upper == QLatin1String("N")) {
            m_stage = Stage::BoundaryKind;
            return QStringLiteral("Specify clipping boundary [Polygonal/Rectangular] <Rectangular>:");
        }
        return std::nullopt;
    }
    if (m_stage == Stage::BoundaryKind) {
        if (upper == QLatin1String("POLYGONAL") || upper == QLatin1String("P")) {
            m_stage = Stage::Polygon;
            return QStringLiteral("Specify first polygon point:");
        }
        if (upper == QLatin1String("RECTANGULAR") || upper == QLatin1String("R")) {
            m_stage = Stage::RectFirst;
            return QStringLiteral("Specify first corner:");
        }
        return std::nullopt;
    }
    return std::nullopt;
}

bool XClipCommand::requestFinish() {
    if (m_stage == Stage::Option) {
        m_stage = Stage::BoundaryKind;
        m_result = QStringLiteral("Specify clipping boundary [Polygonal/Rectangular] <Rectangular>:");
        return true;
    }
    if (m_stage == Stage::BoundaryKind) {
        m_stage = Stage::RectFirst;
        m_result = QStringLiteral("Specify first corner:");
        return true;
    }
    if (m_stage == Stage::Polygon) {
        if (m_polyPoints.size() < 3) {
            finishWith(QStringLiteral("*At least 3 points are needed -- clip boundary not changed*"));
            return true;
        }
        applyClip(m_polyPoints, true, QStringLiteral("*Clip boundary set*"));
        return true;
    }
    m_finished = true;
    return true;
}

void XClipCommand::applyClip(std::vector<lcad::Point2D> boundary, bool enabled, const QString& message) {
    const lcad::Entity* e = m_document.findEntity(m_targetId);
    if (!e || e->type() != lcad::EntityType::Insert) {
        finishWith(QStringLiteral("*Target is no longer a valid block reference*"));
        return;
    }
    auto replacement = e->clone();
    static_cast<lcad::InsertEntity&>(*replacement).setClipBoundary(std::move(boundary));
    static_cast<lcad::InsertEntity&>(*replacement).setClipEnabled(enabled);
    m_document.commandStack().execute(
        std::make_unique<lcad::ReplaceEntityCommand>(m_document, m_targetId, std::move(replacement)));
    finishWith(message);
}

void XClipCommand::finishWith(const QString& message) {
    m_result = message;
    m_finished = true;
}
