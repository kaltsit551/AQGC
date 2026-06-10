#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtQmlIntegration/QtQmlIntegration>

class QNetworkAccessManager;
class QNetworkReply;
class AIVehicleTools;
class AISettingsTools;

class AIChatService : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QString apiKey       READ apiKey         WRITE setApiKey     NOTIFY apiKeyChanged)
    Q_PROPERTY(QString baseUrl      READ baseUrl        WRITE setBaseUrl    NOTIFY baseUrlChanged)
    Q_PROPERTY(QString model        READ model          WRITE setModel      NOTIFY modelChanged)
    Q_PROPERTY(QString conversation READ conversation                       NOTIFY conversationChanged)
    Q_PROPERTY(bool    busy         READ busy                               NOTIFY busyChanged)
    Q_PROPERTY(bool    vehicleControlEnabled READ vehicleControlEnabled WRITE setVehicleControlEnabled NOTIFY vehicleControlEnabledChanged)
    Q_PROPERTY(bool    settingsControlEnabled READ settingsControlEnabled WRITE setSettingsControlEnabled NOTIFY settingsControlEnabledChanged)
    Q_PROPERTY(bool    confirmActions         READ confirmActions         WRITE setConfirmActions         NOTIFY confirmActionsChanged)

public:
    explicit AIChatService(QObject *parent = nullptr);
    ~AIChatService() override;

    static AIChatService *instance();

    QString apiKey() const { return _apiKey; }
    QString baseUrl() const { return _baseUrl; }
    QString model() const { return _model; }
    QString conversation() const { return _conversation; }
    bool busy() const { return _busy; }
    bool vehicleControlEnabled() const { return _vehicleControlEnabled; }
    bool settingsControlEnabled() const { return _settingsControlEnabled; }
    bool confirmActions() const { return _confirmActions; }

    void setApiKey(const QString &apiKey);
    void setBaseUrl(const QString &baseUrl);
    void setModel(const QString &model);
    void setVehicleControlEnabled(bool enabled);
    void setSettingsControlEnabled(bool enabled);
    void setConfirmActions(bool confirm);

    Q_INVOKABLE void sendMessage(const QString &text);
    Q_INVOKABLE void clearConversation();
    Q_INVOKABLE void stop();
    /// QML calls this in response to confirmationRequested.
    Q_INVOKABLE void confirmToolCall(const QString &callId, bool approved);

signals:
    void apiKeyChanged();
    void baseUrlChanged();
    void modelChanged();
    void conversationChanged();
    void busyChanged();
    void vehicleControlEnabledChanged();
    void settingsControlEnabledChanged();
    void confirmActionsChanged();
    /// Emitted when a control tool needs user confirmation before execution.
    void confirmationRequested(const QString &callId, const QString &toolName, const QString &argsText);

private slots:
    void _onFinished();

private:
    void _setBusy(bool busy);
    void _appendConversation(const QString &text);
    void _startRequest();
    QJsonArray _buildMessages() const;
    void _handleAssistantMessage(const QJsonObject &message);
    void _processToolCalls();
    void _appendToolResult(const QString &callId, const QString &name, const QJsonObject &result);
    QString _formatArgs(const QJsonObject &args) const;
    QJsonObject _executeTool(const QString &name, const QJsonObject &args);
    bool _toolNeedsConfirmation(const QString &name) const;

    QNetworkAccessManager *_networkManager = nullptr;
    QNetworkReply *_reply = nullptr;
    AIVehicleTools *_tools = nullptr;
    AISettingsTools *_settingsTools = nullptr;

    QString _apiKey;
    QString _baseUrl;
    QString _model;
    QString _conversation;
    bool _busy = false;
    bool _vehicleControlEnabled = false;
    bool _settingsControlEnabled = false;
    bool _confirmActions = true;

    // Full message history (user/assistant/tool), excluding the system prompt.
    QJsonArray _messages;

    // State for processing tool_calls within the current assistant turn.
    QJsonArray _pendingToolCalls;
    int _pendingIndex = 0;
    QString _awaitingCallId;
};
