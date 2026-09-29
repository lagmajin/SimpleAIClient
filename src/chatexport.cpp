#include "mainwindow.h"
#include "appicons.h"
#include "chatstore.h"
#include "chatwidgets.h"

#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>


// Export and recovery: writing a chat to Markdown and driving the
// backup snapshot restore.


#include <QSaveFile>
#include <QFileDialog>
#include <QMessageBox>
#include <QHeaderView>
#include <QTableWidget>
#include <QScrollBar>


void MainWindow::showShortcutsDialog()
{
    QDialog dialog(this);
    dialog.setWindowTitle("Keyboard Shortcuts");
    dialog.setMinimumWidth(400);

    QVBoxLayout *layout = new QVBoxLayout(&dialog);

    QTableWidget *table = new QTableWidget(&dialog);
    table->setColumnCount(2);
    table->setHorizontalHeaderLabels({"Shortcut", "Action"});
    table->horizontalHeader()->setStretchLastSection(true);
    table->verticalHeader()->setVisible(false);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    const Theme &t = currentTheme();
    table->setStyleSheet(
        "QTableWidget { "
        "  background-color: " + t.surfaceAlt + "; "
        "  color: " + t.textStrong + "; "
        "  gridline-color: " + t.border + "; "
        "  border: 1px solid " + t.borderStrong + "; "
        "  selection-background-color: " + t.accent + "; "
        "  selection-color: " + t.accentText + "; "
        "  font-size: 13px; "
        "} "
        "QHeaderView::section { "
        "  background-color: " + t.surfaceSunken + "; "
        "  color: " + t.textStrong + "; "
        "  padding: 4px; "
        "  border: 1px solid " + t.borderStrong + "; "
        "} "
        + scrollBarStyle()
    );

    QList<QPair<QString, QString>> shortcuts = {
        {"Ctrl+N", "New Chat"},
        {"Ctrl+E", "Export Chat"},
        {"Ctrl+,", "Settings"},
        {"Ctrl+Shift+S", "Advanced Settings"},
        {"Ctrl+Q", "Quit"},
        {"Ctrl+F", "Search in Chat"},
        {"Ctrl+T", "Toggle Theme"},
        {"Ctrl+=", "Increase Font Size"},
        {"Ctrl+-", "Decrease Font Size"},
        {"Ctrl+0", "Reset Font Size"},
        {"Ctrl+?", "Show Shortcuts"},
        {"Enter", "Send Message"},
        {"Shift+Enter", "New Line"},
    };

    table->setRowCount(shortcuts.size());
    for (int i = 0; i < shortcuts.size(); ++i) {
        table->setItem(i, 0, new QTableWidgetItem(shortcuts[i].first));
        table->setItem(i, 1, new QTableWidgetItem(shortcuts[i].second));
    }

    layout->addWidget(table);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, &dialog);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttonBox);

    dialog.exec();
}
void MainWindow::onExportChat()
{
    if (m_currentChatIndex < 0 || m_currentChatIndex >= m_chatSessions.size()) {
        QMessageBox::information(this, "Export Chat", "No chat to export.");
        return;
    }

    loadChatMessages(m_currentChatIndex);

    const auto &chat = m_chatSessions[m_currentChatIndex];
    QString defaultName = chat.title.isEmpty() ? "chat" : chat.title;
    defaultName.replace(QRegularExpression("[^a-zA-Z0-9\\s]"), "");

    QString filePath = QFileDialog::getSaveFileName(this, "Export Chat", defaultName + ".md", "Markdown Files (*.md);;Text Files (*.txt);;All Files (*)");
    if (filePath.isEmpty()) return;

    QString markdown;
    QTextStream out(&markdown);

    out << "# " << chat.title << "\n\n";
    out << "Exported: " << QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss") << "\n\n";
    out << "---\n\n";

    for (const auto &msg : chat.messages) {
        QString role = msg.role == "user" ? "You" : "Assistant";
        out << "## " << role << "\n\n";
        if (!msg.imageUrl.isEmpty()) {
            out << "*[Image attached]*\n\n";
        }
        out << msg.content << "\n\n";
        if (msg.totalTokens > 0) {
            out << "*Tokens: " << msg.promptTokens << " prompt, " << msg.completionTokens << " completion, " << msg.totalTokens << " total*\n\n";
        }
        out << "---\n\n";
    }
    out.flush();

    // QSaveFile so a crash mid-export cannot leave a truncated transcript.
    QSaveFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Export Failed", "Could not save file: " + filePath);
        return;
    }
    file.write(markdown.toUtf8());
    if (!file.commit()) {
        QMessageBox::warning(this, "Export Failed", "Could not finalise file: " + filePath);
        return;
    }
    QMessageBox::information(this, "Export Complete", "Chat exported to:\n" + filePath);
}

void MainWindow::onRecoverChats()
{
    QJsonObject snapshot;
    if (!m_store->loadBackup(&snapshot)) {
        QMessageBox::information(this, "Recover Lost Chats", "No backup snapshot was found yet.");
        return;
    }

    QMap<QString, QJsonObject> backupById;
    QJsonArray sessionsArray = snapshot["sessions"].toArray();
    for (const auto &val : sessionsArray) {
        QJsonObject sessionObj = val.toObject();
        const QString id = sessionObj["id"].toString();
        if (!id.isEmpty()) {
            backupById[id] = sessionObj;
        }
    }

    QList<int> candidates = recoverableChatIndices();
    if (candidates.isEmpty()) {
        QMessageBox::information(this, "Recover Lost Chats", "No recoverable chats were found in the backup.");
        return;
    }

    QDialog dialog(this);
    dialog.setWindowTitle("Recover Lost Chats");
    dialog.setMinimumSize(760, 420);

    QVBoxLayout *layout = new QVBoxLayout(&dialog);

    QLabel *info = new QLabel(
        "These chats are currently empty in the app, but a backup still has message data for them.\n"
        "Select one or more rows and restore them back into the app.",
        &dialog
    );
    info->setWordWrap(true);
    layout->addWidget(info);

    QTableWidget *table = new QTableWidget(&dialog);
    table->setColumnCount(4);
    table->setHorizontalHeaderLabels({"Title", "Current", "Backup", "Chat ID"});
    table->horizontalHeader()->setStretchLastSection(true);
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::ExtendedSelection);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setRowCount(candidates.size());

    for (int row = 0; row < candidates.size(); ++row) {
        int index = candidates[row];
        const ChatSession &chat = m_chatSessions[index];
        const QJsonObject sessionObj = backupById.value(chat.id);
        const int backupCount = sessionObj["messageCount"].toInt();

        auto *titleItem = new QTableWidgetItem(chat.title.isEmpty() ? "(untitled)" : chat.title);
        titleItem->setData(Qt::UserRole, chat.id);
        table->setItem(row, 0, titleItem);
        table->setItem(row, 1, new QTableWidgetItem(QString::number(chat.messageCount)));
        table->setItem(row, 2, new QTableWidgetItem(QString::number(backupCount)));
        table->setItem(row, 3, new QTableWidgetItem(chat.id));
    }
    layout->addWidget(table);

    QDialogButtonBox *buttons = new QDialogButtonBox(&dialog);
    QPushButton *restoreSelectedBtn = buttons->addButton("Restore Selected", QDialogButtonBox::AcceptRole);
    QPushButton *restoreAllBtn = buttons->addButton("Restore All", QDialogButtonBox::ActionRole);
    buttons->addButton(QDialogButtonBox::Close);
    layout->addWidget(buttons);

    auto restoreRows = [this, table, &dialog](const QList<int> &rows) {
        int restored = 0;
        int failed = 0;
        for (int row : rows) {
            QTableWidgetItem *item = table->item(row, 0);
            if (!item) {
                ++failed;
                continue;
            }
            const QString chatId = item->data(Qt::UserRole).toString();
            if (restoreChatFromBackup(chatId)) {
                ++restored;
            } else {
                ++failed;
            }
        }

        if (restored > 0) {
            updateChatList();
            if (m_currentChatIndex >= 0 && m_currentChatIndex < m_chatSessions.size()) {
                switchToChat(m_currentChatIndex);
            }
        }

        QMessageBox::information(
            &dialog,
            "Recover Lost Chats",
            QString("Restored %1 chat(s).%2")
                .arg(restored)
                .arg(failed > 0 ? QString("\n%1 chat(s) could not be restored.").arg(failed) : QString())
        );
    };

    connect(restoreSelectedBtn, &QPushButton::clicked, &dialog, [&]() {
        QList<int> rows;
        for (const auto &range : table->selectedRanges()) {
            for (int row = range.topRow(); row <= range.bottomRow(); ++row) {
                rows.append(row);
            }
        }
        if (rows.isEmpty()) {
            QMessageBox::information(&dialog, "Recover Lost Chats", "Select one or more rows first.");
            return;
        }
        restoreRows(rows);
    });

    connect(restoreAllBtn, &QPushButton::clicked, &dialog, [&]() {
        QList<int> rows;
        for (int row = 0; row < table->rowCount(); ++row) {
            rows.append(row);
        }
        restoreRows(rows);
    });

    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);

    dialog.exec();
}

void MainWindow::onExportBackupSnapshot()
{
    QString defaultName = QString("simpleaiclient-backup-%1.json")
        .arg(QDateTime::currentDateTime().toString("yyyyMMdd-HHmmss"));
    QString filePath = QFileDialog::getSaveFileName(
        this,
        "Export Backup Snapshot",
        defaultName,
        "JSON Files (*.json);;All Files (*)"
    );
    if (filePath.isEmpty()) {
        return;
    }

    // An explicit export stays readable on purpose: the user picked this path,
    // and the store accepts both the encrypted and the plain form on import.
    QJsonDocument doc(m_store->exportSnapshot(currentChatId()));
    QSaveFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Export Failed", "Could not save file: " + filePath);
        return;
    }

    file.write(doc.toJson(QJsonDocument::Indented));
    if (!file.commit()) {
        QMessageBox::warning(this, "Export Failed", "Could not finalise file: " + filePath);
        return;
    }

    if (!recoverableChatIndices().isEmpty()) {
        QMessageBox::information(this, "Export Complete",
            "Backup snapshot exported to:\n" + filePath +
            "\n\nIt contains your chat transcripts in plain text. Store it accordingly.");
        return;
    }
    QMessageBox::information(this, "Export Complete", "Backup snapshot exported to:\n" + filePath);
}
