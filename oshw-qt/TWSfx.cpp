#include "TWSfx.h"
#include "defs.h"
#include "err.h"
#include <QFile>
#include <QAudioSink>
#include <QAudioFormat>
#include <QObject>
#include <QMediaDevices>
#include <QAudioDecoder>
#include <QUrl>
#include <QString>
#include <QByteArray>
#include <QBuffer>

TWSfx::TWSfx(QString filename, bool repeating, QObject* parent): QObject(parent), repeating(repeating) {
    buf = new QBuffer();
    buf->open(QIODevice::ReadWrite);
    decoder = new QAudioDecoder(this);
    connect(decoder, &QAudioDecoder::bufferReady, this, &TWSfx::consumeConversionBuffer);
    connect(decoder, &QAudioDecoder::finished, this, &TWSfx::finishConvertingSound);
    connect(decoder, QOverload<QAudioDecoder::Error>::of(&QAudioDecoder::error), this, &TWSfx::handleConvertionError);

    decoder->setAudioFormat(TWSfx::defaultFormat());
    decoder->setSource(QUrl::fromLocalFile(filename));
    decoder->start();

    sink = new QAudioSink(QMediaDevices::defaultAudioOutput(), TWSfx::defaultFormat(), this);
    connect(sink, &QAudioSink::stateChanged, this, &TWSfx::handleStateChanged);
}

void TWSfx::handleConvertionError(QAudioDecoder::Error err) {
    QString errorMessage = decoder->errorString();
    warn("cannot initialize sfx: %s", errorMessage.toStdString().c_str());
}

void TWSfx::consumeConversionBuffer() {
    QAudioBuffer audioBuf = decoder->read();
    audioBuf.detach();
    QByteArray byteArray = QByteArray(audioBuf.constData<char>(), audioBuf.byteCount());
    buf->write(byteArray);
}

void TWSfx::finishConvertingSound() {
    decoder->deleteLater();
    decoder = nullptr;
    buf->seek(0);
}

void TWSfx::play() {
    if (sink->state() == QAudio::ActiveState && repeating) return;
    sink->start(buf);
}

void TWSfx::stop() {
    if (sink->state() != QAudio::ActiveState) return;
    sink->stop();
}

void TWSfx::pause() {
    if (sink->state() != QAudio::ActiveState) return;
    sink->suspend();
}

void TWSfx::resume() {
    if (sink->state() != QAudio::SuspendedState) return
    sink->resume();
}

void TWSfx::setVolume(qreal volume) {
    sink->setVolume(volume);
}

void TWSfx::handleStateChanged(QAudio::State state) {
    switch (state) {
    case QAudio::IdleState:
        if (repeating) {
            // buf->seek(0);
            sink->start(buf);
        } else {
            sink->stop();
        }
        break;
    default:
        break;
    }
}

TWSfxManager::TWSfxManager(QObject* parent) :
    QObject(parent),
    enableAudio(false),
    sounds() {
    sounds.resize(SND_COUNT);
}

void TWSfxManager::EnableAudio(bool bEnabled) {
    enableAudio = bEnabled;
}

void TWSfxManager::LoadSoundEffect(int index, QString szFilename)
{
    FreeSoundEffect(index);
    sounds[index] = new TWSfx(szFilename, index >= SND_ONESHOT_COUNT, this);
}

void TWSfxManager::FreeSoundEffect(int index) {
    if (index < 0 || index >= sounds.size())
        return;

    if (sounds[index])
    {
        // Defer deletion in case the effect is still playing.
        sounds[index]->deleteLater();
        sounds[index] = nullptr;
    }
}

void TWSfxManager::SetAudioVolume(qreal fVolume) {
    for (TWSfx* pSoundEffect : sounds)
    {
        if (pSoundEffect)
            pSoundEffect->setVolume(fVolume);
    }
}

void TWSfxManager::PlaySoundEffect(int index) {
    if (index < 0 || index >= sounds.size())
        return;

    TWSfx* pSoundEffect = sounds[index];
    if (pSoundEffect)
    {
        pSoundEffect->play();
    }
}

void TWSfxManager::StopSoundEffect(int index) {
    if (index < 0 || index >= sounds.size())
        return;

    TWSfx* pSoundEffect = sounds[index];
    if (pSoundEffect) {
        pSoundEffect->stop();
    }
}