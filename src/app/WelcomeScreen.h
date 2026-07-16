#pragma once

#include <QDialog>
#include <QString>

class QListWidget;

// Startup dialog: lets the user pick what kind of document to start
// (2D drafting is the only mode KumCAD actually implements today; 3D and
// PCB are shown as visible, disabled placeholders for future modes rather
// than hidden, so the app's intended direction is honest up front) or open
// an existing/recent drawing.
class WelcomeScreen : public QDialog {
    Q_OBJECT
public:
    enum class Choice { NewDrawing, OpenExisting };

    explicit WelcomeScreen(QWidget* parent = nullptr);

    Choice choice() const { return m_choice; }
    // Valid only when choice() == OpenExisting.
    QString selectedPath() const { return m_selectedPath; }

private:
    void refreshRecentList();
    void pickNewDrawing();
    void pickOpenExisting();
    void pickRecentFile(const QString& path);
    void showComingSoon(const QString& modeName);

    QListWidget* m_recentList = nullptr;
    Choice m_choice = Choice::NewDrawing;
    QString m_selectedPath;
};
