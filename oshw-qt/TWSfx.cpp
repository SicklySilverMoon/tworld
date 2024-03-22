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
#include <QSoundEffect>
#include <QtNumeric>

void mixAudio(char* destC, char* srcC, qint64 len) {
    qint16* dest = (qint16*)destC;
    qint16* src = (qint16*)srcC;
    qint64 offset = 0;
    while (offset * 2 != len) {
        qint16 res;
        if (qAddOverflow(src[offset], dest[offset], &res)) {
            if (src[offset] > 0) res = CHAR_MAX;
            else res = CHAR_MIN;
        }
        dest[offset] = res;
        offset += 1;
    }
}

qint64 TWSoundMixer::readData(char *data, qint64 reqLen) {
    reqLen = std::min(reqLen, (qint64)(DEFAULT_SND_FREQ / TICKS_PER_SECOND));
    memset(data, 0, reqLen);
    if (paused) return reqLen;
    for (TWSfx* sfx : sounds) {
        if (!sfx || !sfx->finishedDecoding) continue;
        if (!sfx->playing && (sfx->pos == 0 || sfx->repeating)) continue;
        qint64 avalBytes = sfx->len - sfx->pos;
        if (avalBytes > reqLen) {
            mixAudio(data, sfx->bytes + sfx->pos, reqLen);
            sfx->pos += reqLen;
        } else {
            mixAudio(data, sfx->bytes + sfx->pos, avalBytes);
            sfx->pos = 0;
            if (!sfx->repeating) {
                sfx->playing = false;
            } else if (sfx->playing) {
                while (reqLen - avalBytes >= sfx->len) {
                    mixAudio(data + avalBytes, sfx->bytes, sfx->len);
                    avalBytes += sfx->len;
                }
                sfx->pos = reqLen - avalBytes;
                mixAudio(data + avalBytes, sfx->bytes, sfx->pos);
            }
        }

    }
    return reqLen;
};

qint64 TWSoundMixer::writeData(const char* data, qint64 len) {
    return 0;
}

bool TWSoundMixer::isSequential() const {
    return true;
}

TWSfx::TWSfx(QString const& filename, bool repeating, QObject* parent): QObject(parent), repeating(repeating),
        finishedDecoding(false), playing(false), pos(0), bytes{nullptr}, len{0} {
    buf = new QBuffer(this);
    buf->open(QIODevice::ReadWrite);
    decoder = new QAudioDecoder(this);
    connect(decoder, &QAudioDecoder::bufferReady, this, &TWSfx::consumeConversionBuffer);
    connect(decoder, &QAudioDecoder::finished, this, &TWSfx::finishConvertingSound);
    connect(decoder, QOverload<QAudioDecoder::Error>::of(&QAudioDecoder::error), this, &TWSfx::handleConvertionError);

    decoder->setAudioFormat(TWSoundMixer::defaultFormat());
    decoder->setSource(QUrl::fromLocalFile(filename));
    decoder->start();
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
    len = buf->size();
    bytes = new char[buf->size()];
    memcpy(bytes, buf->data().constData(), buf->size());
    finishedDecoding = true;

    decoder->deleteLater();
    decoder = nullptr;
    buf->deleteLater();
    buf = nullptr;
}

TWSoundMixer::TWSoundMixer(QObject* parent) : QIODevice(parent), paused(false) {
    sounds.resize(SND_COUNT);
}

TWSfxManager::TWSfxManager(QObject* parent) :
    QObject(parent),
    enableAudio(false) {
    mixer = new TWSoundMixer(this);
    mixer->open(QIODevice::ReadOnly);
    sink = new QAudioSink(QMediaDevices::defaultAudioOutput(), TWSoundMixer::defaultFormat(), this);
    sink->setBufferSize(DEFAULT_SND_FREQ / TICKS_PER_SECOND);
    sink->start(mixer);
}

void TWSfxManager::EnableAudio(bool bEnabled) {
    enableAudio = bEnabled;
}

void TWSfxManager::LoadSoundEffect(int index, QString szFilename)
{
    mixer->sounds[index] = new TWSfx(szFilename, index >= SND_ONESHOT_COUNT, mixer);
}

TWSfxManager::~TWSfxManager() {
    sink->stop();
    mixer->close();
    for (int index = 0; index < SND_COUNT; index++) {
        if (!mixer->sounds[index]) continue;
        // Defer deletion in case the effect is still playing.
        mixer->sounds[index]->deleteLater();
        mixer->sounds[index] = nullptr;
    }
}

void TWSfxManager::SetAudioVolume(qreal fVolume) {
    sink->setVolume(fVolume);
}

void TWSfxManager::SetSoundEffects(unsigned long sfx) {
    int i;
    unsigned long flag;
    for (i = 0, flag = 1u; i < SND_COUNT; ++i, flag <<= 1)
    {
        TWSfx* sound = mixer->sounds[i];
        if (!sound) continue;
        if (sfx & flag) {
            sound->playing = true;
            if (!sound->repeating) {
                sound->pos = 0;
            }
        } else if (i >= SND_ONESHOT_COUNT) {
            sound->playing = false;
        }
    }
}

void TWSfxManager::StopSoundEffects() {
    for (TWSfx* sfx: mixer->sounds) {
        if (!sfx) continue;
        sfx->pos = 0;
        sfx->playing = false;
    }
}

void TWSfxManager::PauseSoundEffects(bool pause) {
    mixer->paused = pause;
}
