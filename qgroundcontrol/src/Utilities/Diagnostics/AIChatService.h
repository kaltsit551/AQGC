#pragma once

#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QPair>
#include <QtCore/QString>
#include <QtQmlIntegration/QtQmlIntegration>

class QNetworkAccessManager;
class QNetworkReply;

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

public:
    explicit AIChatService(QObject *parent = nullptr);
    ~AIChatService() override;

    static AIChatService *instance();

    QString apiKey() const { return _apiKey; }
    QString baseUrl() const { return _baseUrl; }
    QString model() const { return _model; }
    QString conversation() const { return _conversation; }
    bool busy() const { return _busy; }

    void setApiKey(const QString &apiKey);
    void setBaseUrl(const QString &baseUrl);
    void setModel(const QString &model);

    Q_INVOKABLE void sendMessage(const QString &text);
    Q_INVOKABLE void clearConversation();
    Q_INVOKABLE void stop();

signals:
    void apiKeyChanged();
    void baseUrlChanged();
    void modelChanged();
    void conversationChanged();
    void busyChanged();

private slots:
    void _onReadyRead();
    void _onFinished();

private:
    void _setBusy(bool busy);
    void _appendConversation(const QString &text);

    QNetworkAccessManager *_networkManager = nullptr;
    QNetworkReply *_reply = nullptr;

    QString _apiKey;
    QString _baseUrl;
    QString _model;
    QString _conversation;
    bool _busy = false;

    // role/content pairs forming the chat history sent to the API
    QList<QPair<QString, QString>> _history;
    // accumulates the assistant's streamed reply for this turn
    QString _assistantReply;
    // buffer for partial SSE lines spanning multiple readyRead calls
    QByteArray _sseBuffer;
};
