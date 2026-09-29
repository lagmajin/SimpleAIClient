#include "mainwindow.h"
#include "appicons.h"
#include "chatwidgets.h"

#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>


// Settings, API profiles and model selection. Touches m_profiles, the
// combo boxes and the QSettings keys, and nothing in the chat view.



#include <QFormLayout>

#include <QInputDialog>
#include <QMessageBox>
#include <QSpinBox>
#include <QScrollBar>




void MainWindow::loadSettings()
{
    QString apiKey = SecretStore::unprotect(m_settings.value("apiKey").toString());

    m_apiClient->setApiKey(apiKey);
    restoreModelSelection();

    m_apiClient->setSystemPrompt(SecretStore::unprotect(m_settings.value("systemPrompt").toString()));

    double temperature = m_settings.value("temperature", 0.7).toDouble();
    m_apiClient->setTemperature(temperature);

    int maxTokens = m_settings.value("maxTokens", 0).toInt();
    m_apiClient->setMaxTokens(maxTokens);

    bool webSearch = m_settings.value("webSearch", false).toBool();
    m_apiClient->setWebSearch(webSearch);

    m_chatFontSize = qBound(10, m_settings.value("chatFontSize", 15).toInt(), 30);
}

void MainWindow::fetchModels()
{
    if (!checkApiKey()) return;
    // Block combo signals while swapping in the placeholder: clear()/addItem()
    // would otherwise emit currentTextChanged and have onModelChanged persist
    // ""/"Loading models..." over the user's saved model selection.
    {
        QSignalBlocker blocker(m_modelCombo);
        m_modelCombo->clear();
        m_modelCombo->addItem("Loading models...");
    }
    m_modelCombo->setEnabled(false);
    m_apiClient->fetchModels();
}

void MainWindow::saveSettings()
{
    m_settings.setValue("chatFontSize", m_chatFontSize);
    m_settings.sync();
}

bool MainWindow::checkApiKey()
{
    if (m_apiClient->apiKey().trimmed().isEmpty()) {
        QMessageBox::warning(this, "API Key Required", "Please set your Venice.ai API key in Settings.");
        return false;
    }
    return true;
}

void MainWindow::applyModelFilter()
{
    QString savedModel = m_settings.value("model", "venice-uncensored").toString();
    bool uncensoredOnly = m_uncensoredFilter->isChecked();

    QSignalBlocker blocker(m_modelCombo);
    m_modelCombo->clear();
    for (const auto &model : m_allModels) {
        if (uncensoredOnly && !model.contains("uncensored", Qt::CaseInsensitive)) {
            continue;
        }
        m_modelCombo->addItem(model);
    }

    int index = m_modelCombo->findText(savedModel);
    if (index >= 0) {
        m_modelCombo->setCurrentIndex(index);
    } else if (!savedModel.isEmpty()) {
        m_modelCombo->setEditText(savedModel);
    }
    m_apiClient->setModel(m_modelCombo->currentText());
}

void MainWindow::onSettings()
{
    const QString currentKey = SecretStore::unprotect(m_settings.value("apiKey").toString());

    bool ok;
    QString apiKey = QInputDialog::getText(this, "Settings", "Venice.ai API Key:",
                                           QLineEdit::Password, currentKey, &ok);
    if (ok) {
        m_settings.setValue("apiKey", SecretStore::protect(apiKey));
        m_apiClient->setApiKey(apiKey);
        fetchModels();
    }

    saveSettings();
}

void MainWindow::onAdvancedSettings()
{
    QDialog dialog(this);
    dialog.setWindowTitle("Advanced Settings");
    dialog.setMinimumWidth(400);

    QVBoxLayout *layout = new QVBoxLayout(&dialog);

    QFormLayout *formLayout = new QFormLayout();
    formLayout->setSpacing(12);

    QString currentPrompt = SecretStore::unprotect(m_settings.value("systemPrompt").toString());
    QTextEdit *systemPromptEdit = new QTextEdit(&dialog);
    systemPromptEdit->setPlainText(currentPrompt);
    systemPromptEdit->setMaximumHeight(100);
    systemPromptEdit->setPlaceholderText("e.g., You are a helpful assistant that speaks in a formal tone...");
    formLayout->addRow("System Prompt:", systemPromptEdit);

    double currentTemp = m_settings.value("temperature", 0.7).toDouble();
    QDoubleSpinBox *tempSpin = new QDoubleSpinBox(&dialog);
    tempSpin->setRange(0.0, 2.0);
    tempSpin->setSingleStep(0.1);
    tempSpin->setValue(currentTemp);
    tempSpin->setToolTip("Controls randomness: 0 = deterministic, 2 = very random");
    formLayout->addRow("Temperature:", tempSpin);

    int currentMaxTokens = m_settings.value("maxTokens", 0).toInt();
    QSpinBox *maxTokensSpin = new QSpinBox(&dialog);
    maxTokensSpin->setRange(0, 32000);
    maxTokensSpin->setSingleStep(256);
    maxTokensSpin->setValue(currentMaxTokens);
    maxTokensSpin->setSpecialValueText("Unlimited");
    maxTokensSpin->setToolTip("Maximum tokens in response (0 = unlimited)");
    formLayout->addRow("Max Tokens:", maxTokensSpin);

    layout->addLayout(formLayout);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttonBox);

    if (dialog.exec() == QDialog::Accepted) {
        m_settings.setValue("systemPrompt", SecretStore::protect(systemPromptEdit->toPlainText()));
        m_settings.setValue("temperature", tempSpin->value());
        m_settings.setValue("maxTokens", maxTokensSpin->value());

        m_apiClient->setSystemPrompt(systemPromptEdit->toPlainText());
        m_apiClient->setTemperature(tempSpin->value());
        m_apiClient->setMaxTokens(maxTokensSpin->value());
    }
}

void MainWindow::onModelsFetched(const QStringList &models)
{
    m_allModels = models;
    m_modelCombo->setEnabled(true);
    applyModelFilter();
    restoreModelSelection();
}

void MainWindow::onModelChanged(const QString &model)
{
    // Never persist transient combo states (cleared, placeholder text).
    if (model.isEmpty() || model == "Loading models...") {
        return;
    }
    m_apiClient->setModel(model);
    m_settings.setValue("model", model);
    m_settings.setValue("lastModel", model);
    if (m_statusModel) {
        m_statusModel->setText(model);
    }
    updateHeaderState();
}

void MainWindow::loadProfiles()
{
    m_profiles.clear();
    m_profileCombo->clear();

    // Only the API keys inside are protected individually, so the profile
    // document itself stays plain and a corrupted key cannot hide the rest.
    QString data = m_settings.value("apiProfiles").toString();
    if (!data.isEmpty()) {
        QJsonDocument doc = QJsonDocument::fromJson(data.toUtf8());
        if (doc.isArray()) {
            for (const auto &val : doc.array()) {
                QJsonObject obj = val.toObject();
                ApiProfile profile;
                profile.name = obj["name"].toString();
                profile.apiKey = SecretStore::unprotect(obj["apiKey"].toString());
                profile.model = obj["model"].toString();
                // A system prompt can carry confidential context, so it is
                // protected too; an empty one is stored unprotected.
                profile.systemPrompt = obj["systemPrompt"].toString();
                if (SecretStore::isEncrypted(obj["systemPrompt"].toString())) {
                    profile.systemPrompt = SecretStore::unprotect(obj["systemPrompt"].toString());
                }
                profile.temperature = obj["temperature"].toDouble();
                profile.maxTokens = obj["maxTokens"].toInt();
                m_profiles.append(profile);
            }
        }
    }

    if (m_profiles.isEmpty()) {
        ApiProfile defaultProfile;
        defaultProfile.name = "Default";
        defaultProfile.apiKey = SecretStore::unprotect(m_settings.value("apiKey").toString());
        defaultProfile.model = m_settings.value("model", "venice-uncensored").toString();
        defaultProfile.systemPrompt = SecretStore::unprotect(m_settings.value("systemPrompt").toString());
        defaultProfile.temperature = m_settings.value("temperature", 0.7).toDouble();
        defaultProfile.maxTokens = m_settings.value("maxTokens", 0).toInt();
        m_profiles.append(defaultProfile);
    }

    for (const auto &profile : m_profiles) {
        m_profileCombo->addItem(profile.name);
    }

    m_currentProfileName = m_settings.value("currentProfile", m_profiles[0].name).toString();
    int idx = m_profileCombo->findText(m_currentProfileName);
    if (idx >= 0) {
        m_profileCombo->setCurrentIndex(idx);
    }
    applyProfile(m_profiles[idx >= 0 ? idx : 0]);
}

void MainWindow::saveProfiles()
{
    QJsonArray arr;
    for (const auto &profile : m_profiles) {
        QJsonObject obj;
        obj["name"] = profile.name;
        obj["apiKey"] = SecretStore::protect(profile.apiKey);
        obj["model"] = profile.model;
        obj["systemPrompt"] = SecretStore::protect(profile.systemPrompt);
        obj["temperature"] = profile.temperature;
        obj["maxTokens"] = profile.maxTokens;
        arr.append(obj);
    }
    m_settings.setValue("apiProfiles", QString::fromUtf8(QJsonDocument(arr).toJson(QJsonDocument::Compact)));
    m_settings.setValue("currentProfile", m_currentProfileName);
}

void MainWindow::applyProfile(const ApiProfile &profile)
{
    m_apiClient->setApiKey(profile.apiKey);
    m_apiClient->setModel(profile.model);
    m_apiClient->setSystemPrompt(profile.systemPrompt);
    m_apiClient->setTemperature(profile.temperature);
    m_apiClient->setMaxTokens(profile.maxTokens);

    QSignalBlocker blocker(m_modelCombo);
    m_modelCombo->setCurrentText(profile.model);
    if (m_statusModel) {
        m_statusModel->setText(profile.model);
    }
    updateHeaderState();
}

void MainWindow::restoreModelSelection()
{
    QString model = m_settings.value("lastModel", m_settings.value("model", "venice-uncensored")).toString();
    if (model.isEmpty()) {
        model = "venice-uncensored";
    }

    QSignalBlocker blocker(m_modelCombo);
    int index = m_modelCombo->findText(model);
    if (index >= 0) {
        m_modelCombo->setCurrentIndex(index);
    } else {
        m_modelCombo->setEditText(model);
    }

    m_apiClient->setModel(model);
    if (m_statusModel) {
        m_statusModel->setText(model);
    }
}

void MainWindow::onProfileChanged()
{
    QString name = m_profileCombo->currentText();
    for (const auto &profile : m_profiles) {
        if (profile.name == name) {
            m_currentProfileName = name;
            applyProfile(profile);
            m_settings.setValue("model", profile.model);
            m_settings.setValue("lastModel", profile.model);
            saveProfiles();
            updateHeaderState();
            break;
        }
    }
}

void MainWindow::onManageProfiles()
{
    QDialog dialog(this);
    dialog.setWindowTitle("Manage API Profiles");
    dialog.setMinimumWidth(500);

    QVBoxLayout *layout = new QVBoxLayout(&dialog);

    QListWidget *profileList = new QListWidget(&dialog);
    for (const auto &profile : m_profiles) {
        profileList->addItem(profile.name);
    }
    layout->addWidget(profileList);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    QPushButton *addBtn = new QPushButton("Add", &dialog);
    addBtn->setIcon(makeLineIcon(AppIconGlyph::NewChat));
    addBtn->setIconSize(QSize(14, 14));
    QPushButton *editBtn = new QPushButton("Edit", &dialog);
    editBtn->setIcon(makeLineIcon(AppIconGlyph::Edit));
    editBtn->setIconSize(QSize(14, 14));
    QPushButton *deleteBtn = new QPushButton("Delete", &dialog);
    deleteBtn->setIcon(makeLineIcon(AppIconGlyph::Trash));
    deleteBtn->setIconSize(QSize(14, 14));
    btnLayout->addWidget(addBtn);
    btnLayout->addWidget(editBtn);
    btnLayout->addWidget(deleteBtn);
    layout->addLayout(btnLayout);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, &dialog);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttonBox);

    auto openProfileDialog = [&](ApiProfile *profile, bool isNew) -> bool {
        QDialog dlg(&dialog);
        dlg.setWindowTitle(isNew ? "Add Profile" : "Edit Profile");
        dlg.setMinimumWidth(400);

        QVBoxLayout *dlgLayout = new QVBoxLayout(&dlg);
        QFormLayout *form = new QFormLayout();

        QLineEdit *nameEdit = new QLineEdit(&dlg);
        nameEdit->setText(profile->name);
        form->addRow("Name:", nameEdit);

        QLineEdit *keyEdit = new QLineEdit(&dlg);
        keyEdit->setText(profile->apiKey);
        keyEdit->setEchoMode(QLineEdit::Password);
        form->addRow("API Key:", keyEdit);

        QLineEdit *modelEdit = new QLineEdit(&dlg);
        modelEdit->setText(profile->model);
        form->addRow("Model:", modelEdit);

        QTextEdit *promptEdit = new QTextEdit(&dlg);
        promptEdit->setPlainText(profile->systemPrompt);
        promptEdit->setMaximumHeight(80);
        form->addRow("System Prompt:", promptEdit);

        QDoubleSpinBox *tempSpin = new QDoubleSpinBox(&dlg);
        tempSpin->setRange(0.0, 2.0);
        tempSpin->setSingleStep(0.1);
        tempSpin->setValue(profile->temperature);
        form->addRow("Temperature:", tempSpin);

        QSpinBox *maxSpin = new QSpinBox(&dlg);
        maxSpin->setRange(0, 32000);
        maxSpin->setSingleStep(256);
        maxSpin->setValue(profile->maxTokens);
        maxSpin->setSpecialValueText("Unlimited");
        form->addRow("Max Tokens:", maxSpin);

        dlgLayout->addLayout(form);

        QDialogButtonBox *bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
        connect(bb, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
        connect(bb, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
        dlgLayout->addWidget(bb);

        if (dlg.exec() == QDialog::Accepted) {
            profile->name = nameEdit->text().trimmed();
            profile->apiKey = keyEdit->text();
            profile->model = modelEdit->text().trimmed();
            profile->systemPrompt = promptEdit->toPlainText();
            profile->temperature = tempSpin->value();
            profile->maxTokens = maxSpin->value();
            return true;
        }
        return false;
    };

    connect(addBtn, &QPushButton::clicked, [&]() {
        ApiProfile newProfile;
        newProfile.name = "New Profile";
        newProfile.model = "venice-uncensored";
        newProfile.temperature = 0.7;
        newProfile.maxTokens = 0;
        if (openProfileDialog(&newProfile, true)) {
            if (newProfile.name.isEmpty()) newProfile.name = "Unnamed";
            m_profiles.append(newProfile);
            profileList->addItem(newProfile.name);
            m_profileCombo->addItem(newProfile.name);
            saveProfiles();
        }
    });

    connect(editBtn, &QPushButton::clicked, [&]() {
        int row = profileList->currentRow();
        if (row < 0 || row >= m_profiles.size()) return;
        ApiProfile profile = m_profiles[row];
        if (openProfileDialog(&profile, false)) {
            m_profiles[row] = profile;
            profileList->item(row)->setText(profile.name);
            m_profileCombo->setItemText(row, profile.name);
            if (m_currentProfileName == m_profiles[row].name || row == m_profileCombo->currentIndex()) {
                applyProfile(profile);
                m_settings.setValue("model", profile.model);
            }
            saveProfiles();
        }
    });

    connect(deleteBtn, &QPushButton::clicked, [&]() {
        int row = profileList->currentRow();
        if (row < 0 || row >= m_profiles.size()) return;
        if (m_profiles.size() <= 1) {
            QMessageBox::warning(&dialog, "Cannot Delete", "At least one profile must exist.");
            return;
        }
        QString name = m_profiles[row].name;
        if (QMessageBox::question(&dialog, "Delete Profile", QString("Delete profile '%1'?").arg(name)) == QMessageBox::Yes) {
            m_profiles.removeAt(row);
            delete profileList->takeItem(row);
            m_profileCombo->removeItem(row);
            if (m_currentProfileName == name) {
                m_currentProfileName = m_profiles[0].name;
                m_profileCombo->setCurrentIndex(0);
                applyProfile(m_profiles[0]);
                m_settings.setValue("model", m_profiles[0].model);
            }
            saveProfiles();
        }
    });

    dialog.exec();
}
