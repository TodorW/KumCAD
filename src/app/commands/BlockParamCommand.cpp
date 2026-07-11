#include "commands/BlockParamCommand.h"

#include <algorithm>

std::optional<QString> BlockParamCommand::onText(const QString& text) {
    if (m_stage != Stage::BlockName) return std::nullopt;
    const std::string name = text.trimmed().toStdString();
    m_block = m_document.findBlock(name);
    if (!m_block) {
        return QStringLiteral("*Block \"%1\" not found*\nEnter block name to make dynamic:")
            .arg(QString::fromStdString(name));
    }
    m_stage = Stage::BasePoint;
    return QStringLiteral("Specify parameter base point:");
}

std::optional<QString> BlockParamCommand::onPoint(const lcad::Point2D& pt) {
    switch (m_stage) {
    case Stage::BasePoint:
        m_basePoint = pt;
        m_stage = Stage::EndPoint;
        return QStringLiteral("Specify parameter endpoint:");
    case Stage::EndPoint:
        if (pt.distanceTo(m_basePoint) < 1e-9) return QStringLiteral("*Endpoint can't match the base point*");
        m_endPoint = pt;
        m_stage = Stage::FrameCorner1;
        return QStringLiteral("Specify first corner of stretch frame:");
    case Stage::FrameCorner1:
        m_frameCorner1 = pt;
        m_stage = Stage::FrameCorner2;
        return QStringLiteral("Specify opposite corner of stretch frame:");
    case Stage::FrameCorner2: {
        lcad::DynamicLinearParameter dp;
        dp.basePoint = m_basePoint;
        dp.endPoint = m_endPoint;
        dp.frameMin = lcad::Point2D(std::min(m_frameCorner1.x, pt.x), std::min(m_frameCorner1.y, pt.y));
        dp.frameMax = lcad::Point2D(std::max(m_frameCorner1.x, pt.x), std::max(m_frameCorner1.y, pt.y));
        m_block->dynamicParam = dp;
        m_finished = true;
        return QStringLiteral("*Block \"%1\" is now dynamic (linear stretch parameter)*")
            .arg(QString::fromStdString(m_block->name));
    }
    default:
        return std::nullopt;
    }
}
