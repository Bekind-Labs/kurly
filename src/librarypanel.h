#pragma once

#include <QWidget>

class QAction;
class QLabel;
class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QPushButton;
class RequestStore;
struct SavedRequest;

// The sidebar listing the saved requests: select, add, duplicate, rename,
// delete, reorder and filter them. Works directly on a RequestStore.
class LibraryPanel : public QWidget
{
    Q_OBJECT
public:
    explicit LibraryPanel(RequestStore *store, QWidget *parent = nullptr);

    // The name shown for a request: its own, an automatic one, or "New request".
    static QString displayName(const SavedRequest &request);

public slots:
    void newRequest();
    void duplicateCurrent();
    void deleteCurrent(); // asks for confirmation
    void renameCurrent();

protected:
    void changeEvent(QEvent *event) override;

private:
    void rebuild();
    void refreshItem(const QString &id);
    void applyFilter();
    void retranslateUi();
    void showContextMenu(const QPoint &pos);
    QString idOf(const QListWidgetItem *item) const;
    QListWidgetItem *itemFor(const QString &id) const;

    RequestStore *m_store;
    QLabel *m_title;
    QLineEdit *m_filter;
    QListWidget *m_list;
    QPushButton *m_new, *m_duplicate, *m_delete;
    QAction *m_renameAction, *m_duplicateAction, *m_deleteAction;
    bool m_rebuilding = false;
    bool m_ignoreListChanges = false;
};
