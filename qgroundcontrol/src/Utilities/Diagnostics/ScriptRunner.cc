#include "ScriptRunner.h"

#include <QtCore/QApplicationStatic>
#include <QtCore/QFileInfo>
#include <QtCore/QStandardPaths>

Q_APPLICATION_STATIC(ScriptRunner, _scriptRunnerInstance);

ScriptRunner::ScriptRunner(QObject *parent)
    : QObject(parent)
{
}

ScriptRunner::~ScriptRunner()
{
    stopScript();
}

ScriptRunner *ScriptRunner::instance()
{
    return _scriptRunnerInstance;
}

void ScriptRunner::runScript(const QString &filePath)
{
    if (_running) {
        stopScript();
    }

    _process = new QProcess(this);
    _process->setProcessChannelMode(QProcess::MergedChannels);

    connect(_process, &QProcess::readyReadStandardOutput, this, &ScriptRunner::_onReadyReadStdout);
    connect(_process, &QProcess::readyReadStandardError, this, &ScriptRunner::_onReadyReadStderr);
    connect(_process, &QProcess::finished, this, &ScriptRunner::_onFinished);

    QFileInfo fi(filePath);
    _process->setWorkingDirectory(fi.absolutePath());

    QString suffix = fi.suffix().toLower();
    if (suffix == "bat" || suffix == "cmd") {
        _process->start("cmd.exe", {"/c", filePath});
    } else if (suffix == "py") {
        QString python = QStandardPaths::findExecutable("py");
        if (python.isEmpty())
            python = QStandardPaths::findExecutable("python");
        if (python.isEmpty())
            python = QStandardPaths::findExecutable("python3");
        if (python.isEmpty())
            python = QStringLiteral("C:/Users/20949/AppData/Local/Programs/Python/Python314/python.exe");
        _process->start(python, {"-u", filePath});
    } else if (suffix == "ps1") {
        _process->start("powershell", {"-ExecutionPolicy", "Bypass", "-File", filePath});
    } else if (suffix == "sh") {
        _process->start("bash", {filePath});
    } else {
        _process->start("cmd.exe", {"/c", filePath});
    }

    _running = true;
    emit runningChanged();
}

void ScriptRunner::stopScript()
{
    if (_process) {
        _process->kill();
        _process->waitForFinished(3000);
        _process->deleteLater();
        _process = nullptr;
    }
    if (_running) {
        _running = false;
        emit runningChanged();
    }
}

void ScriptRunner::clearOutput()
{
    _output.clear();
    emit outputChanged();
}

void ScriptRunner::_onReadyReadStdout()
{
    _output += QString::fromLocal8Bit(_process->readAllStandardOutput());
    emit outputChanged();
}

void ScriptRunner::_onReadyReadStderr()
{
    _output += QString::fromLocal8Bit(_process->readAllStandardError());
    emit outputChanged();
}

void ScriptRunner::_onFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitStatus);
    _output += QStringLiteral("\n[Process exited with code %1]\n").arg(exitCode);
    emit outputChanged();

    _process->deleteLater();
    _process = nullptr;
    _running = false;
    emit runningChanged();
}
