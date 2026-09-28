#ifndef CHATWIDGETS_H
#define CHATWIDGETS_H

// Chat-specific widgets: the message bubble, the sidebar row and the
// in-chat search bar. They share nothing with MainWindow beyond the theme
// tokens and the icon factory, so they live apart from it.

#include <QDateTime>
#include <QEnterEvent>
#include <QFrame>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QListWidgetItem>
#include <QMouseEvent>
#include <QObject>
#include <QPushButton>
#include <QTextEdit>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>

#include "appicons.h"
#include "theme.h"

class AvatarLabel : public QLabel {
public:
    AvatarLabel(const QString &role, QWidget *parent = nullptr);
    void applyTheme();
private:
    QString m_role;
};

class ChatListItem : public QWidget {
    Q_OBJECT
public:
    ChatListItem(const QString &title, const QString &subtitle, int index, bool isPinned, QWidget *parent = nullptr);
    int index() const { return m_index; }
    void setActive(bool active);
    void applyStyle();

signals:
    void clicked(int index);
    void deleteClicked(int index);
    void renameRequested(int index);
    void pinRequested(int index);

protected:
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    bool m_isActive;
    bool m_hovered = false;
    int m_index;
    bool m_isPinned;
    QLabel *m_iconLabel;
    QLabel *m_titleLabel;
    QLabel *m_subtitleLabel;
    QToolButton *m_deleteBtn;
};

class ChatMessageCard : public QWidget {
    Q_OBJECT
public:
    ChatMessageCard(const QString &role, const QString &content, QWidget *parent = nullptr);
    void appendContent(const QString &content);
    void setContent(const QString &content);
    void showCopyButton(bool show);
    void setStreaming(bool streaming);
    void setTokenInfo(int promptTokens, int completionTokens, int totalTokens, int responseTimeMs = 0);
    void setMessageIndex(int index);
    void showRegenerateButton(bool show);
    void showBranchButton(bool show);
    void setEditable(bool editable);
    void startEditing();
    QString content() const { return m_fullContent; }
    void highlightText(const QString &text);
    void clearHighlight();
    void setTimestamp(const QDateTime &timestamp);
    void setContentFontSize(int pixels);
    void applyTheme();

protected:
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

signals:
    void copyRequested(const QString &text);
    void regenerateRequested(int messageIndex);
    void branchRequested(int messageIndex);
    void editRequested(int messageIndex, const QString &newContent);

private:
    void renderMarkdown(const QString &text);
    void applyCardStyle();
    QString streamLabelStyle() const;
    QString editFieldStyle() const;
    QString renderInlineMarkdown(const QString &text);
    void addCodeBlock(const QString &code, const QString &language);
    void addTextBlock(const QString &text);
    void rebuildContent();

    QString m_role;
    QString m_fullContent;
    QString m_highlightText;
    QVBoxLayout *m_outerLayout;
    AvatarLabel *m_avatar;
    QFrame *m_card;
    QVBoxLayout *m_layout;
    QWidget *m_copyContainer;
    QPushButton *m_copyBtn;
    QLabel *m_streamLabel;
    QLabel *m_tokenLabel;
    QPushButton *m_regenerateBtn;
    QPushButton *m_branchBtn;
    QPushButton *m_editBtn;
    QTextEdit *m_editField;
    int m_messageIndex;
    bool m_isStreaming;
    QDateTime m_timestamp;
    QLabel *m_timestampLabel;
    int m_contentFontSize = 15;
};

class ChatSearchBar : public QFrame {
    Q_OBJECT
public:
    ChatSearchBar(QWidget *parent = nullptr);
    QLineEdit *m_searchInput;
    QLabel *m_matchCount;

signals:
    void searchTextChanged(const QString &text);
    void findNext();
    void findPrevious();
    void closed();

private slots:
    void onClose();

private:
    QPushButton *m_prevBtn;
    QPushButton *m_nextBtn;
    QPushButton *m_closeBtn;
};

#endif // CHATWIDGETS_H
