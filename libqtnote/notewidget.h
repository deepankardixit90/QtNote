#ifndef NOTEWIDGET_H
#define NOTEWIDGET_H

#include <QWidget>
#include <QTimer>
#include <QElapsedTimer>

#include "highlighterext.h"

namespace Ui {
class NoteWidget;
}

class TypeAheadFindBar;

namespace QtNote {

struct ActData;
class NoteEdit;
class NoteHighlighter;

class QTNOTE_EXPORT NoteWidget : public QWidget
{
    Q_OBJECT

public:
    enum Feature {
        RichText
    };
    Q_DECLARE_FLAGS(Features, Feature)

    explicit NoteWidget(const QString &storageId, const QString &noteId);
    ~NoteWidget();

    void setText(QString text);
    QString text();
    inline const Features &features() const { return _features; }
    inline void setFeatures(const Features &features) { _features = features; }
    NoteEdit* editWidget() const;
    inline NoteHighlighter* highlighter() const { return _highlighter; }
    void setAcceptRichText(bool state);
    inline QString storageId() const { return _storageId; }
    inline QString noteId() const { return _noteId; }
    void setNoteId(const QString &noteId);
    inline const QString &firstLine() const { return _firstLine; }
    inline qint64 lastChangeElapsed() const { return _lastChangeElapsed.elapsed(); }
    inline bool isTrashRequested() const { return _trashRequested; }
    inline void setTrashRequested(bool state) { _trashRequested = state; }

signals:
    void firstLineChanged();
    void trashRequested();
    void saveRequested();
    void noteIdChanged(const QString &oldId, const QString &newId);
    void invalidated(); // emited when we are unsure we have the latest data

protected:
    void changeEvent(QEvent *e);
    void keyPressEvent(QKeyEvent *event);

public slots:
    void save();
    void rereadSettings();

private slots:
    void onFindTriggered();
    void onReplaceTriggered();
    void autosave();
    void onCopyClicked();
    void textChanged();
    void onPrintClicked();
    void onSaveClicked();
    void onTrashClicked();
    void updateFirstLineColor();    
    void focusReceived();

private:
    Ui::NoteWidget *ui;

    TypeAheadFindBar *findBar;
    NoteHighlighter* _highlighter;
    HighlighterExtension::Ptr _linkHighlighter;
    QString _storageId;
    QString _noteId;
    QString _firstLine;
    QString _extFileName;
    QString _extSelecteFilter;
    QTimer _autosaveTimer;
    QElapsedTimer _lastChangeElapsed;
    Features _features;
    bool _trashRequested;
    bool _changed;

    QAction *initAction(const ActData &name);
};

} // namespace QtNote

Q_DECLARE_OPERATORS_FOR_FLAGS(QtNote::NoteWidget::Features)

#endif // NOTEWIDGET_H
