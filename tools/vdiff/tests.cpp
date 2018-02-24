#include <QFile>
#include <QDir>

#include "paths.h"

#include "tests.h"

static TestState stateFormStr(const QStringRef &str)
{
    bool ok = false;
    const int idx = str.toInt(&ok);

    if (!ok) {
        throw QString("Invalid state ID: '%1'").arg(str.toString());
    }

    switch (idx) {
        case 0 : return TestState::Unknown;
        case 1 : return TestState::Passed;
        case 2 : return TestState::Failed;
        case 3 : return TestState::Crashed;
        default: {
            throw QString("Invalid state ID: %1").arg(idx);
        }
    }
}

Tests Tests::load()
{
    const auto path = Paths::results();
    QFile file(path);
    if (!file.open(QFile::ReadOnly)) {
        throw QString("Failed to open %1.").arg(path);
    }

    const QString text = file.readAll();

    // A minimal csv parser.
    //
    // We don't cate about escape characters, because they are not used.

    Tests tests;

    int row = 1;
    for (const QStringRef &line : text.splitRef('\n')) {
        if (line.isEmpty()) {
            break;
        }

        const auto items = line.split(',');

        if (items.size() != 6) {
            throw QString("Invalid columns count at row %1.").arg(row);
        }

        TestItem item;
        item.path       = items.at(0).toString();
        item.chrome     = stateFormStr(items.at(1));
        item.resvg      = stateFormStr(items.at(2));
        item.inkscape   = stateFormStr(items.at(3));
        item.librsvg    = stateFormStr(items.at(4));
        item.qtsvg      = stateFormStr(items.at(5));

        tests.m_data << item;

        row++;
    }

    return tests;
}

void Tests::save()
{
    QString text;
    for (const auto &item : m_data) {
        text += item.path + ',';
        text += QString::number((int)item.chrome)   + ',';
        text += QString::number((int)item.resvg)    + ',';
        text += QString::number((int)item.inkscape) + ',';
        text += QString::number((int)item.librsvg)  + ',';
        text += QString::number((int)item.qtsvg)    + '\n';
    }

    const auto path = Paths::results();
    QFile file(path);
    if (!file.open(QFile::WriteOnly)) {
        throw QString("Failed to open %1.").arg(path);
    }

    file.write(text.toUtf8());
}

void Tests::resync()
{
    auto oldTests = load();

    const auto files = QDir(Paths::testsPath()).entryInfoList({ "*.svg" });
    for (const auto &fi : files) {
        const auto fileName = fi.fileName();

        bool isExists = false;
        for (const auto &test : oldTests) {
            if (test.path == fileName) {
                isExists = true;
                break;
            }
        }

        if (!isExists) {
            TestItem item;
            item.path = fileName;

            oldTests.m_data << item;
        }
    }

    std::sort(oldTests.m_data.begin(), oldTests.m_data.end(), [](TestItem &a, TestItem &b) {
        return a.path < b.path;
    });

    oldTests.save();
}