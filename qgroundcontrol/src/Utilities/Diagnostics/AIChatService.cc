#include "AIChatService.h"

#include <QtCore/QApplicationStatic>
#include <QtCore/QByteArray>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
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
const char *kDefaultBaseUrl = "https://api.openai.com/v1";
const char *kDefaultModel   = "gpt-4o-mini";
}

AIChatService::AIChatService(QObject *parent)
    : QObject(parent)
    , _networkManager(new QNetworkAccessManager(this))
{
    QSettings settings;
    settings.beginGroup(kSettingsGroup);
    _apiKey  = settings.value(kKeyApiKey).toString();
    _baseUrl = settings.value(kKeyBaseUrl, QString::fromLatin1(kDefaultBaseUrl)).toString();
    _model   = settings.value(kKeyModel, QString::fromLatin1(kDefaultModel)).toString();
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

    _history.append(qMakePair(QStringLiteral("user"), trimmed));
    _appendConversation(QStringLiteral("You: ") + trimmed + QStringLiteral("\n\nAI: "));
    _assistantReply.clear();
    _sseBuffer.clear();

    // Build the request URL: {baseUrl}/chat/completions
    QString url = _baseUrl;
    while (url.endsWith(QLatin1Char('/'))) {
        url.chop(1);
    }
    url += QStringLiteral("/chat/completions");

    QJsonArray messages;
    for (const auto &pair : _history) {
        QJsonObject msg;
        msg[QStringLiteral("role")] = pair.first;
        msg[QStringLiteral("content")] = pair.second;
        messages.append(msg);
    }

    QJsonObject body;
    body[QStringLiteral("model")] = _model;
    body[QStringLiteral("messages")] = messages;
    body[QStringLiteral("stream")] = true;

    QNetworkRequest request{QUrl(url)};
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    request.setRawHeader("Authorization", QByteArray("Bearer ") + _apiKey.toUtf8());
    request.setRawHeader("Accept", "text/event-stream");

    _reply = _networkManager->post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(_reply, &QNetworkReply::readyRead, this, &AIChatService::_onReadyRead);
    connect(_reply, &QNetworkReply::finished, this, &AIChatService::_onFinished);

    _setBusy(true);
}

void AIChatService::clearConversation()
{
    _history.clear();
    _conversation.clear();
    emit conversationChanged();
}

void AIChatService::stop()
{
    if (_reply) {
        _reply->abort();
    }
}

void AIChatService::_onReadyRead()
{
    if (!_reply) {
        return;
    }

    _sseBuffer.append(_reply->readAll());

    // SSE frames are separated by newlines; process complete lines only.
    int newlineIndex;
    while ((newlineIndex = _sseBuffer.indexOf('\n')) != -1) {
        QByteArray line = _sseBuffer.left(newlineIndex).trimmed();
        _sseBuffer.remove(0, newlineIndex + 1);

        if (line.isEmpty() || !line.startsWith("data:")) {
            continue;
        }

        QByteArray payload = line.mid(5).trimmed();
        if (payload == "[DONE]") {
            continue;
        }

        QJsonParseError parseError;
        const QJsonDocument doc = QJsonDocument::fromJson(payload, &parseError);
        if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
            continue;
        }

        const QJsonArray choices = doc.object().value(QStringLiteral("choices")).toArray();
        if (choices.isEmpty()) {
            continue;
        }

        const QJsonObject delta = choices.at(0).toObject().value(QStringLiteral("delta")).toObject();
        const QString content = delta.value(QStringLiteral("content")).toString();
        if (!content.isEmpty()) {
            _assistantReply += content;
            _appendConversation(content);
        }
    }
}

void AIChatService::_onFinished()
{
    if (!_reply) {
        return;
    }

    const QNetworkReply::NetworkError error = _reply->error();
    if (error != QNetworkReply::NoError && error != QNetworkReply::OperationCanceledError) {
        QString message = _reply->errorString();
        const QByteArray remaining = _reply->readAll();
        if (!remaining.isEmpty()) {
            message += QStringLiteral("\n") + QString::fromUtf8(remaining);
        }
        _appendConversation(QStringLiteral("\n[Error] ") + message);
    }

    if (!_assistantReply.isEmpty()) {
        _history.append(qMakePair(QStringLiteral("assistant"), _assistantReply));
    }

    _appendConversation(QStringLiteral("\n\n"));

    _reply->deleteLater();
    _reply = nullptr;
    _assistantReply.clear();
    _sseBuffer.clear();
    _setBusy(false);
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
