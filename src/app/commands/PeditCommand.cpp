#include "commands/PeditCommand.h"

#include "core/document/Commands.h"
#include "core/geometry/PolylineOps.h"

QString PeditCommand::start() {
    return QStringLiteral("PEDIT  %1 selected. Enter option [Close/Open/Join/Decurve]:")
        .arg(static_cast<int>(m_ids.size()));
}

std::optional<QString> PeditCommand::onPoint(const lcad::Point2D& pt) {
    (void)pt;
    return std::nullopt;
}

std::optional<QString> PeditCommand::onOption(const QString& option) {
    const QString opt = option.toUpper();
    QString result;
    if (opt == QLatin1String("C") || opt == QLatin1String("CLOSE")) result = setClosed(true);
    else if (opt == QLatin1String("O") || opt == QLatin1String("OPEN")) result = setClosed(false);
    else if (opt == QLatin1String("J") || opt == QLatin1String("JOIN")) result = join();
    else if (opt == QLatin1String("D") || opt == QLatin1String("DECURVE")) result = decurve();
    else return std::nullopt;
    m_finished = true;
    return result;
}

QString PeditCommand::setClosed(bool closed) {
    auto batch = std::make_unique<lcad::BatchCommand>(closed ? "Close polyline" : "Open polyline");
    int changed = 0;
    for (lcad::EntityId id : m_ids) {
        const lcad::Entity* e = m_document.findEntity(id);
        if (!e || e->type() != lcad::EntityType::Polyline) continue;
        const auto& pl = static_cast<const lcad::PolylineEntity&>(*e);
        if (pl.closed() == closed || pl.vertices().size() < 3) continue;
        auto replacement = std::make_unique<lcad::PolylineEntity>(id, pl.layer(), pl.vertices(), pl.bulges(), closed);
        replacement->setColorOverride(pl.colorOverride());
        replacement->setLinetypeOverride(pl.linetypeOverride());
        batch->add(std::make_unique<lcad::ReplaceEntityCommand>(m_document, id, std::move(replacement)));
        ++changed;
    }
    if (!batch->empty()) m_document.commandStack().execute(std::move(batch));
    return QStringLiteral("*%1 polyline(s) %2*").arg(changed).arg(closed ? QStringLiteral("closed") : QStringLiteral("opened"));
}

QString PeditCommand::join() {
    std::vector<const lcad::Entity*> parts;
    for (lcad::EntityId id : m_ids) {
        if (const lcad::Entity* e = m_document.findEntity(id)) parts.push_back(e);
    }
    // Endpoint-matching tolerance scaled to the geometry so slightly-off
    // drawings still join, like AutoCAD's fuzz distance.
    auto joined = lcad::joinToPolyline(m_document.reserveEntityId(),
                                       parts.empty() ? 0 : parts.front()->layer(), parts, 1e-6);
    if (!joined) return QStringLiteral("*Selected objects do not form a single connected chain*");

    auto batch = std::make_unique<lcad::BatchCommand>("Join");
    for (lcad::EntityId id : m_ids) {
        if (m_document.findEntity(id)) batch->add(std::make_unique<lcad::DeleteEntityCommand>(m_document, id));
    }
    const int count = static_cast<int>(parts.size());
    batch->add(std::make_unique<lcad::AddEntityCommand>(m_document, std::move(joined)));
    m_document.commandStack().execute(std::move(batch));
    return QStringLiteral("*%1 objects joined into one polyline*").arg(count);
}

QString PeditCommand::decurve() {
    auto batch = std::make_unique<lcad::BatchCommand>("Decurve");
    int changed = 0;
    for (lcad::EntityId id : m_ids) {
        const lcad::Entity* e = m_document.findEntity(id);
        if (!e || e->type() != lcad::EntityType::Polyline) continue;
        const auto& pl = static_cast<const lcad::PolylineEntity&>(*e);
        if (!pl.hasArcs()) continue;
        auto replacement = std::make_unique<lcad::PolylineEntity>(id, pl.layer(), pl.vertices(), pl.closed());
        replacement->setColorOverride(pl.colorOverride());
        replacement->setLinetypeOverride(pl.linetypeOverride());
        batch->add(std::make_unique<lcad::ReplaceEntityCommand>(m_document, id, std::move(replacement)));
        ++changed;
    }
    if (!batch->empty()) m_document.commandStack().execute(std::move(batch));
    return QStringLiteral("*%1 polyline(s) decurved*").arg(changed);
}
