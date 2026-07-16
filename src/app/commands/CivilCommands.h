#pragma once

#include "commands/DrawCommand.h"
#include "core/document/Document.h"

#include <vector>

// TIN: survey XYZ file path -- builds a TIN (see core/civil/Tin.h) and
// reports its point/triangle count. Informational only; doesn't add
// anything to the drawing (CONTOUR/PROFILE/CUTFILL each build their own
// TIN internally from a file path, rather than sharing one cached here).
class TinInfoCommand : public DrawCommand {
public:
    explicit TinInfoCommand(lcad::Document& document) : m_document(document) {}

    QString start() override { return QStringLiteral("TIN  Enter survey XYZ file path:"); }
    std::optional<QString> onPoint(const lcad::Point2D& pt) override {
        (void)pt;
        return std::nullopt;
    }
    bool wantsTextInput() const override { return true; }
    std::optional<QString> onText(const QString& text) override;
    bool isFinished() const override { return m_finished; }
    void cancel() override { m_finished = true; }

private:
    lcad::Document& m_document;
    bool m_finished = false;
};

// CONTOUR: survey XYZ file path, then contour interval -- builds a TIN and
// adds one PolylineEntity per contour chain (see core/civil/Contours.h) to
// the document at their real survey coordinates.
class ContourCommand : public DrawCommand {
public:
    explicit ContourCommand(lcad::Document& document) : m_document(document) {}

    QString start() override { return QStringLiteral("CONTOUR  Enter survey XYZ file path:"); }
    std::optional<QString> onPoint(const lcad::Point2D& pt) override {
        (void)pt;
        return std::nullopt;
    }
    bool wantsTextInput() const override { return true; }
    std::optional<QString> onText(const QString& text) override;
    bool isFinished() const override { return m_finished; }
    void cancel() override { m_finished = true; }

private:
    enum class Stage { Path, Interval };
    lcad::Document& m_document;
    Stage m_stage = Stage::Path;
    std::string m_path;
    bool m_finished = false;
};

// CUTFILL: existing-surface file path, proposed-surface file path, sample
// cell size -- reports estimated cut/fill volume (see core/civil/CutFill.h).
class CutFillCommand : public DrawCommand {
public:
    explicit CutFillCommand(lcad::Document& document) : m_document(document) {}

    QString start() override { return QStringLiteral("CUTFILL  Enter existing-surface XYZ file path:"); }
    std::optional<QString> onPoint(const lcad::Point2D& pt) override {
        (void)pt;
        return std::nullopt;
    }
    bool wantsTextInput() const override { return true; }
    std::optional<QString> onText(const QString& text) override;
    bool isFinished() const override { return m_finished; }
    void cancel() override { m_finished = true; }

private:
    enum class Stage { ExistingPath, ProposedPath, CellSize };
    lcad::Document& m_document;
    Stage m_stage = Stage::ExistingPath;
    std::string m_existingPath;
    std::string m_proposedPath;
    bool m_finished = false;
};

// PROFILE: click the alignment's points (like WIRE, Enter to finish), then
// a survey XYZ file path and sample interval, then an insertion point --
// adds one PolylineEntity in profile space (x = station, y = elevation;
// see core/civil/Alignment.h) at that insertion point.
class ProfileCommand : public DrawCommand {
public:
    explicit ProfileCommand(lcad::Document& document) : m_document(document) {}

    QString start() override;
    std::optional<QString> onPoint(const lcad::Point2D& pt) override;
    bool requestFinish() override;
    bool wantsTextInput() const override { return m_stage == Stage::Path || m_stage == Stage::Interval; }
    std::optional<QString> onText(const QString& text) override;
    std::optional<lcad::Point2D> anchorPoint() const override {
        return m_alignment.empty() ? std::nullopt : std::optional<lcad::Point2D>(m_alignment.back());
    }
    bool isFinished() const override { return m_finished; }
    void cancel() override { m_finished = true; }

private:
    enum class Stage { Alignment, Path, Interval, InsertionPoint };
    lcad::Document& m_document;
    Stage m_stage = Stage::Alignment;
    std::vector<lcad::Point2D> m_alignment;
    std::string m_path;
    double m_interval = 1.0;
    bool m_finished = false;
};
