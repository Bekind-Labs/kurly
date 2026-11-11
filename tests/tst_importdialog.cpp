#include <QtTest>

#include <QApplication>
#include <QClipboard>
#include <QLabel>
#include <QPlainTextEdit>

#include "importdialog.h"

class TestImportDialog : public QObject
{
    Q_OBJECT

    static QPlainTextEdit *input(ImportDialog &d) { return d.findChild<QPlainTextEdit *>("input"); }
    static QLabel *error(ImportDialog &d) { return d.findChild<QLabel *>("error"); }

private slots:
    void init() { QApplication::clipboard()->clear(); }

    void prefillsCurlFromClipboard()
    {
        QApplication::clipboard()->setText("  curl https://example.com\n");
        ImportDialog d;
        QCOMPARE(input(d)->toPlainText(), QString("curl https://example.com"));
        // Selected, so typing or pasting replaces it.
        QCOMPARE(input(d)->textCursor().selectedText(), QString("curl https://example.com"));
    }

    void ignoresOtherClipboardText_data()
    {
        QTest::addColumn<QString>("clip");
        QTest::newRow("empty") << "";
        QTest::newRow("prose") << "hello world";
        QTest::newRow("other command") << "wget https://example.com";
    }
    void ignoresOtherClipboardText()
    {
        QFETCH(QString, clip);
        QApplication::clipboard()->setText(clip);
        ImportDialog d;
        QVERIFY(input(d)->toPlainText().isEmpty());
    }

    void errorsKeepDialogOpen_data()
    {
        QTest::addColumn<QString>("text");
        QTest::addColumn<QString>("message");
        QTest::newRow("empty") << "  " << "Paste a curl command first.";
        QTest::newRow("quote") << "curl 'https://x" << "A quote is never closed";
        QTest::newRow("not curl") << "wget https://x" << "doesn't look like a curl command";
        QTest::newRow("no url") << "curl -L" << "No URL was found";
    }
    void errorsKeepDialogOpen()
    {
        QFETCH(QString, text);
        QFETCH(QString, message);
        ImportDialog d;
        d.show();
        input(d)->setPlainText(text);
        QVERIFY(error(d)->isHidden());
        d.accept();
        QVERIFY(d.isVisible());
        QVERIFY(!error(d)->isHidden());
        QVERIFY2(error(d)->text().contains(message), qPrintable(error(d)->text()));
        QVERIFY(!d.result().ok());

        // Editing the text clears the error.
        input(d)->insertPlainText("x");
        QVERIFY(error(d)->isHidden());
    }

    void errorColorFollowsTheTheme()
    {
        ImportDialog d;
        d.show();
        d.accept(); // empty: shows an error
        QCOMPARE(error(d)->styleSheet(), QString("color: #c62828;"));
        QPalette dark = d.palette();
        dark.setColor(QPalette::Window, QColor(50, 50, 50));
        d.setPalette(dark);
        d.accept();
        QCOMPARE(error(d)->styleSheet(), QString("color: #ef9a9a;"));
    }

    void successAccepts()
    {
        ImportDialog d;
        d.show();
        input(d)->setPlainText("curl -X PUT https://example.com --proxy p");
        d.accept();
        QVERIFY(!d.isVisible());
        QCOMPARE(d.QDialog::result(), int(QDialog::Accepted));
        QVERIFY(d.result().ok());
        QCOMPARE(d.result().options.method, QString("PUT"));
        QCOMPARE(d.result().ignored, QStringList{"--proxy"});
    }
};

QTEST_MAIN(TestImportDialog)
#include "tst_importdialog.moc"
