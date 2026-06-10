#include "AIChatService.h"
#include "AIVehicleTools.h"
#include "AISettingsTools.h"

#include <QtCore/QApplicationStatic>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonValue>
#include <QtCore/QSettings>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>

Q_APPLICATION_STATIC(AIChatService, _aiChatServiceInstance);

namespace {
const char *kSettingsGroup = "AIChat";
const char *kKeyApiKey     = "apiKey";
const char *kKeyBaseUrl    = "baseUrl";
const char *kKeyModel      = "model";
const char *kKeyControl    = "vehicleControlEnabled";
const char *kKeySettings   = "settingsControlEnabled";
const char *kKeyConfirm    = "confirmActions";
const char *kDefaultBaseUrl = "https://api.openai.com/v1";
const char *kDefaultModel   = "gpt-4o-mini";

const char *kSystemPrompt =
    "You are an AI assistant embedded in QGroundControl, a drone ground control station. "
    "You can read the connected drone's live status and, when permitted, command it using the "
    "provided tools. You can also read and, when permitted, change a whitelisted subset of "
    "QGroundControl application settings (volume, theme, units, map source, fly-view options). "
    "Always call get_vehicle_status before answering questions about the aircraft or before "
    "issuing a control command that depends on current state. When changing a setting, prefer "
    "reading it first (get_setting/list_settings) to learn its allowed values. Be concise. When a "
    "control command could be unsafe, briefly note the risk.";
}

AIChatService::AIChatService(QObject *parent)
    : QObject(parent)
    , _networkManager(new QNetworkAccessManager(this))
    , _tools(new AIVehicleTools(this))
    , _settingsTools(new AISettingsTools(this))
{
    QSettings settings;
    settings.beginGroup(kSettingsGroup);
    _apiKey  = settings.value(kKeyApiKey).toString();
    _baseUrl = settings.value(kKeyBaseUrl, QString::fromLatin1(kDefaultBaseUrl)).toString();
    _model   = settings.value(kKeyModel, QString::fromLatin1(kDefaultModel)).toString();
    _vehicleControlEnabled = settings.value(kKeyControl, false).toBool();
    _settingsControlEnabled = settings.value(kKeySettings, false).toBool();
    _confirmActions = settings.value(kKeyConfirm, true).toBool();
    settings.endGroup();
}

AIChatService::~AIChatService()
{
    stop();
}

AIChatService *AIChatService::instance()
{
    return _aiChatServiceInstance;
}

// __CONFIG_PLACEHOLDER__
void AIChatService::setApiKey(const QString &apiKey)
{
    if (_apiKey == apiKey) {
        return;
    }
    _apiKey = apiKey;
    QSettings settings;
    settings.beginGroup(kSettingsGroup);
    settings.setValue(kKeyApiKey, _apiKey);
    settings.endGroup();
    emit apiKeyChanged();
}

void AIChatService::setBaseUrl(const QString &baseUrl)
{
    if (_baseUrl == baseUrl) {
        return;
    }
    _baseUrl = baseUrl;
    QSettings settings;
    settings.beginGroup(kSettingsGroup);
    settings.setValue(kKeyBaseUrl, _baseUrl);
    settings.endGroup();
    emit baseUrlChanged();
}

void AIChatService::setModel(const QString &model)
{
    if (_model == model) {
        return;
    }
    _model = model;
    QSettings settings;
    settings.beginGroup(kSettingsGroup);
    settings.setValue(kKeyModel, _model);
    settings.endGroup();
    emit modelChanged();
}

void AIChatService::setVehicleControlEnabled(bool enabled)
{
    if (_vehicleControlEnabled == enabled) {
        return;
    }
    _vehicleControlEnabled = enabled;
    QSettings settings;
    settings.beginGroup(kSettingsGroup);
    settings.setValue(kKeyControl, _vehicleControlEnabled);
    settings.endGroup();
    emit vehicleControlEnabledChanged();
}

void AIChatService::setSettingsControlEnabled(bool enabled)
{
    if (_settingsControlEnabled == enabled) {
        return;
    }
    _settingsControlEnabled = enabled;
    QSettings settings;
    settings.beginGroup(kSettingsGroup);
    settings.setValue(kKeySettings, _settingsControlEnabled);
    settings.endGroup();
    emit settingsControlEnabledChanged();
}

void AIChatService::setConfirmActions(bool confirm)
{
    if (_confirmActions == confirm) {
        return;
    }
    _confirmActions = confirm;
    QSettings settings;
    settings.beginGroup(kSettingsGroup);
    settings.setValue(kKeyConfirm, _confirmActions);
    settings.endGroup();
    emit confirmActionsChanged();
}

void AIChatService::clearConversation()
{
    _messages = QJsonArray();
    _conversation.clear();
    emit conversationChanged();
}

void AIChatService::stop()
{
    _pendingToolCalls = QJsonArray();
    _pendingIndex = 0;
    _awaitingCallId.clear();
    if (_reply) {
        _reply->abort();
    }
}

void AIChatService::_setBusy(bool busy)
{
    if (_busy == busy) {
        return;
    }
    _busy = busy;
    emit busyChanged();
}

void AIChatService::_appendConversation(const QString &text)
{
    _conversation += text;
    emit conversationChanged();
}

QString AIChatService::_formatArgs(const QJsonObject &args) const
{
    QStringList parts;
    for (auto it = args.begin(); it != args.end(); ++it) {
        parts.append(it.key() + QStringLiteral("=") + it.value().toVariant().toString());
    }
    return parts.join(QStringLiteral(", "));
}

// __SEND_PLACEHOLDER__
void AIChatService::sendMessage(const QString &text)
{
    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty() || _busy) {
        return;
    }
    if (_apiKey.isEmpty()) {
        _appendConversation(QStringLiteral("[Error] API Key is not configured.\n\n"));
        return;
    }

    QJsonObject userMsg;
    userMsg[QStringLiteral("role")] = QStringLiteral("user");
    userMsg[QStringLiteral("content")] = trimmed;
    _messages.append(userMsg);

    _appendConversation(QStringLiteral("You: ") + trimmed + QStringLiteral("\n\n"));

    _setBusy(true);
    _startRequest();
}

QJsonArray AIChatService::_buildMessages() const
{
    QJsonArray messages;
    QJsonObject system;
    system[QStringLiteral("role")] = QStringLiteral("system");
    system[QStringLiteral("content")] = QString::fromLatin1(kSystemPrompt);
    messages.append(system);
    for (const QJsonValue &msg : _messages) {
        messages.append(msg);
    }
    return messages;
}

void AIChatService::_startRequest()
{
    QString url = _baseUrl;
    while (url.endsWith(QLatin1Char('/'))) {
        url.chop(1);
    }
    url += QStringLiteral("/chat/completions");

    QJsonObject body;
    body[QStringLiteral("model")] = _model;
    body[QStringLiteral("messages")] = _buildMessages();
    // Tool rounds are handled non-streaming for reliable tool_call parsing.
    body[QStringLiteral("stream")] = false;

    QJsonArray tools = AIVehicleTools::toolDefinitions(_vehicleControlEnabled);
    const QJsonArray settingsTools = AISettingsTools::toolDefinitions(_settingsControlEnabled);
    for (const QJsonValue &tool : settingsTools) {
        tools.append(tool);
    }
    body[QStringLiteral("tools")] = tools;

    QNetworkRequest request{QUrl(url)};
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    request.setRawHeader("Authorization", QByteArray("Bearer ") + _apiKey.toUtf8());

    _reply = _networkManager->post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(_reply, &QNetworkReply::finished, this, &AIChatService::_onFinished);
}

void AIChatService::_onFinished()
{
    if (!_reply) {
        return;
    }

    const QNetworkReply::NetworkError netError = _reply->error();
    const QByteArray data = _reply->readAll();
    _reply->deleteLater();
    _reply = nullptr;

    if (netError != QNetworkReply::NoError && netError != QNetworkReply::OperationCanceledError) {
        QString message = QString::fromUtf8(data);
        if (message.isEmpty()) {
            message = QStringLiteral("Network error.");
        }
        _appendConversation(QStringLiteral("[Error] ") + message + QStringLiteral("\n\n"));
        _setBusy(false);
        return;
    }
    if (netError == QNetworkReply::OperationCanceledError) {
        _appendConversation(QStringLiteral("\n[Stopped]\n\n"));
        _setBusy(false);
        return;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        _appendConversation(QStringLiteral("[Error] Invalid response from server.\n\n"));
        _setBusy(false);
        return;
    }

    const QJsonArray choices = doc.object().value(QStringLiteral("choices")).toArray();
    if (choices.isEmpty()) {
        // Surface API-level errors (e.g. auth, model not found).
        const QJsonObject errObj = doc.object().value(QStringLiteral("error")).toObject();
        const QString errMsg = errObj.value(QStringLiteral("message")).toString();
        _appendConversation(QStringLiteral("[Error] ") + (errMsg.isEmpty() ? QStringLiteral("Empty response.") : errMsg) + QStringLiteral("\n\n"));
        _setBusy(false);
        return;
    }

    _handleAssistantMessage(choices.at(0).toObject().value(QStringLiteral("message")).toObject());
}

// __TOOLLOOP_PLACEHOLDER__
void AIChatService::_handleAssistantMessage(const QJsonObject &message)
{
    // Record the assistant message verbatim (it may carry tool_calls that the
    // subsequent tool messages must reference).
    _messages.append(message);

    const QJsonArray toolCalls = message.value(QStringLiteral("tool_calls")).toArray();
    const QString content = message.value(QStringLiteral("content")).toString();

    if (toolCalls.isEmpty()) {
        // Final natural-language answer.
        _appendConversation(QStringLiteral("AI: ") + content + QStringLiteral("\n\n"));
        _setBusy(false);
        return;
    }

    if (!content.isEmpty()) {
        _appendConversation(QStringLiteral("AI: ") + content + QStringLiteral("\n"));
    }

    _pendingToolCalls = toolCalls;
    _pendingIndex = 0;
    _processToolCalls();
}

void AIChatService::_processToolCalls()
{
    while (_pendingIndex < _pendingToolCalls.size()) {
        const QJsonObject call = _pendingToolCalls.at(_pendingIndex).toObject();
        const QString callId = call.value(QStringLiteral("id")).toString();
        const QJsonObject function = call.value(QStringLiteral("function")).toObject();
        const QString name = function.value(QStringLiteral("name")).toString();

        QJsonObject args;
        const QString argsStr = function.value(QStringLiteral("arguments")).toString();
        if (!argsStr.isEmpty()) {
            args = QJsonDocument::fromJson(argsStr.toUtf8()).object();
        }

        const QString argsText = _formatArgs(args);
        _appendConversation(QStringLiteral("  [Tool] ") + name
            + QStringLiteral("(") + argsText + QStringLiteral(")\n"));

        // Control / write tools may need user confirmation before execution.
        if (_toolNeedsConfirmation(name)) {
            _awaitingCallId = callId;
            emit confirmationRequested(callId, name, argsText);
            return; // resumes via confirmToolCall()
        }

        const QJsonObject result = _executeTool(name, args);
        _appendToolResult(callId, name, result);
        _pendingIndex++;
    }

    // All tool calls handled; ask the model to continue.
    _startRequest();
}

void AIChatService::confirmToolCall(const QString &callId, bool approved)
{
    if (callId != _awaitingCallId || _pendingIndex >= _pendingToolCalls.size()) {
        return;
    }
    _awaitingCallId.clear();

    const QJsonObject call = _pendingToolCalls.at(_pendingIndex).toObject();
    const QJsonObject function = call.value(QStringLiteral("function")).toObject();
    const QString name = function.value(QStringLiteral("name")).toString();

    if (approved) {
        QJsonObject args;
        const QString argsStr = function.value(QStringLiteral("arguments")).toString();
        if (!argsStr.isEmpty()) {
            args = QJsonDocument::fromJson(argsStr.toUtf8()).object();
        }
        const QJsonObject result = _executeTool(name, args);
        _appendToolResult(callId, name, result);
    } else {
        QJsonObject denied;
        denied[QStringLiteral("success")] = false;
        denied[QStringLiteral("error")] = QStringLiteral("The user declined to execute this command.");
        _appendToolResult(callId, name, denied);
    }

    _pendingIndex++;
    _processToolCalls();
}

QJsonObject AIChatService::_executeTool(const QString &name, const QJsonObject &args)
{
    if (AISettingsTools::isSettingsTool(name)) {
        return _settingsTools->execute(name, args);
    }
    return _tools->execute(name, args);
}

bool AIChatService::_toolNeedsConfirmation(const QString &name) const
{
    if (!_confirmActions) {
        return false;
    }
    return AIVehicleTools::isControlTool(name) || AISettingsTools::isWriteTool(name);
}

void AIChatService::_appendToolResult(const QString &callId, const QString &name, const QJsonObject &result)
{
    const QByteArray resultJson = QJsonDocument(result).toJson(QJsonDocument::Compact);

    QJsonObject toolMsg;
    toolMsg[QStringLiteral("role")] = QStringLiteral("tool");
    toolMsg[QStringLiteral("tool_call_id")] = callId;
    toolMsg[QStringLiteral("content")] = QString::fromUtf8(resultJson);
    _messages.append(toolMsg);

    _appendConversation(QStringLiteral("  [Result] ") + QString::fromUtf8(resultJson) + QStringLiteral("\n"));
    Q_UNUSED(name)
}



