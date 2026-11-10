#include "librarypanel.h"

#include "requeststore.h"
#include "spacing.h"

#include <QAction>
#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

LibraryPanel::LibraryPanel(RequestStore *store, QWidget *parent)
    : QWidget(parent)
    , m_store(store)
{
    auto *layout = new QVBoxLayout(this);
    // Pads its own content, like the window's other panes.
    layout->setContentsMargins(Spacing::large, Spacing::large, Spacing::large, Spacing::large);
    layout->setSpacing(Spacing::small);

    m_title = new QLabel;
    m_title->setObjectName(QStringLiteral("libraryTitle"));
    QFont bold = m_title->font();
    bold.setBold(true);
    m_title->setFont(bold);
    layout->addWidget(m_title);

    m_filter = new QLineEdit;
    m_filter->setObjectName(QStringLiteral("libraryFilter"));
    m_filter->setClearButtonEnabled(true);
    layout->addWidget(m_filter);

    m_list = new QListWidget;
    m_list->setObjectName(QStringLiteral("libraryList"));
    m_list->setSelectionMode(QAbstractItemView::SingleSelection);
    m_list->setDragDropMode(QAbstractItemView::InternalMove);
    m_list->setDefaultDropAction(Qt::MoveAction);
    m_list->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
    // Shorten long names with "…" in the middle (keeping the method and the
    // end of the path) instead of scrolling sideways.
    m_list->setTextElideMode(Qt::ElideMiddle);
    m_list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_list->setUniformItemSizes(true);
    // Comfortable rows, per the HIG's small spacing.
    m_list->setStyleSheet(QStringLiteral("QListWidget::item { padding: %1px %2px; }")
                              .arg(Spacing::small).arg(Spacing::medium));
    m_list->setContextMenuPolicy(Qt::CustomContextMenu);
    layout->addWidget(m_list, 1);

    auto *buttons = new QHBoxLayout;
    buttons->setSpacing(Spacing::medium);
    m_new = new QPushButton;
    m_new->setObjectName(QStringLiteral("libraryNew"));
    m_duplicate = new QPushButton;
    m_duplicate->setObjectName(QStringLiteral("libraryDuplicate"));
    m_delete = new QPushButton;
    m_delete->setObjectName(QStringLiteral("libraryDelete"));
    for (auto *b : {m_new, m_duplicate, m_delete})
        buttons->addWidget(b, 1);
    layout->addLayout(buttons);

    // Shared by the context menu and the list's keyboard shortcuts.
    m_renameAction = new QAction(this);
    m_renameAction->setShortcut(Qt::Key_F2);
    m_duplicateAction = new QAction(this);
    m_deleteAction = new QAction(this);
    m_deleteAction->setShortcut(QKeySequence::Delete);
    for (auto *a : {m_renameAction, m_deleteAction}) {
        a->setShortcutContext(Qt::WidgetShortcut);
        m_list->addAction(a);
    }

    setMinimumWidth(11 * Spacing::gridUnit);

    connect(m_new, &QPushButton::clicked, this, &LibraryPanel::newRequest);
    connect(m_duplicate, &QPushButton::clicked, this, &LibraryPanel::duplicateCurrent);
    connect(m_delete, &QPushButton::clicked, this, &LibraryPanel::deleteCurrent);
    connect(m_renameAction, &QAction::triggered, this, &LibraryPanel::renameCurrent);
    connect(m_duplicateAction, &QAction::triggered, this, &LibraryPanel::duplicateCurrent);
    connect(m_deleteAction, &QAction::triggered, this, &LibraryPanel::deleteCurrent);
    connect(m_filter, &QLineEdit::textChanged, this, &LibraryPanel::applyFilter);
    connect(m_list, &QWidget::customContextMenuRequested, this, &LibraryPanel::showContextMenu);

    connect(m_list, &QListWidget::currentItemChanged, this, [this](QListWidgetItem *item) {
        if (!m_rebuilding && item)
            m_store->setCurrentId(idOf(item));
    });
    connect(m_list, &QListWidget::itemChanged, this, [this](QListWidgetItem *item) {
        if (m_rebuilding)
            return;
        const QString id = idOf(item);
        const SavedRequest *r = m_store->find(id);
        if (!r)
            return;
        // Leaving the automatic name as it was keeps it automatic.
        const QString text = item->text().trimmed();
        if (text == displayName(*r) && r->name.isEmpty())
            return;
        m_store->rename(id, text == RequestStore::autoName(r->options) ? QString() : text);
        refreshItem(id);
    });
    // Drag and drop reordering.
    connect(m_list->model(), &QAbstractItemModel::rowsMoved, this,
            [this](const QModelIndex &, int start, int, const QModelIndex &, int row) {
                if (m_rebuilding)
                    return;
                // The list already shows the new order; rebuilding it in the
                // middle of the drop would delete items Qt is still using.
                m_ignoreListChanges = true;
                m_store->move(start, row > start ? row - 1 : row);
                m_ignoreListChanges = false;
            });

    connect(m_store, &RequestStore::listChanged, this, &LibraryPanel::rebuild);
    connect(m_store, &RequestStore::requestChanged, this, &LibraryPanel::refreshItem);
    connect(m_store, &RequestStore::currentChanged, this, [this](const QString &id) {
        if (QListWidgetItem *item = itemFor(id)) {
            m_rebuilding = true;
            m_list->setCurrentItem(item);
            m_rebuilding = false;
        }
    });

    retranslateUi();
    rebuild();
}

QString LibraryPanel::displayName(const SavedRequest &request)
{
    if (!request.name.isEmpty())
        return request.name;
    const QString automatic = RequestStore::autoName(request.options);
    return automatic.isEmpty() ? tr("New request") : automatic;
}

void LibraryPanel::retranslateUi()
{
    m_title->setText(tr("Library"));
    m_filter->setPlaceholderText(tr("Filter requests"));
    m_new->setText(tr("New"));
    m_new->setToolTip(tr("Add a new request (%1)")
                          .arg(QKeySequence(Qt::CTRL | Qt::Key_N).toString(QKeySequence::NativeText)));
    m_duplicate->setText(tr("Duplicate"));
    m_duplicate->setToolTip(tr("Make a copy of the selected request"));
    m_delete->setText(tr("Delete"));
    m_delete->setToolTip(tr("Delete the selected request"));
    m_renameAction->setText(tr("Rename"));
    m_duplicateAction->setText(tr("Duplicate"));
    m_deleteAction->setText(tr("Delete"));
    m_list->setToolTip(tr("Double-click a request to rename it. Drag to reorder."));
    rebuild(); // "New request" names
}

void LibraryPanel::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
        retranslateUi();
    QWidget::changeEvent(event);
}

// ---- Keeping the list in sync ----------------------------------------------------

void LibraryPanel::rebuild()
{
    if (m_ignoreListChanges)
        return;
    m_rebuilding = true;
    m_list->clear();
    for (const auto &r : m_store->requests()) {
        auto *item = new QListWidgetItem(displayName(r));
        item->setData(Qt::UserRole, r.id);
        item->setToolTip(r.options.url.isEmpty() ? QString()
                                                 : r.options.method + QLatin1Char(' ') + r.options.url);
        item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable
                       | Qt::ItemIsDragEnabled);
        m_list->addItem(item);
        if (r.id == m_store->currentId())
            m_list->setCurrentItem(item);
    }
    m_rebuilding = false;
    applyFilter();
}

void LibraryPanel::refreshItem(const QString &id)
{
    QListWidgetItem *item = itemFor(id);
    const SavedRequest *r = m_store->find(id);
    if (!item || !r || m_list->isPersistentEditorOpen(item))
        return;
    m_rebuilding = true;
    item->setText(displayName(*r));
    item->setToolTip(r->options.url.isEmpty() ? QString() : r->options.method + QLatin1Char(' ') + r->options.url);
    m_rebuilding = false;
    applyFilter();
}

void LibraryPanel::applyFilter()
{
    const QString needle = m_filter->text().trimmed();
    for (int i = 0; i < m_list->count(); ++i) {
        QListWidgetItem *item = m_list->item(i);
        const SavedRequest *r = m_store->find(idOf(item));
        const bool match = needle.isEmpty() || item->text().contains(needle, Qt::CaseInsensitive)
                           || (r && r->options.url.contains(needle, Qt::CaseInsensitive));
        item->setHidden(!match);
    }
    // Reordering a filtered list would be confusing.
    m_list->setDragDropMode(needle.isEmpty() ? QAbstractItemView::InternalMove
                                             : QAbstractItemView::NoDragDrop);
}

QString LibraryPanel::idOf(const QListWidgetItem *item) const
{
    return item ? item->data(Qt::UserRole).toString() : QString();
}

QListWidgetItem *LibraryPanel::itemFor(const QString &id) const
{
    for (int i = 0; i < m_list->count(); ++i)
        if (idOf(m_list->item(i)) == id)
            return m_list->item(i);
    return nullptr;
}

// ---- Actions -----------------------------------------------------------------------

void LibraryPanel::newRequest()
{
    m_filter->clear(); // so the new request is visible
    const QString id = m_store->add({}, {}, m_store->currentId());
    m_store->setCurrentId(id);
}

void LibraryPanel::duplicateCurrent()
{
    m_filter->clear();
    const QString id = m_store->duplicate(m_store->currentId());
    if (!id.isEmpty())
        m_store->setCurrentId(id);
}

void LibraryPanel::renameCurrent()
{
    if (QListWidgetItem *item = itemFor(m_store->currentId()))
        m_list->editItem(item);
}

void LibraryPanel::deleteCurrent()
{
    const SavedRequest *r = m_store->find(m_store->currentId());
    if (!r)
        return;
    const QString id = r->id;
    QMessageBox box(QMessageBox::Question, tr("Delete Request"),
                    tr("Delete “%1”? This can't be undone.").arg(displayName(*r)),
                    QMessageBox::NoButton, this);
    QPushButton *remove = box.addButton(tr("Delete"), QMessageBox::DestructiveRole);
    box.addButton(tr("Cancel"), QMessageBox::RejectRole);
    box.setDefaultButton(remove);
    box.exec();
    if (box.clickedButton() == remove)
        m_store->remove(id);
}

void LibraryPanel::showContextMenu(const QPoint &pos)
{
    QListWidgetItem *item = m_list->itemAt(pos);
    if (!item)
        return;
    m_list->setCurrentItem(item);
    QMenu menu(this);
    menu.addAction(m_renameAction);
    menu.addAction(m_duplicateAction);
    menu.addSeparator();
    menu.addAction(m_deleteAction);
    menu.exec(m_list->viewport()->mapToGlobal(pos));
}
