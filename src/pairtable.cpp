#include "pairtable.h"

#include "spacing.h"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QPushButton>
#include <QSignalBlocker>
#include <QTableWidget>
#include <QVBoxLayout>

#include <algorithm>

PairTable::PairTable(const QString &objectName, const QString &addName,
                     const QString &removeName, QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(Spacing::small);

    m_table = new QTableWidget(0, 2);
    m_table->setObjectName(objectName);
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->verticalHeader()->hide();
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setMinimumHeight(6 * Spacing::gridUnit);

    auto *buttons = new QHBoxLayout;
    buttons->setSpacing(Spacing::medium);
    m_add = new QPushButton;
    m_add->setObjectName(addName);
    m_remove = new QPushButton;
    m_remove->setObjectName(removeName);
    buttons->addWidget(m_add);
    buttons->addWidget(m_remove);
    buttons->addStretch();
    Layout::lineUp({m_add, m_remove});

    layout->addWidget(m_table);
    layout->addLayout(buttons);

    connect(m_table, &QTableWidget::itemChanged, this, &PairTable::changed);
    connect(m_add, &QPushButton::clicked, this, [this] { addRow(); });
    connect(m_remove, &QPushButton::clicked, this, &PairTable::removeSelected);

    appendRow({}, {});
}

QList<QPair<QString, QString>> PairTable::pairs() const
{
    QList<QPair<QString, QString>> result;
    for (int r = 0; r < m_table->rowCount(); ++r) {
        const auto *n = m_table->item(r, 0);
        const auto *v = m_table->item(r, 1);
        if (n && !n->text().trimmed().isEmpty())
            result.append({n->text(), v ? v->text() : QString()});
    }
    return result;
}

void PairTable::setPairs(const QList<QPair<QString, QString>> &pairs)
{
    QSignalBlocker block(m_table);
    m_table->setRowCount(0);
    for (const auto &p : pairs)
        appendRow(p.first, p.second);
    if (pairs.isEmpty())
        appendRow({}, {});
}

void PairTable::setTexts(const QString &nameColumn, const QString &valueColumn,
                         const QString &add, const QString &remove)
{
    m_table->setHorizontalHeaderLabels({nameColumn, valueColumn});
    m_add->setText(add);
    m_remove->setText(remove);
}

void PairTable::appendRow(const QString &name, const QString &value)
{
    QSignalBlocker block(m_table);
    const int row = m_table->rowCount();
    m_table->insertRow(row);
    m_table->setItem(row, 0, new QTableWidgetItem(name));
    m_table->setItem(row, 1, new QTableWidgetItem(value));
}

void PairTable::addRow(const QString &name, const QString &value)
{
    appendRow(name, value);
    m_table->setCurrentCell(m_table->rowCount() - 1, 0);
    emit changed();
}

void PairTable::removeSelected()
{
    QList<int> rows;
    for (const auto &idx : m_table->selectionModel()->selectedRows())
        rows << idx.row();
    if (rows.isEmpty() && m_table->currentRow() >= 0)
        rows << m_table->currentRow();
    std::sort(rows.begin(), rows.end(), std::greater<int>());
    for (int r : rows)
        m_table->removeRow(r);
    emit changed();
}
