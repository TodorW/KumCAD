#include "commands/GradientCommand.h"

#include "commands/HatchBoundary.h"
#include "core/document/Commands.h"
#include "core/geometry/Polyline.h"
#include "core/io/DxfColors.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace {
const std::array<lcad::GradientPreset, 9> kPresetOrder{
    lcad::GradientPreset::Linear,          lcad::GradientPreset::Cylinder,          lcad::GradientPreset::InvCylinder,
    lcad::GradientPreset::Spherical,       lcad::GradientPreset::InvSpherical,      lcad::GradientPreset::Hemispherical,
    lcad::GradientPreset::InvHemispherical, lcad::GradientPreset::Curved,            lcad::GradientPreset::InvCurved,
};
} // namespace

QString GradientCommand::start() {
    if (m_stage == Stage::BoundaryPick) return QStringLiteral("GRADIENT  Pick internal point:");
    return QStringLiteral("GRADIENT  %1").arg(presetPrompt());
}

QString GradientCommand::presetPrompt() const {
    QString list;
    for (std::size_t i = 0; i < kPresetOrder.size(); ++i) {
        if (i) list += QStringLiteral("/");
        list += QStringLiteral("%1:%2").arg(i + 1).arg(QLatin1String(lcad::gradientPresetName(kPresetOrder[i])));
    }
    return QStringLiteral("Select gradient [%1] <1:LINEAR>:").arg(list);
}

std::optional<QString> GradientCommand::onPoint(const lcad::Point2D& pt) {
    if (m_stage != Stage::BoundaryPick) return std::nullopt;
    if (auto boundary = pickHatchBoundary(m_document, pt)) {
        m_pickedBoundaries.push_back(std::move(*boundary));
        return QStringLiteral("*%1 boundary(ies) found* Pick internal point (Enter to finish):")
            .arg(m_pickedBoundaries.size());
    }
    return QStringLiteral("*Valid boundary not found there*\nPick internal point:");
}

std::optional<QString> GradientCommand::onScalar(double value) {
    switch (m_stage) {
    case Stage::Preset: {
        const int idx = static_cast<int>(value) - 1;
        if (idx < 0 || idx >= static_cast<int>(kPresetOrder.size())) {
            return QStringLiteral("*Enter 1-%1 or a preset name*\n%2").arg(kPresetOrder.size()).arg(presetPrompt());
        }
        m_preset = kPresetOrder[static_cast<std::size_t>(idx)];
        m_stage = Stage::OneColorChoice;
        return QStringLiteral("One color? [Yes/No] <No>:");
    }
    case Stage::Color1:
        if (value < 1 || value > 255) return QStringLiteral("*Color must be an ACI number 1-255*");
        m_aci1 = static_cast<int>(value);
        if (m_oneColor) {
            m_stage = Stage::Tint;
            return QStringLiteral("Specify tint percent, 0=color 100=white <50>:");
        }
        m_stage = Stage::Color2;
        return QStringLiteral("Enter second color [ACI 1-255] <150>:");
    case Stage::Color2:
        if (value < 1 || value > 255) return QStringLiteral("*Color must be an ACI number 1-255*");
        m_aci2 = static_cast<int>(value);
        m_stage = Stage::Angle;
        return QStringLiteral("Specify gradient angle <0>:");
    case Stage::Tint:
        m_tintPercent = std::clamp(value, 0.0, 100.0);
        m_stage = Stage::Angle;
        return QStringLiteral("Specify gradient angle <0>:");
    case Stage::Angle:
        m_angleDeg = value;
        commit();
        m_finished = true;
        return m_result;
    default:
        return std::nullopt;
    }
}

std::optional<QString> GradientCommand::onOption(const QString& option) {
    switch (m_stage) {
    case Stage::Preset: {
        if (const auto preset = lcad::gradientPresetFromName(option.trimmed().toUpper().toStdString())) {
            m_preset = *preset;
            m_stage = Stage::OneColorChoice;
            return QStringLiteral("One color? [Yes/No] <No>:");
        }
        return QStringLiteral("*Unknown preset*\n%1").arg(presetPrompt());
    }
    case Stage::OneColorChoice: {
        const QString upper = option.trimmed().toUpper();
        if (upper == QLatin1String("Y") || upper == QLatin1String("YES")) {
            m_oneColor = true;
        } else if (upper == QLatin1String("N") || upper == QLatin1String("NO")) {
            m_oneColor = false;
        } else {
            return QStringLiteral("*Enter Yes or No*");
        }
        m_stage = Stage::Color1;
        return m_oneColor ? QStringLiteral("Enter color [ACI 1-255] <5>:")
                          : QStringLiteral("Enter first color [ACI 1-255] <5>:");
    }
    default:
        return std::nullopt;
    }
}

bool GradientCommand::requestFinish() {
    if (m_stage == Stage::BoundaryPick) {
        if (m_pickedBoundaries.empty()) {
            m_finished = true;
            return false;
        }
        m_stage = Stage::Preset;
        return true; // not finished yet -- resultMessage() gives the preset prompt
    }
    commit();
    m_finished = true;
    return true;
}

void GradientCommand::commit() {
    auto batch = std::make_unique<lcad::BatchCommand>("Gradient");
    int made = 0;
    int skipped = 0;
    const lcad::Color color1 = lcad::aciToColor(m_aci1);
    lcad::Color color2;
    if (m_oneColor) {
        // AutoCAD's one-color gradient blends toward white by the tint
        // percentage (its "Shade/Tint" slider); the shade-toward-black case
        // for very light base colors isn't modeled.
        const double t = std::clamp(m_tintPercent / 100.0, 0.0, 1.0);
        const auto blend = [t](std::uint8_t c) {
            return static_cast<std::uint8_t>(std::lround(c + (255 - c) * t));
        };
        color2 = lcad::Color{blend(color1.r), blend(color1.g), blend(color1.b)};
    } else {
        color2 = lcad::aciToColor(m_aci2);
    }
    const double angleRad = m_angleDeg * M_PI / 180.0;
    const lcad::GradientPreset preset = m_preset;

    auto addGradient = [&](std::vector<lcad::Point2D> vertices, lcad::LayerId layer) {
        auto hatch = std::make_unique<lcad::HatchEntity>(m_document.reserveEntityId(), layer, std::move(vertices),
                                                          lcad::HatchPattern::Solid, 1.0, angleRad);
        hatch->setColorOverride(color1);
        hatch->setGradientColor2(color2);
        hatch->setGradientPreset(preset);
        batch->add(std::make_unique<lcad::AddEntityCommand>(m_document, std::move(hatch)));
        ++made;
    };

    for (lcad::EntityId id : m_ids) {
        const lcad::Entity* e = m_document.findEntity(id);
        if (e && e->type() == lcad::EntityType::Polyline) {
            const auto& pl = static_cast<const lcad::PolylineEntity&>(*e);
            if (pl.closed() && pl.vertices().size() >= 3) {
                addGradient(pl.flattenedVertices(), e->layer());
                continue;
            }
        }
        ++skipped;
    }
    for (auto& boundary : m_pickedBoundaries) addGradient(std::move(boundary), m_document.currentLayer());

    if (!batch->empty()) m_document.commandStack().execute(std::move(batch));
    m_result = skipped > 0
                   ? QStringLiteral("*%1 filled, %2 skipped (only closed polylines can be filled)*").arg(made).arg(skipped)
                   : QStringLiteral("*%1 filled*").arg(made);
}
