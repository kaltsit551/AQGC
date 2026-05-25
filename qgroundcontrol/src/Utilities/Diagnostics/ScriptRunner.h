#pragma once

#include <QtCore/QObject>
#include <QtCore/QProcess>
#include <QtQmlIntegration/QtQmlIntegration>

class ScriptRunner : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QString output READ output NOTIFY outputChanged)
    Q_PROPERTY(bool running READ running NOTIFY runningChanged)

public:
    explicit ScriptRunner(QObject *parent = nullptr);
    ~ScriptRunner() override;

    static ScriptRunner *instance();

    QString output() const { return _output; }
    bool running() const { return _running; }

    Q_INVOKABLE void runScript(const QString &filePath);
    Q_INVOKABLE void stopScript();
    Q_INVOKABLE void clearOutput();

signals:
    void outputChanged();
    void runningChanged();

private slots:
    void _onReadyReadStdout();
    void _onReadyReadStderr();
    void _onFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    QProcess *_process = nullptr;
    QString _output;
    bool _running = false;
};
