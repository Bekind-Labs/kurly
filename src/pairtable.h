#pragma once

#include <QList>
#include <QPair>
#include <QWidget>

class QPushButton;
class QTableWidget;

// An editable table of name/value pairs with Add and Remove buttons, used for
// headers and cookies.
class PairTable : public QWidget
{
    Q_OBJECT
public:
    // objectName names the table; addName and removeName its buttons.
    PairTable(const QString &objectName, const QString &addName, const QString &removeName,
              QWidget *parent = nullptr);

    // The rows that have a name, in order.
    QList<QPair<QString, QString>> pairs() const;
    // Replaces the rows without emitting changed(). An empty list leaves one
    // blank row to type into.
    void setPairs(const QList<QPair<QString, QString>> &pairs);

    void setTexts(const QString &nameColumn, const QString &valueColumn,
                  const QString &add, const QString &remove);

    QTableWidget *table() const { return m_table; }

public slots:
    // Adds a row and puts the cursor in it.
    void addRow(const QString &name = {}, const QString &value = {});
    // Removes the selected rows, or the current one if none is selected.
    void removeSelected();

signals:
    void changed();

private:
    void appendRow(const QString &name, const QString &value);

    QTableWidget *m_table;
    QPushButton *m_add, *m_remove;
};
